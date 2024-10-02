#pragma once 

#include "SSD1306.h"
#include "gfxfont.h"

/**
 * @brief 
 * 
 */
class SSD1306_gfx {
    private:
        SSD1306 *device;
        char TAG[12] = "SSD1306_gfx";

        size_t pixel_width;
        size_t pixel_height;

        uint8_t*  tx_buffer[2];
        size_t tx_bytes;

        uint8_t*  display_buffer[2];
        size_t display_bytes;

        uint8_t* front_buffer;
        bool front_buffer_idx;

        SemaphoreHandle_t front_buffer_sem;
        TaskHandle_t refreshTaskHandle;

        bool blankOnDraw;

        const GFXfont *gfxFont; 
        int8_t cursor_x;
        int8_t cursor_y;
        
        void init(void);

        void flipBuffer(void);
        static void refreshTask(void* pParams);

    public:
        SSD1306_gfx(SSD1306 *device, size_t width, size_t height);
        void setPixel(uint8_t x, uint8_t y, bool color);
        bool getPixel(uint8_t x, uint8_t y);

        //primatives
        void drawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color);
        void drawImage(uint8_t* buff);

        void drawChar(int8_t x, int8_t y, unsigned char c);
        size_t write(const char* buffer, size_t size);

        void setCursor(int8_t x, int8_t y) {
            cursor_x = x;
            cursor_y = y;
        }

        void doneDrawing(void);

        void start(void);
        void stop(void);

        unsigned int getByteAddress(uint8_t x, uint8_t y);
        uint8_t getBitMask(uint8_t x, uint8_t y);

};