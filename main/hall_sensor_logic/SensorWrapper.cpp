#include "SensorWrapper.h"
#include "esp_log.h"
#include "../config.h"
#include "../peripherals/i2c.h"

static const char* TAG = "SensorWrapper";

SensorWrapper::SensorWrapper(uint8_t i2cAddress, uint8_t logicalID)
    : sensor(i2cAddress), id(logicalID), detected(false), sensorAvailable(false), lastZValue(0.0f) {}

bool SensorWrapper::begin() {
    // Check if sensor is present on I2C bus
    sensorAvailable = I2C::devicePresent(sensor.address);

    if (sensorAvailable) {
        ESP_LOGI(TAG, "✅ Sensor #%d (0x%02X): detected and initialized", id, sensor.address);
    } else {
        ESP_LOGD(TAG, "⚠️  Sensor #%d (0x%02X): not present on I2C bus", id, sensor.address);
    }

    return sensorAvailable;
}

bool SensorWrapper::update() {
    // Skip update if sensor is not available
    if (!sensorAvailable) {
        return false;
    }

    if (!sensor.update()) {
        // Only log error once when sensor becomes unavailable
        if (sensorAvailable) {
            ESP_LOGE(TAG, "❌ Sensor #%d (0x%02X): communication error", id, sensor.address);
        }
        return false;
    }

    float z = sensor.z;

    // Only log when there's a significant change or state transition
    if (z > TRIGGER_THRESHOLD && !detected) {
        ESP_LOGI(TAG, "✅ Sensor #%d (0x%02X): Magnet detected! Z = %.2f", id, sensor.address, z);
        detected = true;
        lastZValue = z;
    } else if (z <= TRIGGER_THRESHOLD && detected) {
        ESP_LOGI(TAG, "⚠️  Sensor #%d (0x%02X): Magnet removed, Z = %.2f", id, sensor.address, z);
        // Don't reset detected flag - we want to keep detection state once triggered
        lastZValue = z;
    }

    return detected;
}

bool SensorWrapper::isLogDetected() const {
    return detected;
}

uint8_t SensorWrapper::getID() const {
    return id;
}

void SensorWrapper::reset() {
    detected = false;
    ESP_LOGI(TAG, "Sensor #%d (0x%02X): Detection state reset", id, sensor.address);
}

float SensorWrapper::getZField() const {
    return sensor.z;
}

void SensorWrapper::debug() const {
    ESP_LOGI(TAG, "Sensor #%d (0x%02X): Z = %.2f", id, sensor.address, sensor.z);
}
