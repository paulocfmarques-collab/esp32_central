#pragma once
#include <Arduino.h>
#include <NetworkClient.h>

// HTTP chunked com buffer fixo: nao monta a exposicao inteira no heap.
class MetricsChunkStream : public Print {
public:
    explicit MetricsChunkStream(NetworkClient client) : client(client) {}
    size_t write(uint8_t value) override { return write(&value, 1); }
    size_t write(const uint8_t* data, size_t length) override {
        size_t copied = 0;
        while (copied < length && !getWriteError()) {
            size_t amount = min(sizeof(buffer) - used, length - copied);
            memcpy(buffer + used, data + copied, amount);
            used += amount;
            copied += amount;
            if (used == sizeof(buffer) && !flushBlock()) break;
        }
        return copied;
    }
    bool flushBlock() {
        if (getWriteError()) return false;
        if (!used) return true;
        char prefix[16];
        int count = snprintf(prefix, sizeof(prefix), "%x\r\n", unsigned(used));
        if (!writeAll(reinterpret_cast<const uint8_t*>(prefix), count) ||
            !writeAll(buffer, used) || !writeAll(reinterpret_cast<const uint8_t*>("\r\n"), 2)) return false;
        used = 0;
        yield();
        return true;
    }
private:
    NetworkClient client;
    uint8_t buffer[512];
    size_t used = 0;
    bool writeAll(const uint8_t* data, size_t length) {
        size_t sent = 0;
        while (sent < length) {
            size_t amount = client.write(data + sent, length - sent);
            if (!amount) {
                setWriteError();
                client.stop();
                return false;
            }
            sent += amount;
        }
        return true;
    }
};
