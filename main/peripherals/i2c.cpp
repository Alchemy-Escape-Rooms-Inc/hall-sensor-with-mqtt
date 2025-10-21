#include "i2c.h"

#include <algorithm>
#include <vector>

#include "esp_log.h"  // ✅ Required for ESP_LOG* macros
#include "driver/i2c_master.h"
#include "pins.h"

namespace I2C
{
    static const char* TAG = "I2C";

    struct I2CDevice
    {
        uint8_t address;
        i2c_master_dev_handle_t deviceHandle;
    };

    namespace
    {
        bool initialized = false;

        i2c_master_bus_handle_t busHandle;
        std::vector<I2CDevice> slaves;

        constexpr uint32_t I2C_CLK_SPEED = 100000; // 100KHz for better reliability
    }

    void init()
    {
        if (initialized) return;

        i2c_master_bus_config_t i2cBusConfig = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = I2C_SDA,
            .scl_io_num = I2C_SCL,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
        };

        esp_err_t err = i2c_new_master_bus(&i2cBusConfig, &busHandle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(err));
            return;
        }

        initialized = true;
        ESP_LOGI(TAG, "I2C bus initialized on port %d", i2cBusConfig.i2c_port);
    }

    bool getDevice(uint8_t address, i2c_master_dev_handle_t& handle)
    {
        for (const auto& dev : slaves)
        {
            if (dev.address == address)
            {
                handle = dev.deviceHandle;
                return true;
            }
        }
        ESP_LOGW(TAG, "Device 0x%02X not found in registry", address);
        return false;
    }

    bool registerDevice(uint8_t address)
    {
        if (!initialized) return false;

        i2c_device_config_t masterConfig = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = uint8_t(address & 0x7F),
            .scl_speed_hz = I2C_CLK_SPEED,
        };

        i2c_master_dev_handle_t deviceHandle;
        esp_err_t err = i2c_master_bus_add_device(busHandle, &masterConfig, &deviceHandle);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to register device at 0x%02X: %s", address, esp_err_to_name(err));
            return false;
        }

        slaves.push_back(I2CDevice{address, deviceHandle});
        ESP_LOGI(TAG, "Device registered at 0x%02X", address);
        return true;
    }

    bool unregisterDevice(uint8_t address)
    {
        if (!initialized) return false;

        i2c_master_dev_handle_t deviceHandle;
        if (!getDevice(address, deviceHandle)) return true;

        esp_err_t err = i2c_master_bus_rm_device(deviceHandle);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to unregister device at 0x%02X: %s", address, esp_err_to_name(err));
            return false;
        }

        std::erase_if(slaves, [&](const I2CDevice& device) {
            return device.address == address;
        });

        ESP_LOGI(TAG, "Device unregistered at 0x%02X", address);
        return true;
    }

    bool changeAddress(uint8_t oldAddress, uint8_t newAddress)
    {
        if (!initialized) return false;

        i2c_master_dev_handle_t handle;
        if (getDevice(newAddress, handle)) return false;

        if (!unregisterDevice(oldAddress)) return false;
        if (!registerDevice(newAddress)) return false;

        return true;
    }

    bool devicePresent(uint8_t address)
    {
        if (!initialized) return false;

        i2c_device_config_t dev_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = address,
            .scl_speed_hz = I2C_CLK_SPEED,
        };

        i2c_master_dev_handle_t handle;
        esp_err_t err = i2c_master_bus_add_device(busHandle, &dev_config, &handle);
        if (err != ESP_OK) {
            return false;
        }

        uint8_t dummyReg = 0x00;
        uint8_t dummyData[1];
        err = i2c_master_transmit_receive(handle, &dummyReg, 1, dummyData, 1, -1);

        i2c_master_bus_rm_device(handle);
        return err == ESP_OK;
    }

    bool read(uint8_t address, uint8_t* sendPayload, size_t sendSize, uint8_t* receivePayload, size_t receiveSize)
    {
        if (!initialized) return false;

        i2c_master_dev_handle_t deviceHandle;
        if (!getDevice(address, deviceHandle)) return false;

        esp_err_t err = i2c_master_transmit_receive(deviceHandle, sendPayload, sendSize, receivePayload, receiveSize, -1);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Read failed at 0x%02X: %s", address, esp_err_to_name(err));
            ESP_LOGW(TAG, "  Sent reg 0x%02X, expected %d bytes", sendPayload[0], receiveSize);
        } else {
            ESP_LOGI(TAG, "Read success from 0x%02X: %d byte(s)", address, receiveSize);
        }
        return err == ESP_OK;
    }

    bool write(uint8_t address, uint8_t* sendPayload, size_t sendSize)
    {
        if (!initialized) return false;

        i2c_master_dev_handle_t deviceHandle;
        if (!getDevice(address, deviceHandle)) return false;

        esp_err_t err = i2c_master_transmit(deviceHandle, sendPayload, sendSize, -1);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Write failed at 0x%02X: %s", address, esp_err_to_name(err));
            ESP_LOGW(TAG, "  Sending %d bytes starting from reg 0x%02X", sendSize, sendPayload[0]);
        } else {
            ESP_LOGI(TAG, "Write success to 0x%02X: %d byte(s)", address, sendSize);
        }
        return err == ESP_OK;
    }
}
