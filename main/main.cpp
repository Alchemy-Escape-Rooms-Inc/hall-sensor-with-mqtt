#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include <cstring>

#include "config.h"
#include "hall_sensor_logic/SensorManager.h"
#include "hall_sensor_logic/PuzzleState.h"
#include "hall_sensor_logic/Trigger.h"

#include "allegro/als31300.h"
#include "peripherals/i2c.h"
#include "peripherals/pins.h"

// WiFi + MQTT
extern "C" {
    #include "esp_wifi.h"
    #include "esp_event.h"
    #include "mqtt_client.h"
    #include "esp_netif.h"
}

static const char* TAG = "MAIN";
SensorManager* sensorManager = nullptr;
PuzzleState puzzle;
Trigger trigger;

// Event group for Wi-Fi connection
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

// Configuration
#define I2C_STARTUP_DELAY_MS 200
#define I2C_RETRY_COUNT 3
#define I2C_RETRY_DELAY_MS 50
#define SENSOR_CHECK_INTERVAL_MS 1000
#define STATUS_UPDATE_INTERVAL_MS 60000  // 1 minute

// Global MQTT client handle for access from tasks
static esp_mqtt_client_handle_t g_mqtt_client = nullptr;
static uint32_t last_status_update_time = 0;

// Wi-Fi event handler with detailed logging
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "Wi-Fi STA started, attempting to connect...");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
            ESP_LOGI(TAG, "Wi-Fi connected to AP, awaiting IP...");
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "Wi-Fi disconnected. Retrying...");
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "✅ Wi-Fi got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifiConnect() {
    ESP_LOGI(TAG, "📡 Initializing Wi-Fi...");

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password) - 1);

    ESP_LOGI(TAG, "📶 Connecting to SSID: %s", WIFI_SSID);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Block until connected with timeout
    ESP_LOGI(TAG, "⏳ Waiting for Wi-Fi connection...");
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, 
                                           WIFI_CONNECTED_BIT, 
                                           pdFALSE, 
                                           pdTRUE, 
                                           pdMS_TO_TICKS(30000)); // 30 second timeout
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "✅ Wi-Fi connected successfully");
    } else {
        ESP_LOGE(TAG, "❌ Wi-Fi connection timeout!");
    }
}

// Send current puzzle status via MQTT
void sendPuzzleStatus() {
    if (g_mqtt_client == nullptr || sensorManager == nullptr) {
        return;
    }

    // Get count of triggered sensors and available sensors
    int triggered_count = sensorManager->getTriggeredCount();
    int available_sensors = sensorManager->getAvailableCount();

    char status_msg[128];
    if (puzzle.isSolved()) {
        snprintf(status_msg, sizeof(status_msg),
                "Status: SOLVED | Sensors: %d/%d | Uptime: %lu ms",
                triggered_count, available_sensors, (unsigned long)esp_timer_get_time() / 1000);
    } else {
        snprintf(status_msg, sizeof(status_msg),
                "Status: ACTIVE | Sensors: %d/%d | Uptime: %lu ms",
                triggered_count, available_sensors, (unsigned long)esp_timer_get_time() / 1000);
    }

    ESP_LOGI(TAG, "📊 %s", status_msg);
    esp_mqtt_client_publish(g_mqtt_client, "sensor/status", status_msg, 0, 1, 0);
}

// Reset the puzzle state
void resetPuzzle() {
    ESP_LOGI(TAG, "🔄 Resetting puzzle...");
    
    // Reset puzzle state
    puzzle.reset();
    
    // Reset all sensor detection states
    if (sensorManager != nullptr) {
        sensorManager->resetAll();
    }
    
    ESP_LOGI(TAG, "✅ Puzzle reset complete");
    
    if (g_mqtt_client) {
        esp_mqtt_client_publish(g_mqtt_client, "sensor/status", "Puzzle reset - ready for new attempt", 0, 1, 0);
        
        // Send updated status showing 0/X sensors
        sendPuzzleStatus();
    }
}

esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "✅ MQTT connected to broker");
            esp_mqtt_client_subscribe(event->client, "sensor/status", 1);
            esp_mqtt_client_subscribe(event->client, "sensor/debug", 1);
            esp_mqtt_client_subscribe(event->client, "sensor/reset", 1);
            esp_mqtt_client_subscribe(event->client, "sensor/status/request", 1);
            ESP_LOGI(TAG, "📡 Subscribed to MQTT topics");
            
            // Send initial status after connection
            vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to ensure subscription is ready
            sendPuzzleStatus();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "⚠️  MQTT disconnected from broker");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "📤 MQTT message published (msg_id=%d)", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "📥 MQTT message received on topic: %.*s", event->topic_len, event->topic);
            
            // Handle reset command
            if (strncmp(event->topic, "sensor/reset", event->topic_len) == 0) {
                ESP_LOGI(TAG, "🔄 Reset command received via MQTT");
                resetPuzzle();
            }
            // Handle status request
            else if (strncmp(event->topic, "sensor/status/request", event->topic_len) == 0) {
                ESP_LOGI(TAG, "📊 Status request received via MQTT");
                sendPuzzleStatus();
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "❌ MQTT error occurred");
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    mqtt_event_handler_cb(event);
}

esp_mqtt_client_handle_t mqttClientInit() {
    ESP_LOGI(TAG, "🔌 Initializing MQTT client...");
    
    esp_mqtt_client_config_t mqtt_cfg = {};
    // Use broker.address.uri which matches the esp-mqtt config structure for this IDF
    mqtt_cfg.broker.address.uri = MQTT_URI;

    ESP_LOGI(TAG, "📍 MQTT broker: %s", MQTT_URI);

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "❌ Failed to initialize MQTT client");
        return NULL;
    }
    
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, client);
    
    esp_err_t start_result = esp_mqtt_client_start(client);
    if (start_result != ESP_OK) {
        ESP_LOGE(TAG, "❌ Failed to start MQTT client: %s", esp_err_to_name(start_result));
        return NULL;
    }
    
    ESP_LOGI(TAG, "✅ MQTT client started");
    
    // Store client handle globally
    g_mqtt_client = client;
    
    return client;
}

bool probeI2CDevice(uint8_t addr, int retries = I2C_RETRY_COUNT) {
    for (int attempt = 0; attempt < retries; attempt++) {
        if (I2C::devicePresent(addr)) {
            ESP_LOGI(TAG, "✅ Device found at 0x%02X (attempt %d/%d)", addr, attempt + 1, retries);
            return true;
        }
        
        if (attempt < retries - 1) {
            ESP_LOGW(TAG, "⚠️  No response from 0x%02X, retrying... (%d/%d)", addr, attempt + 1, retries);
            vTaskDelay(pdMS_TO_TICKS(I2C_RETRY_DELAY_MS));
        }
    }
    
    ESP_LOGE(TAG, "❌ No device at 0x%02X after %d attempts", addr, retries);
    return false;
}

void checkSensorsTask(void* param) {
    esp_mqtt_client_handle_t client = static_cast<esp_mqtt_client_handle_t>(param);

    uint32_t iteration = 0;
    TickType_t last_status_time = xTaskGetTickCount();

    ESP_LOGI(TAG, "🔄 Sensor monitoring task started");

    while (true) {
        iteration++;

        // Silently update sensors
        sensorManager->updateAll();

        // Check for puzzle solved
        if (sensorManager->allSensorsTriggered() && !puzzle.isSolved()) {
            ESP_LOGI(TAG, "🎉 PUZZLE SOLVED!");
            puzzle.markSolved();
            trigger.sendSolvedMessage();

            char msg[64];
            snprintf(msg, sizeof(msg), "Puzzle Solved at iteration %lu", iteration);
            if (client) {
                esp_mqtt_client_publish(client, "sensor/status", msg, 0, 1, 0);
            }

            // Send full status update
            sendPuzzleStatus();
        }

        // Send periodic status updates (every minute)
        TickType_t current_time = xTaskGetTickCount();
        if ((current_time - last_status_time) >= pdMS_TO_TICKS(STATUS_UPDATE_INTERVAL_MS)) {
            sendPuzzleStatus();
            last_status_time = current_time;
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_CHECK_INTERVAL_MS));
    }
}

bool initializeHardware() {
    ESP_LOGI(TAG, "⚙️  Initializing hardware peripherals...");
    
    // Initialize pins
    ESP_LOGI(TAG, "📌 Initializing GPIO pins...");
    Pins::init();
    ESP_LOGI(TAG, "✅ GPIO pins initialized");
    
    // Initialize sensors
    ESP_LOGI(TAG, "� Starting sensor initialization...");
    sensorManager = new SensorManager();
    if (sensorManager == nullptr) {
        ESP_LOGE(TAG, "❌ Failed to allocate SensorManager!");
        return false;
    }
    ESP_LOGI(TAG, "✅ SensorManager created");

    // Call begin() to initialize each SensorWrapper (no return value)
    sensorManager->begin();
    ESP_LOGI(TAG, "✅ SensorManager initialization invoked");

    // Track how many I2C devices we find during probing
    int devices_found = 0;
    bool any_device_found = false;
    for (uint8_t addr = 0x60; addr <= 0x67; ++addr) {
        bool found = probeI2CDevice(addr);
        if (found) {
            devices_found++;
            any_device_found = true;
        }
    }
    
    ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    ESP_LOGI(TAG, "📊 I2C probe complete: %d device(s) found", devices_found);
    
    if (!any_device_found) {
        ESP_LOGE(TAG, "❌ CRITICAL: No I2C devices found!");
        ESP_LOGE(TAG, "   Check:");
        ESP_LOGE(TAG, "   - Pull-up resistors (4.7kΩ recommended)");
        ESP_LOGE(TAG, "   - SDA/SCL wiring");
        ESP_LOGE(TAG, "   - Sensor power (3.3V)");
        ESP_LOGE(TAG, "   - Common ground");
        return false;
    }
    
    return true;
}

bool initializeSensors() {
    ESP_LOGI(TAG, "🎯 Initializing sensor subsystem...");
    
    // Set up ALS31300 callbacks
    ESP_LOGI(TAG, "🔗 Registering ALS31300 I2C callbacks...");
    ALS31300::Sensor::setCallbacks(
        I2C::registerDevice,
        I2C::unregisterDevice,
        I2C::changeAddress,
        I2C::write,
        I2C::read
    );
    ESP_LOGI(TAG, "✅ Callbacks registered");
    
    // If a SensorManager wasn't already created earlier, create it now.
    if (sensorManager == nullptr) {
        ESP_LOGI(TAG, "🏗️  Creating SensorManager instance...");
        sensorManager = new SensorManager();
        if (sensorManager == nullptr) {
            ESP_LOGE(TAG, "❌ Failed to allocate SensorManager!");
            return false;
        }
        ESP_LOGI(TAG, "✅ SensorManager created");
    }

    // Initialize sensors (SensorManager::begin() returns void in this IDF/driver)
    ESP_LOGI(TAG, "🚀 Starting sensor initialization...");
    sensorManager->begin();
    ESP_LOGI(TAG, "✅ SensorManager initialization invoked");
    
    // Give sensors one more moment before first read
    vTaskDelay(pdMS_TO_TICKS(100));
    
    return true;
}

// Forward declaration so app_main can call probeI2CBus before its definition
bool probeI2CBus();

// Simple scanner for I2C bus: returns true if any device responds
bool probeI2CBus() {
    ESP_LOGI(TAG, "🔎 Probing I2C bus for devices...");
    bool any_found = false;
    for (uint8_t addr = 0x03; addr <= 0x77; ++addr) {
        if (I2C::devicePresent(addr)) {
            ESP_LOGI(TAG, " - Found device at 0x%02X", addr);
            any_found = true;
        }
    }
    ESP_LOGI(TAG, "🔎 I2C scan complete. Any device found: %s", any_found ? "yes" : "no");
    return any_found;
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "  Hall Sensor Puzzle System Starting  ");
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    
    // Initialize NVS (required for WiFi)
    ESP_LOGI(TAG, "💾 Initializing NVS flash...");
    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "⚠️  NVS partition needs erase, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);
    ESP_LOGI(TAG, "✅ NVS initialized");
    
    // Connect to WiFi
    wifiConnect();
    
    // Initialize trigger system
    ESP_LOGI(TAG, "🎯 Initializing trigger system...");
    trigger.begin();
    ESP_LOGI(TAG, "✅ Trigger system ready");
    
    // Initialize hardware
    if (!initializeHardware()) {
        ESP_LOGE(TAG, "❌ Hardware initialization failed! Halting.");
        return;
    }
    
    // Probe I2C bus
    if (!probeI2CBus()) {
        ESP_LOGE(TAG, "❌ I2C bus probe failed! Check hardware connections.");
        ESP_LOGE(TAG, "⛔ System halted - fix hardware issues and reset");
        return;
    }
    
    // Initialize sensors
    if (!initializeSensors()) {
        ESP_LOGE(TAG, "❌ Sensor initialization failed! Halting.");
        ESP_LOGE(TAG, "⛔ System halted - check logs above for details");
        return;
    }
    
    // Initialize MQTT
    esp_mqtt_client_handle_t mqtt_client = mqttClientInit();
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "❌ MQTT initialization failed! Continuing without MQTT...");
        // Don't halt - we can still run without MQTT
    }
    
    // Start sensor monitoring task
    ESP_LOGI(TAG, "🚀 Starting sensor monitoring task...");
    BaseType_t task_result = xTaskCreate(
        &checkSensorsTask, 
        "sensor_check", 
        4096, 
        mqtt_client, 
        5,  // Higher priority
        nullptr
    );
    
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "❌ Failed to create sensor monitoring task!");
        return;
    }
    
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    ESP_LOGI(TAG, "  ✅ System Initialization Complete   ");
    ESP_LOGI(TAG, "  🔄 Sensor monitoring active         ");
    ESP_LOGI(TAG, "═══════════════════════════════════════");
    
    // Send initial status after short delay to allow MQTT connection
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "📊 Sending initial boot status...");
    sendPuzzleStatus();
    
    // Main task can now idle - FreeRTOS scheduler handles everything
    ESP_LOGI(TAG, "💤 Main task entering idle state");
}
