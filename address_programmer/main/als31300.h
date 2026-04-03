#pragma once
#include <cstdint>

namespace ALS31300 {
    bool programAddress(uint8_t currentAddress, uint8_t newAddress);
    bool readCurrentAddress(uint8_t scanAddress, uint8_t& foundAddress);
}
