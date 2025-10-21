#include "als31300.h"
#include "registers.h"

#include <cstdio>
#include <cmath>
#include "esp_log.h"

namespace ALS31300
{
    static const char* TAG = "ALS31300";

    Sensor::ReadCallback Sensor::i2cRead = defaultReadCallback;
    Sensor::WriteCallback Sensor::i2cWrite = defaultWriteCallback;
    Sensor::RegisterCallback Sensor::i2cRegister = defaultRegisterCallback;
    Sensor::UnregisterCallback Sensor::i2cUnregister = defaultUnregisterCallback;
    Sensor::ChangeAddressCallback Sensor::i2cChangeAddress = defaultChangeAddressCallback;

    void Sensor::setCallbacks(RegisterCallback registerCallback, UnregisterCallback unregisterCallback, ChangeAddressCallback changeAddressCallback, WriteCallback writeCallback, ReadCallback readCallback)
    {
        i2cWrite = writeCallback;
        i2cRead = readCallback;
        i2cRegister = registerCallback;
        i2cUnregister = unregisterCallback;
        i2cChangeAddress = changeAddressCallback;
    }

    Sensor::Sensor(uint8_t address) : address(address & 0x7F)
    {
        if (!i2cRegister(address)) {
            ESP_LOGW(TAG, "Failed to register sensor at 0x%02X", address);
        }
    }

    Sensor::~Sensor()
    {
        // ❗ Removed unregister to prevent early device deregistration from temporary objects
        // i2cUnregister(address);
    }

    bool Sensor::update()
    {
        uint16_t newX, newY, newZ;
        uint32_t readData;

        if (!read(0x28, readData)) {
            ESP_LOGW(TAG, "Read failed at reg 0x28 (addr 0x%02X)", address);
            return false;
        }
        Register0x28 reg28{readData};

        newX = reg28.xAxisMsbs << 8;
        newY = reg28.yAxisMsbs << 8;
        newZ = reg28.zAxisMsbs << 8;

        if (!read(0x29, readData)) {
            ESP_LOGW(TAG, "Read failed at reg 0x29 (addr 0x%02X)", address);
            return false;
        }
        Register0x29 reg29{readData};

        newX |= reg29.xAxisLsbs;
        newY |= reg29.yAxisLsbs;
        newZ |= reg29.zAxisLsbs;

        const float filterIntensity = 32.0f;
        x = (float((int16_t)newX) + x * (filterIntensity - 1)) / filterIntensity;
        y = (float((int16_t)newY) + y * (filterIntensity - 1)) / filterIntensity;
        z = (float((int16_t)newZ) + z * (filterIntensity - 1)) / filterIntensity;

        return true;
    }

    bool Sensor::programAddress(uint8_t newAddress)
    {
        newAddress &= 0x7F;

        if (!write(customerAccessRegister, customerAccessCode)) {
            ESP_LOGW(TAG, "Failed to enter Customer Access Mode at addr 0x%02X", address);
            return false;
        }

        uint32_t readData;
        if (!read(0x02, readData)) {
            ESP_LOGW(TAG, "Failed to read 0x02 during address program (addr 0x%02X)", address);
            return false;
        }

        Register0x02 registerData = {readData};
        registerData.slaveAddress = newAddress;

        if (!write(0x02, registerData.raw)) {
            ESP_LOGW(TAG, "Failed to write new address 0x%02X to register 0x02", newAddress);
            return false;
        }

        ESP_LOGI(TAG, "Address programming successful! Power cycle device to check.");
        return true;
    }

    bool Sensor::write(uint8_t reg, uint32_t value)
    {
        uint8_t sendPayload[5] = {
            reg,
            uint8_t(value >> 24 & 0xFF),
            uint8_t(value >> 16 & 0xFF),
            uint8_t(value >> 8  & 0xFF),
            uint8_t(value >> 0  & 0xFF)
        };

        bool result = i2cWrite(address, sendPayload, 5);
        if (!result) {
            ESP_LOGW(TAG, "Write failed at reg 0x%02X (addr 0x%02X)", reg, address);
        }
        return result;
    }

    bool Sensor::read(uint8_t reg, uint32_t& value)
    {
        uint8_t sendPayload = reg;
        uint8_t receivePayload[4];

        bool result = i2cRead(address, &sendPayload, 1, receivePayload, 4);
        if (!result) {
            ESP_LOGW(TAG, "Read failed at reg 0x%02X (addr 0x%02X)", reg, address);
            return false;
        }

        value = receivePayload[0] << 24 |
                receivePayload[1] << 16 |
                receivePayload[2] << 8  |
                receivePayload[3];

        return true;
    }

    uint16_t Sensor::getAngle()
    {
        float currentAngle = angleFromXY(x, y);
        float currentX, currentY;
        xyFromAngle(currentAngle, currentX, currentY);

        float avgX, avgY;
        xyFromAngle(avgAngle, avgX, avgY);

        const float filterIntensity = 10.0f;
        avgX = (currentX + avgX * (filterIntensity - 1)) / filterIntensity;
        avgY = (currentY + avgY * (filterIntensity - 1)) / filterIntensity;

        avgAngle = angleFromXY(avgX, avgY);
        return avgAngle + 0.5f;
    }

    float Sensor::angleFromXY(float x, float y)
    {
        constexpr float PI = float(M_PI);
        constexpr float RAD_TO_DEG = 180.0f / PI;

        float angle = atan2f(y, x) * RAD_TO_DEG;
        if (angle < 0) angle += 360.0f;
        return angle;
    }

    void Sensor::xyFromAngle(float angle, float& x, float& y)
    {
        constexpr float PI = float(M_PI);
        constexpr float DEG_TO_RAD = PI / 180.0f;

        x = cos(angle * DEG_TO_RAD);
        y = sin(angle * DEG_TO_RAD);
    }
}
