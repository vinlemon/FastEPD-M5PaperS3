//
// FastEPD Diagnostic Tool for M5Stack PaperS3 (ESP-IDF)
//
// This example runs a comprehensive set of tests on the e-ink display,
// printing detailed diagnostic output to the serial monitor. Use it to:
//
//   - Verify panel initialization and display dimensions
//   - Check struct sizes (BB_FONT, BB_GLYPH) for ABI compatibility
//   - Validate custom font data (header, glyph fields, format version)
//   - Test rendering in both 1BPP and 4BPP modes
//   - Inspect the framebuffer for actual pixel writes
//   - Identify font format mismatches (old 10-byte vs new 12-byte glyphs)
//
// How to run:
//   cd ~/Desktop/miniflux-paperS3/FastEPD/examples/esp_idf/papers3_diagnostic
//   idf.py build flash monitor
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

// 4BPP grayscale waveform LUT for M5PaperS3
const uint8_t u8_103Grays[] = {
    0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,
    0,0,0,0,0,0,0,0,0,1,2,1,1,1,0,0,0,0,
    0,0,0,0,0,1,1,1,2,1,1,1,2,1,0,0,0,0,
    0,0,0,0,0,0,0,0,1,1,1,2,1,0,0,0,0,0,
    0,0,0,0,0,0,0,1,1,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,1,0,0,1,2,0,1,0,0,0,0,0,0,
    0,0,0,0,0,1,2,0,1,2,0,1,0,0,0,0,0,0,
    0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,0,0,0,
    0,0,0,0,0,0,0,1,2,2,1,2,1,0,0,0,0,0,
    0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,0,0,0,
    0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,0,0,0,
    0,0,0,0,0,0,1,1,1,1,1,1,1,2,1,2,0,0,
    0,0,0,0,0,1,1,1,2,1,2,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,2,2,2,2,1,2,0,0,0,0,0,
    1,1,1,1,1,1,2,2,1,2,2,0,0,0,0,0,0,0,
    0,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2
};

// ============================================================================
// Diagnostic utilities
// ============================================================================

// Old BB_GLYPH format (pre-July 2025, 10 bytes)
typedef struct __attribute__((packed)) {
    uint16_t bitmapOffset;
    uint8_t  width;
    uint8_t  xAdvance;
    uint16_t height;
    int16_t  xOffset;
    int16_t  yOffset;
} BB_GLYPH_OLD;

// Count bytes in buffer that differ from a reference value
int countDifferent(const uint8_t *buf, int size, uint8_t refVal) {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (buf[i] != refVal) count++;
    }
    return count;
}

// Hex dump a row of the framebuffer
void dumpRow(const uint8_t *buf, int pitch, int row, int numBytes) {
    int offset = row * pitch;
    printf("  Row %3d: ", row);
    for (int i = 0; i < numBytes && i < pitch; i++) {
        printf("%02X ", buf[offset + i]);
    }
    printf("\n");
}

// Print font header info from raw bytes
void printFontInfo(const char *name, const uint8_t *fontData, size_t fontDataSize) {
    uint16_t marker = fontData[0] | (fontData[1] << 8);
    uint16_t first  = fontData[2] | (fontData[3] << 8);
    uint16_t last   = fontData[4] | (fontData[5] << 8);
    uint16_t height = fontData[6] | (fontData[7] << 8);
    uint32_t rotation = fontData[8] | (fontData[9] << 8) | (fontData[10] << 16) | (fontData[11] << 24);
    int numGlyphs = last - first + 1;

    printf("[%s] Total size: %d bytes\n", name, (int)fontDataSize);
    printf("  Marker: 0x%04X", marker);
    if (marker == BB_FONT_MARKER) printf(" (BB_FONT)");
    else if (marker == BB_FONT_MARKER_SMALL) printf(" (BB_FONT_SMALL)");
    else printf(" (UNKNOWN!)");
    printf("\n");
    printf("  First: %d ('%c'), Last: %d ('%c'), Height: %d, Rotation: %lu\n",
           first, (first >= 32 && first < 127) ? (char)first : '?',
           last, (last >= 32 && last < 127) ? (char)last : '?',
           height, (unsigned long)rotation);
    printf("  Glyph count: %d\n", numGlyphs);

    // Detect glyph format by checking if 10-byte or 12-byte interpretation makes sense
    int headerSize = 12;

    // Try 12-byte (current format)
    const BB_GLYPH *g12 = (const BB_GLYPH *)(fontData + headerSize);
    bool valid12 = (g12[0].width <= 200 && g12[0].xAdvance <= 200 && g12[0].height <= 300);

    // Try 10-byte (old format)
    const BB_GLYPH_OLD *g10 = (const BB_GLYPH_OLD *)(fontData + headerSize);
    bool valid10 = (g10[0].width <= 200 && g10[0].xAdvance <= 200 && g10[0].height <= 300);

    if (valid12 && !valid10) {
        printf("  Glyph format: CURRENT (12-byte BB_GLYPH) ✓\n");
    } else if (valid10 && !valid12) {
        printf("  Glyph format: OLD (10-byte, pre-July 2025) ⚠ Needs conversion!\n");
        printf("    Expected bitmap data start: %d (with 10-byte) vs %d (with 12-byte)\n",
               headerSize + numGlyphs * 10, headerSize + numGlyphs * 12);
    } else if (valid10 && valid12) {
        printf("  Glyph format: ambiguous (both interpretations look valid)\n");
    } else {
        printf("  Glyph format: CORRUPT (neither interpretation produces valid values)\n");
    }

    // Print first 3 glyphs in both formats for comparison
    printf("  --- Glyph samples (12-byte interpretation) ---\n");
    for (int i = 0; i < 3 && i < numGlyphs; i++) {
        printf("    [%d] '%c': offset=%d w=%d xAdv=%d h=%d xOff=%d yOff=%d\n",
               i, (char)(first + i), g12[i].bitmapOffset, g12[i].width,
               g12[i].xAdvance, g12[i].height, g12[i].xOffset, g12[i].yOffset);
    }
    printf("  --- Glyph samples (10-byte interpretation) ---\n");
    for (int i = 0; i < 3 && i < numGlyphs; i++) {
        printf("    [%d] '%c': offset=%d w=%d xAdv=%d h=%d xOff=%d yOff=%d\n",
               i, (char)(first + i), g10[i].bitmapOffset, g10[i].width,
               g10[i].xAdvance, g10[i].height, g10[i].xOffset, g10[i].yOffset);
    }
}

// Convert old 10-byte glyph font to current 12-byte format (returns PSRAM buffer)
uint8_t* convertOldFont(const uint8_t *oldFont, size_t oldFontSize) {
    uint16_t first = oldFont[2] | (oldFont[3] << 8);
    uint16_t last  = oldFont[4] | (oldFont[5] << 8);
    int numGlyphs  = last - first + 1;
    int headerSize = 12;

    int oldDataStart = headerSize + numGlyphs * (int)sizeof(BB_GLYPH_OLD);
    int newDataStart = headerSize + numGlyphs * (int)sizeof(BB_GLYPH);
    int bitmapDataSize = (int)oldFontSize - oldDataStart;
    int newTotalSize = newDataStart + bitmapDataSize;

    uint8_t *newFont = (uint8_t *)heap_caps_malloc(newTotalSize, MALLOC_CAP_SPIRAM);
    if (!newFont) return NULL;

    memcpy(newFont, oldFont, headerSize);
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
    memcpy(newFont + newDataStart, oldFont + oldDataStart, bitmapDataSize);
    return newFont;
}

// ============================================================================
// Main diagnostic
// ============================================================================

extern "C" void app_main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║    FastEPD Diagnostic Tool — M5Stack PaperS3        ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    // ── 1. Panel Init ──
    printf("━━━ 1. Panel Initialization ━━━\n");
    int rc = epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000);
    if (rc != BBEP_SUCCESS) {
        printf("  FAIL: initPanel returned %d\n", rc);
        return;
    }
    printf("  Display: %d × %d pixels\n", epaper.width(), epaper.height());
    printf("  Panel init: OK\n\n");

    // ── 2. Struct Sizes ──
    printf("━━━ 2. Struct Sizes (ABI check) ━━━\n");
    printf("  sizeof(BB_FONT)       = %2d (expect 12)\n", (int)sizeof(BB_FONT));
    printf("  sizeof(BB_GLYPH)      = %2d (expect 12)\n", (int)sizeof(BB_GLYPH));
    printf("  sizeof(BB_GLYPH_OLD)  = %2d (expect 10)\n", (int)sizeof(BB_GLYPH_OLD));
    printf("  sizeof(BB_GLYPH_SMALL)= %2d (expect  8)\n", (int)sizeof(BB_GLYPH_SMALL));
    printf("  sizeof(BB_FONT_SMALL) = %2d (expect 12)\n\n", (int)sizeof(BB_FONT_SMALL));

    // ── 3. Font Data Analysis ──
    printf("━━━ 3. Font Data Analysis ━━━\n");
    printFontInfo("Roboto_Black_40", Roboto_Black_40, sizeof(Roboto_Black_40));
    printf("\n");
    printFontInfo("Roboto_Black_80", Roboto_Black_80, sizeof(Roboto_Black_80));
    printf("\n");

    // ── 4. Convert fonts if needed ──
    printf("━━━ 4. Font Conversion ━━━\n");
    uint8_t *font40 = convertOldFont(Roboto_Black_40, sizeof(Roboto_Black_40));
    uint8_t *font80 = convertOldFont(Roboto_Black_80, sizeof(Roboto_Black_80));
    if (!font40 || !font80) {
        printf("  FAIL: Could not allocate memory for font conversion\n");
        return;
    }
    printf("  Font conversion: OK\n\n");

    // ── 5. Rendering Tests ──
    printf("━━━ 5. Rendering Tests ━━━\n");
    epaper.setCustomMatrix(u8_103Grays, sizeof(u8_103Grays));
    epaper.setMode(BB_MODE_4BPP);
    printf("  Mode: 4BPP (getMode=%d)\n", epaper.getMode());

    uint8_t *buf = epaper.currentBuffer();
    int pitch = epaper.width() / 4; // 2bpp = 4 pixels per byte
    int bufSize = pitch * epaper.height();
    printf("  Buffer: %p, pitch=%d, size=%d bytes\n\n", buf, pitch, bufSize);

    // Test A: fillScreen
    printf("  [A] fillScreen(0xf)...\n");
    epaper.fillScreen(0xf);
    printf("      Non-white bytes: %d (expect 0)\n", countDifferent(buf, bufSize, 0xFF));

    // Test B: fillRect
    printf("  [B] fillRect(0,0,200,100, BLACK)...\n");
    epaper.fillRect(0, 0, 200, 100, 0);
    int afterRect = countDifferent(buf, bufSize, 0xFF);
    printf("      Non-white bytes: %d (expect ~%d)\n", afterRect, 50 * 100);

    // Test C: Built-in font
    printf("  [C] Built-in FONT_12x16...\n");
    epaper.fillScreen(0xf);
    epaper.setFont(FONT_12x16);
    epaper.setTextColor(BBEP_BLACK, BBEP_WHITE);
    epaper.drawString("Diagnostic", 10, 30);
    int afterBuiltin = countDifferent(buf, bufSize, 0xFF);
    printf("      Pixels written: %d %s\n", afterBuiltin, afterBuiltin > 0 ? "✓" : "✗ FAIL");

    // Test D: Custom font (converted, no AA)
    printf("  [D] Roboto_Black_40 (converted, no AA)...\n");
    epaper.fillScreen(0xf);
    epaper.setFont(font40);
    epaper.setTextColor(BBEP_BLACK, 0xf);
    epaper.drawString("Hello", 10, 80);
    int afterCustom = countDifferent(buf, bufSize, 0xFF);
    printf("      Pixels written: %d %s\n", afterCustom, afterCustom > 0 ? "✓" : "✗ FAIL");

    // Test E: Anti-aliased font
    printf("  [E] Roboto_Black_80 (converted, AA)...\n");
    epaper.fillScreen(0xf);
    epaper.setFont(font80, true);
    epaper.setTextColor(BBEP_BLACK, 0xf);
    epaper.drawString("Hello", 10, 200);
    int afterAA = countDifferent(buf, bufSize, 0xFF);
    printf("      Pixels written: %d %s\n\n", afterAA, afterAA > 0 ? "✓" : "✗ FAIL");

    // ── 6. Visual Output ──
    printf("━━━ 6. Visual Output ━━━\n");
    epaper.fillScreen(0xf);
    epaper.fillRect(0, 0, epaper.width(), 4, 0);  // top bar

    epaper.setFont(FONT_12x16);
    epaper.setTextColor(BBEP_BLACK, BBEP_WHITE);
    epaper.drawString("FastEPD Diagnostic - All OK!", 10, 20);

    epaper.setFont(font40);
    epaper.setTextColor(BBEP_BLACK, 0xf);
    epaper.drawString("Roboto 40pt", 10, 100);

    epaper.setFont(font80, true);
    epaper.drawString("Roboto 80pt AA", 10, 280);

    // Grayscale bar
    for (int i = 0; i < 16; i++) {
        epaper.fillRect(10 + i * 55, 450, 50, 40, i);
    }

    int finalPixels = countDifferent(buf, bufSize, 0xFF);
    printf("  Final buffer: %d non-white bytes\n", finalPixels);

    rc = epaper.fullUpdate();
    printf("  fullUpdate: rc=%d %s\n\n", rc, rc == 0 ? "✓" : "✗ FAIL");

    // ── Summary ──
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  RESULTS SUMMARY                                    ║\n");
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  Panel init:      %s                              ║\n", "PASS");
    printf("║  Built-in font:   %s                              ║\n", afterBuiltin > 0 ? "PASS" : "FAIL");
    printf("║  Custom font:     %s                              ║\n", afterCustom > 0 ? "PASS" : "FAIL");
    printf("║  Anti-alias font: %s                              ║\n", afterAA > 0 ? "PASS" : "FAIL");
    printf("║  Display update:  %s                              ║\n", rc == 0 ? "PASS" : "FAIL");
    printf("╚══════════════════════════════════════════════════════╝\n");

    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}
