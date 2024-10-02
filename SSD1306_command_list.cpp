#include "SSD1306_command_list.h"

#include <stdint.h>
#include <vector>

using namespace std;

SSD1306_command_list::SSD1306_command_list() {
    this->command_data.push_back(0x00);
}

void SSD1306_command_list::add_commands(uint8_t command) {
    this->command_data.push_back(command);
}

void SSD1306_command_list::add_commands(uint8_t command, uint8_t arg1) {
    this->command_data.push_back(command);
    this->command_data.push_back(arg1);
}

void SSD1306_command_list::add_commands(uint8_t command, uint8_t arg1, uint8_t arg2) {
    this->command_data.push_back(command);
    this->command_data.push_back(arg1);
    this->command_data.push_back(arg2);
}

uint8_t* SSD1306_command_list::command_array(void) {
    return this->command_data.data();
}

size_t SSD1306_command_list::command_array_len(void) {
    return this->command_data.size();
}