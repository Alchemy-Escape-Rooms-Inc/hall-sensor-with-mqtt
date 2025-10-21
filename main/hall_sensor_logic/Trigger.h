#pragma once

#include <string>

// Handles MQTT puzzle solved message trigger
class Trigger {
public:
    void begin();                     // Initializes MQTT
    void sendSolvedMessage();         // Sends solved message to broker
    bool isConnected() const;         // Returns MQTT connection status

private:
    bool connected = false;
};
