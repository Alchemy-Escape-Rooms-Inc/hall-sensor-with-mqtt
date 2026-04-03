#pragma once
#include <cstdint>
#include <cstddef>

namespace I2C {
    void init();
    bool devicePresent(uint8_t address);
    bool write(uint8_t address, uint8_t* data, size_t len);
    bool read(uint8_t address, uint8_t* sendData, size_t sendLen, uint8_t* recvData, size_t recvLen);
    bool registerDevice(uint8_t address);
    bool unregisterDevice(uint8_t address);
    bool changeAddress(uint8_t oldAddr, uint8_t newAddr);
}
