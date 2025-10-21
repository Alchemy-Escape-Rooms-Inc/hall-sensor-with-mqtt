#include "SensorManager.h"
#include "config.h"
#include "esp_log.h"

static const char* TAG = "SensorManager";

SensorManager::SensorManager() : availableSensors(0) {
    uint8_t baseAddress = 96; // Starting I2C address (0x60)
    ESP_LOGI(TAG, "Creating SensorManager with base address 0x%02X", baseAddress);

    for (uint8_t i = 0; i < SENSOR_COUNT; ++i) {
        uint8_t address = baseAddress + i;
        sensors.emplace_back(address, i);
        ESP_LOGD(TAG, "Sensor #%d assigned to I2C address 0x%02X", i, address);
    }
}

void SensorManager::begin() {
    ESP_LOGI(TAG, "Scanning for sensors...");
    availableSensors = 0;

    for (auto& sensor : sensors) {
        if (sensor.begin()) {
            availableSensors++;
        }
    }

    ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    ESP_LOGI(TAG, "📊 NEW CODE V2 - Sensor initialization complete:");
    ESP_LOGI(TAG, "   Available: %d / %d sensors", availableSensors, (int)sensors.size());
    ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

void SensorManager::updateAll() {
    // Silently update all available sensors
    for (auto& sensor : sensors) {
        if (sensor.isAvailable()) {
            sensor.update();
        }
    }
}

bool SensorManager::allSensorsTriggered() const {
    // Check if all AVAILABLE sensors are triggered
    if (availableSensors == 0) {
        return false;
    }

    int triggered = 0;
    for (const auto& sensor : sensors) {
        if (sensor.isAvailable() && sensor.isLogDetected()) {
            triggered++;
        }
    }

    return (triggered == availableSensors);
}

void SensorManager::resetAll() {
    ESP_LOGI(TAG, "🔄 Resetting all sensor states...");
    for (auto& sensor : sensors) {
        if (sensor.isAvailable()) {
            sensor.reset();
        }
    }
}

int SensorManager::getTriggeredCount() const {
    int count = 0;
    for (const auto& sensor : sensors) {
        if (sensor.isAvailable() && sensor.isLogDetected()) {
            count++;
        }
    }
    return count;
}

int SensorManager::getAvailableCount() const {
    return availableSensors;
}

int SensorManager::getTotalCount() const {
    return sensors.size();
}
