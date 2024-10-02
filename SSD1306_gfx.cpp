#include "SSD1306_gfx.h"
#include "SSD1306.h"

#include <memory.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gfxfont.h"
#include "FreeMono9pt7b.h"

SSD1306_gfx::SSD1306_gfx(SSD1306 *device, size_t width, size_t height) {
    this->device = device;
    this->pixel_width = width;
    this->pixel_height = height;
    this->init();
}

void SSD1306_gfx::init(void) {
    ESP_LOGI(this->TAG, "Created.");
    this->display_bytes = this->pixel_width * this->pixel_height / 8;
    this->tx_bytes      = this->display_bytes + 1;

    this->tx_buffer[0] = (uint8_t*)calloc(this->tx_bytes, sizeof(uint8_t));
    this->tx_buffer[1] = (uint8_t*)calloc(this->tx_bytes, sizeof(uint8_t));

    if(this->tx_buffer[0] == NULL) { ESP_ERROR_CHECK(ESP_ERR_NO_MEM); }
    if(this->tx_buffer[1] == NULL) { ESP_ERROR_CHECK(ESP_ERR_NO_MEM); }

    this->tx_buffer[0][0] = 0x40; // data transmission code
    this->tx_buffer[1][0] = 0x40; // data transmission code

    this->display_buffer[0] = &(this->tx_buffer[0][1]);
    this->display_buffer[1] = &(this->tx_buffer[1][1]);

    this->front_buffer_idx = 0;
    this->front_buffer = this->display_buffer[this->front_buffer_idx];

    this->front_buffer_sem = xSemaphoreCreateMutex();

    this->blankOnDraw = 1;

    this->gfxFont = &(FreeMono9pt7b);
    this->cursor_x = 0;
    this->cursor_y = 0;

    ESP_LOGI(TAG, "tx_buffers %x %x, display_buffers %x %x", 
        (unsigned int)this->tx_buffer[0],
        (unsigned int)this->tx_buffer[1],
        (unsigned int)this->display_buffer[0],
        (unsigned int)this->display_buffer[1]
        );
    // return value?
    xTaskCreate(SSD1306_gfx::refreshTask, "SSD1306_gfx_thread", 4096, this, 20, &this->refreshTaskHandle);

}

void SSD1306_gfx::start(void) {
    vTaskResume(this->refreshTaskHandle);

}

void SSD1306_gfx::stop(void) {
    vTaskSuspend(this->refreshTaskHandle);
}


/**
 * @brief Flip the back buffer to the front buffer. Do this within a mutex so that this cannot happen
 * while the screen is being drawn. That is beecause there is a possibility that another thread could start
 * writing to the same buffer that is being drawn, creating tearing. 
 * 
 */
void SSD1306_gfx::flipBuffer(void) {
    xSemaphoreTake(this->front_buffer_sem, -1);
    this->front_buffer_idx ^= 0x01;
    xSemaphoreGive(this->front_buffer_sem);

    if(blankOnDraw) {
        memset(this->display_buffer[!(this->front_buffer_idx)], 0, this->display_bytes);
    }
}

unsigned int SSD1306_gfx::getByteAddress(uint8_t x, uint8_t y) {
    return (y >> 3) * this->pixel_width + x;
}

uint8_t SSD1306_gfx::getBitMask(uint8_t x, uint8_t y) {
    return 1 << (y & 7);
}

void SSD1306_gfx::setPixel(uint8_t x, uint8_t y, bool color) {
    if(x >= this->pixel_width || y >= this->pixel_height) { return; }

    uint8_t  *dst      = this->display_buffer[!this->front_buffer_idx];
    uint32_t byte      = this->getByteAddress(x, y);
    uint8_t  bit_mask  = this->getBitMask(x, y);

    //ESP_LOGI(TAG, "(%d, %d) -> (%d, %d)", x, y, byte, bit_mask);
    if(color) { dst[byte] |= bit_mask;  }
    else      { dst[byte] &= ~bit_mask; }
}

bool SSD1306_gfx::getPixel(uint8_t x, uint8_t y) {
    if(x >= this->pixel_width || y >= this->pixel_height) { return 0; }

    uint8_t  *dst      = this->display_buffer[!this->front_buffer_idx];
    uint32_t byte      = this->getByteAddress(x, y);
    uint8_t  bit_mask  = this->getBitMask(x, y);

    return ((dst[byte] & bit_mask) > 0);
}

void SSD1306_gfx::drawImage(uint8_t* buff) {
    memcpy(this->display_buffer[!(this->front_buffer_idx)], buff, this->display_bytes);
}

void SSD1306_gfx::drawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color) {
    //lazy and slow way to write this for now. Better would be to write byte.
    for(uint8_t x_i = 0; x_i < width; x_i++) {
        for(uint8_t y_i = 0; y_i < height; y_i++) {
            this->setPixel(x + x_i, y + y_i, color);
        }
    }

    //faster implementation would:
    // Question -- where does clipping happen?
    // 
    //align the draw to byte boundaries
    //upper left byte: this->getByteAddress(x, y);
    //upper right byte: this>getByteAddress(x+width, y)
    //lower left byte: this>getByteAddress(x, y+height)
    //upper right byte: this>getByteAddress(x+width, y+height)
    //
    //then figure out the bitmasks for each byte
}
/**
 * @brief When done drawing, call this to draw to the screen. This will flip the drawing buffer to the front buffer, 
 * and then notify the refresh task to flush to the screen. 
 * 
 */
void SSD1306_gfx::doneDrawing(void) {
    this->flipBuffer();
    xTaskNotifyGive(this->refreshTaskHandle);
}

/**
 * @brief This function handles screen refreshes. This hangs out in the background waiting for notification that
 * the front buffer is ready for a refresh. Once notified, it will flush that buffer to the i2c device. A mutex is 
 * used to endure that the front buffer is not changed while it is being flushed.
 * 
 */
void SSD1306_gfx::refreshTask(void* pParam) {
    SSD1306_gfx *g = (SSD1306_gfx*) pParam;

    while(1) { //perhaps put in code to exit later. Also could just suspend the thread.
        ulTaskNotifyTake(pdTRUE, -1);
        xSemaphoreTake(g->front_buffer_sem, -1);
        g->device->flush(g->tx_buffer[g->front_buffer_idx], g->tx_bytes);
        xSemaphoreGive(g->front_buffer_sem);
    }
}

/**
 * @brief Ported from the Adafruit_GFX library
 * 
 * @param x 
 * @param y 
 * @param c 
 */
void SSD1306_gfx::drawChar(int8_t x, int8_t y, unsigned char c) {
    // Character is assumed previously filtered by write() to eliminate
    // newlines, returns, non-printable characters, etc.  Calling
    // drawChar() directly with 'bad' characters of font may cause mayhem!

    c -= (uint8_t)this->gfxFont->first;
    GFXglyph *glyph = &(gfxFont->glyph[c]);
    uint8_t *bitmap = this->gfxFont->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t w = glyph->width;
    uint8_t h = glyph->height;
    int8_t xo = glyph->xOffset;
    int8_t yo = glyph->yOffset;
    uint8_t xx, yy, bits = 0, bit = 0;

    for (yy = 0; yy < h; yy++) {
      for (xx = 0; xx < w; xx++) {
        if (!(bit++ & 7)) {
          bits = bitmap[bo++];
        }
        if (bits & 0x80) {
          this->setPixel(x + xo + xx, y + yo + yy, 1);
        }
        bits <<= 1;
      }
    }
}


size_t SSD1306_gfx::write(const char *buffer, size_t size) {
    size_t n = 0;
    while(size--) {
        char c = buffer[n++];
        if (c == '\n') {
            cursor_x = 0;
            cursor_y += (uint8_t)gfxFont->yAdvance;
        } else if (c != '\r') {
            uint8_t first = gfxFont->first;

            if ((c >= first) && (c <= (uint8_t)gfxFont->last)) {
               GFXglyph *glyph = &(this->gfxFont->glyph[c - first]);
               uint8_t w = glyph->width;
               uint8_t h = glyph->height;

                if ((w > 0) && (h > 0)) { // Is there an associated bitmap?
                    drawChar(cursor_x, cursor_y, c);
                }
                cursor_x += (uint8_t)glyph->xAdvance;
            }
        }
    }

    return n;
}
