// Implementation for Trigger
#include "hall_sensor_logic/Trigger.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "../config.h"

// Local topic for solved message
static constexpr const char* MQTT_TOPIC = "alchemy/driftwood";

static const char* TAG = "Trigger";
static esp_mqtt_client_handle_t mqttClient = nullptr;

void Trigger::begin() {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_URI;
    mqttClient = esp_mqtt_client_init(&mqtt_cfg);
    esp_err_t err = esp_mqtt_client_start(mqttClient);

    if (err == ESP_OK) {
        connected = true;
        ESP_LOGI(TAG, "MQTT client started");
    } else {
        connected = false;
        ESP_LOGW(TAG, "MQTT client failed to start: %d", err);
    }
}

void Trigger::sendSolvedMessage() {
    if (!connected || mqttClient == nullptr) return;

    const char* msg = "{\"status\": \"solved\"}";
    esp_mqtt_client_publish(mqttClient, MQTT_TOPIC, msg, 0, 1, 0);
    ESP_LOGI(TAG, "Sent MQTT puzzle solved message");
}

bool Trigger::isConnected() const {
    return connected;
}
