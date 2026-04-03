#include "als31300.h"
#include "i2c.h"
#include "esp_log.h"

static const char* TAG = "ALS31300";

// Customer access code for write access to EEPROM
static constexpr uint32_t CUSTOMER_ACCESS_CODE = 0x2C413534;
static constexpr uint8_t CUSTOMER_ACCESS_REG = 0x35;
static constexpr uint8_t ADDRESS_REG = 0x02;

static bool writeReg(uint8_t i2cAddr, uint8_t reg, uint32_t value) {
    uint8_t data[5] = {
        reg,
        uint8_t((value >> 24) & 0xFF),
        uint8_t((value >> 16) & 0xFF),
        uint8_t((value >> 8) & 0xFF),
        uint8_t(value & 0xFF)
    };
    return I2C::write(i2cAddr, data, 5);
}

static bool readReg(uint8_t i2cAddr, uint8_t reg, uint32_t& value) {
    uint8_t recvData[4];
    if (!I2C::read(i2cAddr, &reg, 1, recvData, 4)) {
        return false;
    }
    value = (recvData[0] << 24) | (recvData[1] << 16) | (recvData[2] << 8) | recvData[3];
    return true;
}

namespace ALS31300 {

bool programAddress(uint8_t currentAddress, uint8_t newAddress) {
    ESP_LOGI(TAG, "Programming sensor at 0x%02X to new address 0x%02X", currentAddress, newAddress);

    // Enter customer access mode
    if (!writeReg(currentAddress, CUSTOMER_ACCESS_REG, CUSTOMER_ACCESS_CODE)) {
        ESP_LOGE(TAG, "Failed to enter customer access mode");
        return false;
    }
    ESP_LOGI(TAG, "Entered customer access mode");

    // Read current register 0x02 value
    uint32_t regValue;
    if (!readReg(currentAddress, ADDRESS_REG, regValue)) {
        ESP_LOGE(TAG, "Failed to read register 0x02");
        return false;
    }
    ESP_LOGI(TAG, "Current register 0x02 value: 0x%08lX", regValue);

    // Modify address bits (bits 0-6 contain the I2C address)
    regValue = (regValue & 0xFFFFFF80) | (newAddress & 0x7F);
    ESP_LOGI(TAG, "New register 0x02 value: 0x%08lX", regValue);

    // Write new address
    if (!writeReg(currentAddress, ADDRESS_REG, regValue)) {
        ESP_LOGE(TAG, "Failed to write new address");
        return false;
    }

    ESP_LOGI(TAG, "Address programmed successfully!");
    ESP_LOGI(TAG, "POWER CYCLE the sensor to apply the new address.");
    return true;
}

bool readCurrentAddress(uint8_t scanAddress, uint8_t& foundAddress) {
    uint32_t regValue;
    if (!readReg(scanAddress, ADDRESS_REG, regValue)) {
        return false;
    }
    foundAddress = regValue & 0x7F;
    return true;
}

}
