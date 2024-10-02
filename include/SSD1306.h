
#pragma once 

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "SSD1306_command_list.h"

using namespace std;

/**
 * @brief This class handles all the i2c communication to the SSD1306 device.  
 * 
 */
class SSD1306 {
    private:
        gpio_num_t sda;
        gpio_num_t scl;

        i2c_master_bus_handle_t i2c_bus;

        i2c_device_config_t     i2c_device_config;
        i2c_master_bus_config_t i2c_bus_config;

        i2c_master_dev_handle_t i2c_device;

        char* TAG;

        void init(void);
        void send_commands(SSD1306_command_list *commands);
        void send_data(uint8_t* buff, size_t n);

    public:

        /**
         * @brief Construct a new SSD1306 object
         * 
         */
        SSD1306();

        /**
         * @brief Construct a new SSD1306 object
         * 
         * @param sda: data pin
         * @param scl: clock pin
         */
        SSD1306(gpio_num_t sda, gpio_num_t scl);

        /**
         * @brief Destroy the SSD1306 object
         * 
         */
        ~SSD1306();

        void on(void);
        void off(void);

        void flush(uint8_t* buffer, size_t tx_buffer_size);



};