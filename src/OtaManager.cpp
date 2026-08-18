#include "OtaManager.h"

bool OtaManager::beginUpdate(size_t totalBytes) {
    if (totalBytes == 0) {
        snprintf(errorMsg_, sizeof(errorMsg_), "Invalid size 0");
        state_ = OtaState::ERROR;
        return false;
    }

    if (Update.isRunning()) {
        Update.abort();
    }

    totalBytes_ = totalBytes;
    writtenBytes_ = 0;
    errorMsg_[0] = '\0';

    if (!Update.begin(totalBytes_, U_FLASH)) {
        snprintf(errorMsg_, sizeof(errorMsg_), "Update.begin failed");
        state_ = OtaState::ERROR;
        return false;
    }

    state_ = OtaState::READY;
    lastChunkMs_ = millis();
    return true;
}

size_t OtaManager::writeChunk(const uint8_t* data, size_t len) {
    if (state_ != OtaState::READY && state_ != OtaState::FLASHING) {
        return 0;
    }

    if (!data || len == 0) return 0;

    state_ = OtaState::FLASHING;
    lastChunkMs_ = millis();

    size_t bytesWritten = Update.write(const_cast<uint8_t*>(data), len);
    writtenBytes_ += bytesWritten;

    if (bytesWritten != len) {
        snprintf(errorMsg_, sizeof(errorMsg_), "Write failed");
        state_ = OtaState::ERROR;
    }

    return bytesWritten;
}

bool OtaManager::endUpdate() {
    if (state_ != OtaState::FLASHING && state_ != OtaState::READY) {
        return false;
    }

    if (Update.end(false)) { // Do not pad remaining bytes; fail instantly if bytes are missing
        state_ = OtaState::SUCCESS;
        return true;
    } else {
        snprintf(errorMsg_, sizeof(errorMsg_), "Update.end failed (wrote %u / %u bytes)", (unsigned)writtenBytes_, (unsigned)totalBytes_);
        state_ = OtaState::ERROR;
        return false;
    }
}

void OtaManager::abortUpdate() {
    if (Update.isRunning()) {
        Update.abort();
    }
    state_ = OtaState::IDLE;
    writtenBytes_ = 0;
    totalBytes_ = 0;
}

void OtaManager::update() {
    if (state_ == OtaState::READY || state_ == OtaState::FLASHING) {
        if (millis() - lastChunkMs_ > 5000) {
            abortUpdate();
        }
    }
}
