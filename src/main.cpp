#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Update.h>
#include <esp_ota_ops.h>

// Include custom decoupled modules
#include "TaskManager.h"
#include "BluetoothManager.h"
#include "SystemDataProvider.h"
#include "UIManager.h"
#include "PwmManager.h"
#include "NvsManager.h"
#include "BleOtaHandler.h"

// High-Speed NUS Binary Stream OTA State
static bool isNusOtaActive = false;
static size_t nusOtaBytesWritten = 0;
static size_t nusOtaTotalSize = 0;

// ==========================================
// Hardware Configuration (ESP32-C3)
// ==========================================
#define SDA_PIN 5
#define SCL_PIN 6
#define LED_PIN 8 // Onboard Status LED

// OLED Display Configuration
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

// ==========================================
// Global Objects
// ==========================================
BluetoothManager btManager;
SystemDataProvider dataProvider;
UIManager uiManager;
PwmManager pwmManager;
NvsManager nvsManager;
BleOtaHandler bleOtaHandler;

// ==========================================
// SYSTEM TASKS (Scheduled by TaskManager)
// ==========================================

/**
 * @brief Helper function to robustly extract integer values from JSON string keys.
 * Handles variable spacing and key ordering without rigid sscanf formatting.
 */
static int getJsonInt(const std::string& json, const std::string& key, int defaultVal = 0) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos != std::string::npos) {
        size_t colon = json.find(':', pos);
        if (colon != std::string::npos) {
            size_t start = json.find_first_of("0123456789-", colon);
            if (start != std::string::npos) {
                return atoi(&json[start]);
            }
        }
    }
    return defaultVal;
}

/**
 * Task: Manages Bluetooth connectivity and syncs it with DataProvider.
 */
void checkBluetoothTask() {
    btManager.update();
    bool connected = btManager.isConnected();
    dataProvider.setBleConnected(connected);

    static bool wasConnected = false;

    if (connected) {
        dataProvider.setRssi(btManager.getRssi());
        digitalWrite(LED_PIN, LOW);

        // Clear justConnected flag
        btManager.hasJustConnected();
        wasConnected = true;

        // Send config_sync on first CCCD subscription
        if (btManager.hasJustSubscribed()) {
            btManager.sendConfigSync(
                dataProvider.isPwmEnabled(),
                dataProvider.getPwmHighTime(),
                dataProvider.getPwmMinHighTime(),
                dataProvider.getPwmCenterHighTime(),
                dataProvider.getPwmMaxHighTime(),
                dataProvider.getPwmFrequency(),
                dataProvider.getPwmGpioPin()
            );
        }
    } else {
        if (wasConnected) {
            wasConnected = false;
            dataProvider.setPwmEnabled(false);
            dataProvider.setPwmHighTime(dataProvider.getPwmCenterHighTime());
            if (isNusOtaActive || Update.isRunning()) {
                Update.abort();
                isNusOtaActive = false;
            }
        }
        dataProvider.setRssi(-99);
        digitalWrite(LED_PIN, HIGH);
    }

    // Helper lambda to broadcast live telemetry JSON packet
    auto sendCurrentStatus = [&]() {
        btManager.sendStatus(
            dataProvider.isPwmEnabled(),
            dataProvider.getPwmHighTime(),
            dataProvider.getPwmFrequency(),
            dataProvider.getPwmMinHighTime(),
            dataProvider.getPwmCenterHighTime(),
            dataProvider.getPwmMaxHighTime(),
            dataProvider.getPwmGpioPin(),
            dataProvider.getRssi()
        );
    };

    // 1 Hz Periodic Telemetry Broadcast
    static uint32_t lastStatusBroadcastMs = 0;
    uint32_t now = millis();
    if (connected && btManager.isSubscribed() && (now - lastStatusBroadcastMs >= 1000)) {
        lastStatusBroadcastMs = now;
        sendCurrentStatus();
    }

    // Process incoming BLE control commands or fast binary OTA stream chunks
    while (btManager.available()) {
        std::string rxData = btManager.readString();
        btManager.touchRxActivity(); // Reset watchdog on any incoming BLE packet

        // Strict Guard: JSON commands are small text payloads starting with '{' and ending with '}'
        // Binary firmware chunks (240B) do not satisfy this check and will never trigger false positive commands!
        bool isJsonCommand = (rxData.size() > 0 && rxData.size() < 128 && rxData.front() == '{' && rxData.back() == '}');

        if (isJsonCommand) {
            bool isOtaStart = (rxData.find("ota_start") != std::string::npos);
            bool isOtaEnd   = (rxData.find("ota_end") != std::string::npos);
            bool isOtaAbort = (rxData.find("ota_abort") != std::string::npos);

            if (isOtaStart) {
                if (Update.isRunning() || isNusOtaActive) {
                    Update.abort();
                }
                Update.clearError();
                int size = getJsonInt(rxData, "size", 0);
                // Use UPDATE_SIZE_UNKNOWN so Update.end(true) verifies header magic (0xE7) & executes setBootOutput() partition switch
                bool ok = Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
                if (ok) {
                    isNusOtaActive = true;
                    nusOtaBytesWritten = 0;
                    nusOtaTotalSize = size;
                    Serial.printf("[Fast OTA]: Started — Flash erase OK. Target size: %d bytes.\n", size);
                    btManager.write("{\"type\":\"ota_ready\",\"status\":\"ok\"}");
                } else {
                    isNusOtaActive = false;
                    uint8_t err = Update.getError();
                    const char* errStr = Update.errorString();
                    Serial.printf("[Fast OTA ERROR]: Update.begin failed! Code %u: %s\n", err, errStr ? errStr : "Unknown");
                    Update.printError(Serial);
                    btManager.write("{\"type\":\"ota_ready\",\"status\":\"error\",\"msg\":\"Flash erase failed\"}");
                }
            }
            else if (isOtaEnd) {
                Serial.printf("[Fast OTA]: ota_end received. Total written: %u bytes. Running Update.end(true)...\n", nusOtaBytesWritten);
                bool endOk = Update.end(true); // Must return TRUE to guarantee setBootOutput() updated otadata boot partition
                if (isNusOtaActive && endOk) {
                    Serial.printf("[Fast OTA]: SUCCESS - %u / %u bytes flashed to new partition. Rebooting in 1.5s...\n", 
                                  nusOtaBytesWritten, nusOtaTotalSize);
                    btManager.write("{\"type\":\"ota_status\",\"state\":\"success\",\"msg\":\"OTA Completed\"}");
                    delay(1500);
                    ESP.restart();
                } else {
                    uint8_t err = Update.getError();
                    const char* errStr = Update.errorString();
                    Serial.printf("[Fast OTA ERROR]: Update.end failed! Code: %u (%s) | Written: %u | Expected: %u\n", 
                                  err, errStr ? errStr : "Unknown", nusOtaBytesWritten, nusOtaTotalSize);
                    Update.printError(Serial);
                    Update.abort();
                    isNusOtaActive = false;

                    char errBuf[192];
                    snprintf(errBuf, sizeof(errBuf),
                             "{\"type\":\"ota_status\",\"state\":\"error\",\"msg\":\"Verification failed (Code %u: %s, Written: %uB)\"}",
                             err, errStr ? errStr : "Err", nusOtaBytesWritten);
                    btManager.write(std::string(errBuf));
                }
            }
            else if (isOtaAbort) {
                if (isNusOtaActive || Update.isRunning()) {
                    Update.abort();
                    isNusOtaActive = false;
                    Serial.println("[Fast OTA]: Aborted by user command.");
                }
            }
            else {
                // Standard non-OTA JSON control commands
                if (rxData.find("set_pwm_hightime") != std::string::npos) {
                    int val = getJsonInt(rxData, "val", 1500);
                    dataProvider.setPwmHighTime((uint16_t)val);
                }
                else if (rxData.find("set_pwm_enable") != std::string::npos) {
                    if (rxData.find("true") != std::string::npos) {
                        dataProvider.setPwmEnabled(true);
                        sendCurrentStatus();
                    } else if (rxData.find("false") != std::string::npos) {
                        dataProvider.setPwmEnabled(false);
                        sendCurrentStatus();
                    }
                }
                else if (rxData.find("set_pwm_range") != std::string::npos) {
                    int minUs = getJsonInt(rxData, "min", 500);
                    int maxUs = getJsonInt(rxData, "max", 2500);
                    if (minUs > 0 && maxUs > minUs) {
                        dataProvider.setPwmRange((uint16_t)minUs, (uint16_t)maxUs);
                        nvsManager.save(
                            dataProvider.getPwmMinHighTime(), dataProvider.getPwmCenterHighTime(),
                            dataProvider.getPwmMaxHighTime(), dataProvider.getPwmFrequency(),
                            dataProvider.getPwmGpioPin()
                        );
                        sendCurrentStatus();
                    }
                }
                else if (rxData.find("set_pwm_freq") != std::string::npos) {
                    int val = getJsonInt(rxData, "val", 50);
                    if (val >= 10 && val <= 500) {
                        dataProvider.setPwmFrequency((uint32_t)val);
                        nvsManager.save(
                            dataProvider.getPwmMinHighTime(), dataProvider.getPwmCenterHighTime(),
                            dataProvider.getPwmMaxHighTime(), dataProvider.getPwmFrequency(),
                            dataProvider.getPwmGpioPin()
                        );
                        sendCurrentStatus();
                    }
                }
                else if (rxData.find("set_pwm_center") != std::string::npos) {
                    int val = getJsonInt(rxData, "val", 1500);
                    if (val >= dataProvider.getPwmMinHighTime() && val <= dataProvider.getPwmMaxHighTime()) {
                        dataProvider.setPwmCenterHighTime((uint16_t)val);
                        nvsManager.save(
                            dataProvider.getPwmMinHighTime(), dataProvider.getPwmCenterHighTime(),
                            dataProvider.getPwmMaxHighTime(), dataProvider.getPwmFrequency(),
                            dataProvider.getPwmGpioPin()
                        );
                        sendCurrentStatus();
                    }
                }
                else if (rxData.find("set_pwm_pin") != std::string::npos) {
                    int val = getJsonInt(rxData, "val", 0);
                    dataProvider.setPwmGpioPin((uint8_t)val);
                    nvsManager.save(
                        dataProvider.getPwmMinHighTime(), dataProvider.getPwmCenterHighTime(),
                        dataProvider.getPwmMaxHighTime(), dataProvider.getPwmFrequency(),
                        dataProvider.getPwmGpioPin()
                    );
                    sendCurrentStatus();
                }
                else if (rxData.find("ping") != std::string::npos) {
                    // Heartbeat ping
                }
                else if (rxData.find("get_status") != std::string::npos) {
                    sendCurrentStatus();
                }
            }
        }
        else if (isNusOtaActive) {
            // Raw binary firmware chunk (240 bytes) — written directly to flash partition
            size_t written = Update.write((uint8_t*)rxData.data(), rxData.size());
            nusOtaBytesWritten += written;

            static uint32_t lastProgressLogMs = 0;
            static uint32_t lastOtaAckMs = 0;
            uint32_t nowMs = millis();

            if (nowMs - lastProgressLogMs >= 2000) {
                lastProgressLogMs = nowMs;
                Serial.printf("[Fast OTA Progress]: %u bytes written so far...\n", nusOtaBytesWritten);
            }

            // Real-time flash progress telemetry notification sent every 150ms
            if (nowMs - lastOtaAckMs >= 150 || nusOtaBytesWritten >= nusOtaTotalSize) {
                lastOtaAckMs = nowMs;
                char ackBuf[96];
                snprintf(ackBuf, sizeof(ackBuf), "{\"type\":\"ota_ack\",\"written\":%u}", (unsigned int)nusOtaBytesWritten);
                btManager.write(std::string(ackBuf));
            }
        }
    }

    // Process BLEOTA background tasks & reset watchdog during active BLEOTA updates
    if (bleOtaHandler.isRegistered()) {
        btManager.touchRxActivity();
        bleOtaHandler.update();
    }
}

/**
 * Task: Updates ESP32 LEDC hardware PWM generator.
 */
void pwmTask() {
    pwmManager.update(dataProvider);
}

/**
 * Task: Manages OLED UI updates and rendering.
 * Uses dirty-flag + animation-aware gating to minimize SPI bus traffic.
 * Animations (scrolling text, blinking icon) are preserved at ~30 Hz.
 * Static content renders only on data change.
 */
void uiTask() {
    uiManager.update();
    uint32_t now = millis();
    if (uiManager.needsRender(now)) {
        u8g2.clearBuffer();
        uiManager.render(u8g2);
        u8g2.sendBuffer();
        uiManager.clearDirty();
    }
}

// ==========================================
// MAIN APP ROUTINES
// ==========================================

void setup() {
    // 0. Mark currently running app valid to cancel any automatic bootloader rollback
    esp_ota_mark_app_valid_cancel_rollback();

    // 1. Initialize Bluetooth stack & create NUS GATT service
    btManager.begin("ESP32-ServoLab");

    // 2. Register BLEOTA GATT service on the same NimBLE server
    bleOtaHandler.begin(btManager.getServer());

    // 3. Start GATT server & advertise both services
    btManager.startAdvertising("ESP32-ServoLab");

    Serial.begin(115200);
    delay(500);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    if (u8g2.begin()) {
        u8g2.setBusClock(1000000);
        Serial.println("OLED Initialized.");
    }

    // Load settings from NVS, apply to data provider
    uint16_t minUs = 500, centerUs = 1500, maxUs = 2500;
    uint32_t freqHz = 50;
    uint8_t gpioPin = 0;

    if (nvsManager.load(minUs, centerUs, maxUs, freqHz, gpioPin)) {
        Serial.println("[Setup]: Applying NVS settings to data provider.");
        dataProvider.setPwmRange(minUs, maxUs);
        dataProvider.setPwmCenterHighTime(centerUs);
        dataProvider.setPwmHighTime(centerUs); // Start at center
        dataProvider.setPwmFrequency(freqHz);
        dataProvider.setPwmGpioPin(gpioPin);
    } else {
        Serial.println("[Setup]: No NVS settings found, using firmware defaults.");
    }

    // Initialize UI Manager
    uiManager.setup(u8g2, dataProvider);

    // Initialize Hardware PWM with (possibly NVS-loaded) settings
    pwmManager.begin(dataProvider.getPwmGpioPin(), dataProvider.getPwmFrequency());

    Serial.println("System Ready.");

    addTask(checkBluetoothTask);
    addTask(pwmTask);
    addTask(uiTask);
}

void loop() {
    static TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);

    runTasks();

    // Yield control to FreeRTOS background processes
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
}
