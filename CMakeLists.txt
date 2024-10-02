idf_component_register(SRCS "SSD1306_gfx.cpp" "SSD1306.cpp" "SSD1306_command_list.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES driver
                    REQUIRES esp_driver_i2c)
