#pragma once 

#include <stdint.h>
#include <vector>

class SSD1306_command_list {
    private:
        std::vector<uint8_t> command_data;

        void init(void);

    public:
        SSD1306_command_list();

        void add_commands(uint8_t command);
        void add_commands(uint8_t command, uint8_t arg1);
        void add_commands(uint8_t command, uint8_t arg1, uint8_t arg2);

        uint8_t* command_array(void);
        size_t command_array_len(void);
};