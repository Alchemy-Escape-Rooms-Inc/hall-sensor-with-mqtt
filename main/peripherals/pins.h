#pragma once

#include <driver/gpio.h>

// ESP32-S3 GPIO definitions (GPIOs 0-48)
constexpr gpio_num_t RSVD_IO0		= GPIO_NUM_0;
constexpr gpio_num_t GPIO_1		    = GPIO_NUM_1;
constexpr gpio_num_t GPIO_2		    = GPIO_NUM_2;
constexpr gpio_num_t RSVD_IO3		= GPIO_NUM_3;
constexpr gpio_num_t GPIO_4		    = GPIO_NUM_4;
constexpr gpio_num_t GPIO_5			= GPIO_NUM_5;
constexpr gpio_num_t GPIO_6	        = GPIO_NUM_6;
constexpr gpio_num_t GPIO_7			= GPIO_NUM_7;
constexpr gpio_num_t GPIO_8			= GPIO_NUM_8;
constexpr gpio_num_t GPIO_9			= GPIO_NUM_9;
constexpr gpio_num_t GPIO_10		= GPIO_NUM_10;
constexpr gpio_num_t GPIO_11		= GPIO_NUM_11;
constexpr gpio_num_t GPIO_12		= GPIO_NUM_12;
constexpr gpio_num_t GPIO_13		= GPIO_NUM_13;
constexpr gpio_num_t GPIO_14		= GPIO_NUM_14;
constexpr gpio_num_t GPIO_15		= GPIO_NUM_15;
constexpr gpio_num_t GPIO_16		= GPIO_NUM_16;
constexpr gpio_num_t GPIO_17		= GPIO_NUM_17;
constexpr gpio_num_t GPIO_18		= GPIO_NUM_18;
constexpr gpio_num_t _INT_USB_DPOS	= GPIO_NUM_19;
constexpr gpio_num_t _INT_USB_DNEG	= GPIO_NUM_20;
constexpr gpio_num_t GPIO_21		= GPIO_NUM_21;
constexpr gpio_num_t NONEXISTENT22	= GPIO_NUM_NC;
constexpr gpio_num_t NONEXISTENT23	= GPIO_NUM_NC;
constexpr gpio_num_t NONEXISTENT24	= GPIO_NUM_NC;
constexpr gpio_num_t NONEXISTENT25	= GPIO_NUM_NC;
constexpr gpio_num_t _INT_SPI_CS1	= GPIO_NUM_26;
constexpr gpio_num_t _INT_SPI_HD	= GPIO_NUM_27;
constexpr gpio_num_t _INT_SPI_WP	= GPIO_NUM_28;
constexpr gpio_num_t _INT_SPI_CS0	= GPIO_NUM_29;
constexpr gpio_num_t _INT_SPI_CLK	= GPIO_NUM_30;
constexpr gpio_num_t _INT_SPI_Q		= GPIO_NUM_31;
constexpr gpio_num_t _INT_SPI_D		= GPIO_NUM_32;
constexpr gpio_num_t GPIO_33		= GPIO_NUM_33;
constexpr gpio_num_t GPIO_34		= GPIO_NUM_34;
constexpr gpio_num_t GPIO_35	    = GPIO_NUM_35;
constexpr gpio_num_t GPIO_36		= GPIO_NUM_36;
constexpr gpio_num_t GPIO_37		= GPIO_NUM_37;
constexpr gpio_num_t GPIO_38		= GPIO_NUM_38;
constexpr gpio_num_t GPIO_39		= GPIO_NUM_39;
constexpr gpio_num_t GPIO_40		= GPIO_NUM_40;
constexpr gpio_num_t GPIO_41		= GPIO_NUM_41;
constexpr gpio_num_t GPIO_42		= GPIO_NUM_42;
constexpr gpio_num_t UART0_TX		= GPIO_NUM_43;
constexpr gpio_num_t UART0_RX		= GPIO_NUM_44;
constexpr gpio_num_t RSVD_IO45		= GPIO_NUM_45;
constexpr gpio_num_t RSVD_IO46		= GPIO_NUM_46;
constexpr gpio_num_t GPIO_47		= GPIO_NUM_47;
constexpr gpio_num_t GPIO_48		= GPIO_NUM_48;

#define I2C_SCL GPIO_NUM_18
#define I2C_SDA GPIO_NUM_15

namespace Pins
{
    void init();
    void initPin(gpio_num_t pin, gpio_mode_t mode, bool defaultHigh);

    void setOutput(gpio_num_t pin, bool high);
    bool getOutput(gpio_num_t pin);
    bool getInput(gpio_num_t pin);

    class DebouncedInput
    {
        static const uint8_t undefinedRead = 0xFF;

        uint8_t debounceCount;
        uint8_t previousRead = undefinedRead;
        uint8_t settledRead = undefinedRead;

        static const uint8_t DEBOUNCE_FILTER = 5;

    public:
        bool debounce(gpio_num_t pin);
        bool get();
    };
}