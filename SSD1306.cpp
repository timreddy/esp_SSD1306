#include "SSD1306.h"
#include "SSD1306_command_list.h"

#include <memory.h>

#include "esp_err.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

SSD1306::SSD1306()
{
    this->sda = GPIO_NUM_21;
    this->scl = GPIO_NUM_22;
    this->init();
}

SSD1306::SSD1306(gpio_num_t sda, gpio_num_t scl)
{
    this->sda = sda;
    this->scl = scl;
    this->init();
}

void SSD1306::init(void) {
        this->TAG = (char*) calloc(32, sizeof(char));
        snprintf(this->TAG, 32, "SSD1306 (clk: %d, sda: %d)", this->scl, this->sda);

            //i2c_bus_handle_t        i2c_bus;
        //i2c_device_config_t     i2c_device_config;
        this->i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
        this->i2c_bus_config.i2c_port = -1;
        this->i2c_bus_config.scl_io_num = this->scl;
        this->i2c_bus_config.sda_io_num = this->sda;
        this->i2c_bus_config.glitch_ignore_cnt = 7;
        this->i2c_bus_config.intr_priority = 0;
        this->i2c_bus_config.flags.enable_internal_pullup = true;
        this->i2c_bus_config.trans_queue_depth = 0;

        ESP_ERROR_CHECK(i2c_new_master_bus(&this->i2c_bus_config, &this->i2c_bus));
        if(i2c_master_probe(this->i2c_bus, 0x3C, -1) == ESP_OK) {
            ESP_LOGI(SSD1306::TAG, "SSD1306 device detected");

            this->i2c_device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
            this->i2c_device_config.device_address = 0x3C;
            this->i2c_device_config.scl_speed_hz = 400000;   // 400 kHz
            this->i2c_device_config.scl_wait_us = 0;

            ESP_ERROR_CHECK(i2c_master_bus_add_device(this->i2c_bus, &this->i2c_device_config, &this->i2c_device));
            ESP_LOGI(SSD1306::TAG, "SSD1306 device connected");
            vTaskDelay(100); //needed to let the SSD1306 turn on
        } else {
            ESP_LOGE(SSD1306::TAG, "No SSD1306 device detected");
        }


}

SSD1306::~SSD1306() {
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(this->i2c_device));
    ESP_ERROR_CHECK(i2c_del_master_bus(this->i2c_bus));
}

void SSD1306::send_commands(SSD1306_command_list *commands) {
    ESP_ERROR_CHECK(i2c_master_transmit(this->i2c_device, commands->command_array(), commands->command_array_len(),-1));
}

void SSD1306::send_data(uint8_t* buffer, size_t length) {
    ESP_ERROR_CHECK(i2c_master_transmit(this->i2c_device, buffer, length, -1)); 
}

void SSD1306::on(void) {
    SSD1306_command_list initialization;

    //set clock divider
    initialization.add_commands(0x5D, 0x80);
    //set mux
    initialization.add_commands(0xA8, 31);
    //set display offet
    initialization.add_commands(0xD3, 0x00);
    //set start line to 0
    initialization.add_commands(0x40);
    //set charge pump 
    initialization.add_commands(0x8D, 0x14);
    //set memory mode
    initialization.add_commands(0x20, 0x00);
    //set segment remapping
    initialization.add_commands(0xA1);
    //set com scan dec
    initialization.add_commands(0xC8);
    //set com pins
    initialization.add_commands(0xDA, 0x02);
    //set contrast
    initialization.add_commands(0x81, 0xAF);
    //set precharge
    initialization.add_commands(0xD9, 0xF1);
    //set v com detect
    initialization.add_commands(0xD8, 0x40);
    //resume display
    initialization.add_commands(0xA4);
    //normal display
    initialization.add_commands(0xA6);
    //deactivate scroll
    initialization.add_commands(0x2E);
    //display on
    initialization.add_commands(0xAF);

    //ESP_ERROR_CHECK(i2c_master_transmit(this->i2c_device, initialization.command_array(), initialization.command_array_len(),-1));
    this->send_commands(&initialization);
    vTaskDelay(100); //required after 0xAF for display to turn on
    ESP_LOGI(TAG, "Display on");
}

void SSD1306::off(void) {
    SSD1306_command_list shutdown;
    shutdown.add_commands(0xAE);
    this->send_commands(&shutdown);
}

void SSD1306::flush(uint8_t* tx_buff, size_t tx_buffer_size) {
    //SSD1306_command_list pre_write;
    SSD1306_command_list pre_write;
    pre_write.add_commands(0x20, 0x00);
    pre_write.add_commands(0x22, 0, 4);
    pre_write.add_commands(0x21, 0, 127);
  
    this->send_commands(&pre_write);
    this->send_data(tx_buff, tx_buffer_size);
}