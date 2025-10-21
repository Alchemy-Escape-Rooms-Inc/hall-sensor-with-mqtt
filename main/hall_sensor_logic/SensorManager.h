// SensorManager.h - Add this method declaration to your existing header

#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <vector>
#include "SensorWrapper.h"

class SensorManager {
public:
    SensorManager();
    void begin();
    void updateAll();
    bool allSensorsTriggered() const;
    void resetAll();

    int getTriggeredCount() const;
    int getAvailableCount() const;  // Get number of detected sensors
    int getTotalCount() const;      // Get total configured sensors

private:
    std::vector<SensorWrapper> sensors;
    int availableSensors = 0;
};

#endif // SENSORMANAGER_H
