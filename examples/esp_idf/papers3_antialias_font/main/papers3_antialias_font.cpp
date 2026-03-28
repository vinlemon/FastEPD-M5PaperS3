//
// Anti-aliased font example for M5Stack PaperS3 (ESP-IDF)
//
// NOTE: The Roboto font .h files were generated with an older version of FastEPD
// where BB_GLYPH was 10 bytes (width/xAdvance as uint8_t). The current library
// uses 12-byte BB_GLYPH (width/xAdvance as uint16_t). The convertOldFont()
// function handles this conversion at runtime.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../../src/FastEPD.h"
#include "../../../../src/Group5.h"
#include "Roboto_Black_40.h"
#include "Roboto_Black_80.h"

FASTEPD epaper;

// 4BPP grayscale waveform LUT for M5PaperS3 (IT8951 controller)
const uint8_t u8_103Grays[] = {
/* 0 */   0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,
/* 1 */   0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  1,  1,  1,  0,  0,  0,  0,
/* 2 */   0,  0,  0,  0,  0,  1,  1,  1,  2,  1,  1,  1,  2,  1,  0,  0,  0,  0,
/* 3 */   0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  2,  1,  0,  0,  0,  0,  0,
/* 4 */   0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  2,  1,  0,  0,  0,  0,  0,  0,
/* 5 */   0,  0,  0,  0,  0,  1,  0,  0,  1,  2,  0,  1,  0,  0,  0,  0,  0,  0,
/* 6 */   0,  0,  0,  0,  0,  1,  2,  0,  1,  2,  0,  1,  0,  0,  0,  0,  0,  0,
/* 7 */   0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  0,  0,  0,
/* 8 */   0,  0,  0,  0,  0,  0,  0,  1,  2,  2,  1,  2,  1,  0,  0,  0,  0,  0,
/* 9 */   0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  2,  0,  0,  0,
/* 10 */  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  2,  0,  0,  0,
/* 11 */  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  2,  1,  2,  0,  0,
/* 12 */  0,  0,  0,  0,  0,  1,  1,  1,  2,  1,  2,  0,  0,  0,  0,  0,  0,  0,
/* 13 */  0,  0,  0,  0,  0,  1,  1,  2,  2,  2,  2,  1,  2,  0,  0,  0,  0,  0,
/* 14 */  1,  1,  1,  1,  1,  1,  2,  2,  1,  2,  2,  0,  0,  0,  0,  0,  0,  0,
/* 15 */  0,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2
};

//
// Old BB_GLYPH format (pre-July 2025): 10 bytes per glyph
// Current BB_GLYPH format: 12 bytes per glyph
//
typedef struct __attribute__((packed)) {
    uint16_t bitmapOffset;
    uint8_t  width;
    uint8_t  xAdvance;
    uint16_t height;
    int16_t  xOffset;
    int16_t  yOffset;
} BB_GLYPH_OLD;

// Convert a font from old 10-byte glyph format to current 12-byte format.
// Returns a newly allocated buffer (in PSRAM) with the converted font data.
uint8_t* convertOldFont(const uint8_t *oldFont, size_t oldFontSize) {
    uint16_t first = oldFont[2] | (oldFont[3] << 8);
    uint16_t last  = oldFont[4] | (oldFont[5] << 8);
    int numGlyphs  = last - first + 1;
    int headerSize = 12; // sizeof(BB_FONT) without flexible array

    int oldDataStart = headerSize + numGlyphs * (int)sizeof(BB_GLYPH_OLD);
    int newDataStart = headerSize + numGlyphs * (int)sizeof(BB_GLYPH);
    int bitmapDataSize = (int)oldFontSize - oldDataStart;
    int newTotalSize = newDataStart + bitmapDataSize;

    uint8_t *newFont = (uint8_t *)heap_caps_malloc(newTotalSize, MALLOC_CAP_SPIRAM);
    if (!newFont) return NULL;

    // Copy header unchanged
    memcpy(newFont, oldFont, headerSize);

    // Convert each glyph: expand width & xAdvance from uint8 to uint16
    for (int i = 0; i < numGlyphs; i++) {
        const BB_GLYPH_OLD *src = (const BB_GLYPH_OLD *)(oldFont + headerSize + i * sizeof(BB_GLYPH_OLD));
        BB_GLYPH *dst = (BB_GLYPH *)(newFont + headerSize + i * sizeof(BB_GLYPH));
        dst->bitmapOffset = src->bitmapOffset;
        dst->width    = src->width;
        dst->xAdvance = src->xAdvance;
        dst->height   = src->height;
        dst->xOffset  = src->xOffset;
        dst->yOffset  = src->yOffset;
    }

    // Copy compressed bitmap data
    memcpy(newFont + newDataStart, oldFont + oldDataStart, bitmapDataSize);
    return newFont;
}

extern "C" void app_main(void)
{
    if (epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000) != BBEP_SUCCESS) {
        printf("Panel init failed\n");
        return;
    }

    // Convert old-format font data to current format
    uint8_t *font40 = convertOldFont(Roboto_Black_40, sizeof(Roboto_Black_40));
    uint8_t *font80 = convertOldFont(Roboto_Black_80, sizeof(Roboto_Black_80));
    if (!font40 || !font80) {
        printf("Font conversion failed\n");
        return;
    }

    epaper.setCustomMatrix(u8_103Grays, sizeof(u8_103Grays));
    epaper.setMode(BB_MODE_4BPP);
    epaper.fillScreen(0xf);

    epaper.setTextColor(BBEP_BLACK, 0xf);

    // Normal font (no anti-aliasing)
    epaper.setFont(font40);
    epaper.drawString("Roboto Black 40pt", 0, 60);

    // Anti-aliased font
    epaper.setFont(font80, true);
    epaper.drawString("Roboto Black 80pt aa", 0, 240);

    epaper.fullUpdate();

    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}
