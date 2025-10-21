#pragma once
#include <cstdint>
#include "allegro/als31300.h"

class SensorWrapper {
public:
    SensorWrapper(uint8_t i2cAddress, uint8_t logicalID);

    bool begin();  // Returns true if sensor is detected
    bool update();
    bool isLogDetected() const;
    bool isTriggered() const { return isLogDetected(); }  // optional alias
    bool isAvailable() const { return sensorAvailable; }  // Check if sensor is present
    uint8_t getID() const;
    void reset();
    float getZField() const;
    void debug() const;

private:
    ALS31300::Sensor sensor;
    uint8_t id;
    bool detected = false;
    bool sensorAvailable = false;
    float lastZValue = 0.0f;
};
