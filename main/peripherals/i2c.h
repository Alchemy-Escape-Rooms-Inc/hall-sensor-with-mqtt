#pragma once

#include <cstdint>
#include <cstddef>

namespace I2C
{
    void init();
    bool devicePresent(uint8_t address);
    bool read(uint8_t address, uint8_t* sendPayload, size_t sendSize, uint8_t* receivePayload, size_t receiveSize);
    bool write(uint8_t address, uint8_t* sendPayload, size_t sendSize);
    bool registerDevice(uint8_t address);
    bool unregisterDevice(uint8_t address);
    bool changeAddress(uint8_t oldAddress, uint8_t newAddress);
}