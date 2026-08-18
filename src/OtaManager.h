#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <Update.h>

enum class OtaState {
    IDLE,
    READY,
    FLASHING,
    SUCCESS,
    ERROR
};

/**
 * @file OtaManager.h
 * @brief Bluetooth LE Over-The-Air (OTA) Firmware Flash Manager.
 *
 * Manages ESP32 Update.h partition erasing, binary chunk writing,
 * CRC verification, and automatic reboot.
 */
class OtaManager {
public:
    OtaManager() = default;

    /**
     * @brief Prepares flash partition for OTA update.
     * @param totalBytes Size of the binary firmware image.
     * @return true if partition was prepared successfully.
     */
    bool beginUpdate(size_t totalBytes);

    /**
     * @brief Writes a chunk of binary firmware payload to flash.
     * @param data Pointer to binary chunk buffer.
     * @param len Length of binary chunk buffer in bytes.
     * @return Number of bytes successfully written to flash.
     */
    size_t writeChunk(const uint8_t* data, size_t len);

    /**
     * @brief Finalizes update, verifies CRC, and schedules automatic reboot.
     * @return true if firmware update succeeded.
     */
    bool endUpdate();

    /** @brief Aborts current OTA update session. */
    void abortUpdate();

    /** @brief Periodically checks for inactivity timeouts (5s watchdog). */
    void update();

    /** @brief Returns current OTA state. */
    OtaState getState() const { return state_; }

    /** @brief Returns total expected firmware size in bytes. */
    size_t getTotalBytes() const { return totalBytes_; }

    /** @brief Returns number of bytes transferred/written so far. */
    size_t getWrittenBytes() const { return writtenBytes_; }

    /** @brief Returns error description string if update failed. */
    const char* getLastErrorStr() const { return errorMsg_; }

private:
    OtaState state_ = OtaState::IDLE;
    size_t totalBytes_ = 0;
    size_t writtenBytes_ = 0;
    uint32_t lastChunkMs_ = 0;
    char errorMsg_[64] = "";
};

#endif // OTA_MANAGER_H
