#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "../../../../src/FastEPD.h"
#include "AnimatedGIF.h"
#include "../../../../examples/Arduino/gif_player/Roboto_Black_50.h"
#include "../../../../examples/Arduino/gif_player/1bitsmallcity.h"
#include "../../../../examples/Arduino/gif_player/spiral_1bit.h"

AnimatedGIF gif;
FASTEPD epaper;
uint8_t *pFramebuffer;
int center_x, center_y;

void DrawPixel(int x, int y, uint8_t ucColor)
{
    uint8_t ucMask;
    int index;

    x += center_x;
    y += center_y;
    ucMask = 0x80 >> (x & 7);
    index = (x>>3) + (y * (epaper.width()/8));
    if (ucColor)
        pFramebuffer[index] |= ucMask; // black
    else
        pFramebuffer[index] &= ~ucMask;
}

void GIFDraw(GIFDRAW *pDraw)
{
    uint8_t *s;
    int x, y, iWidth;
    static uint8_t ucPalette[256];

    if (pDraw->y == 0) { // first line, convert palette to 0/1
        for (x = 0; x < 256; x++) {
            uint16_t usColor = pDraw->pPalette[x];
            int gray = (usColor & 0xf800) >> 8; // red
            gray += ((usColor & 0x7e0) >> 2); // plus green*2
            gray += ((usColor & 0x1f) << 3); // plus blue
            ucPalette[x] = (gray >> 9); // 0->511 = 0, 512->1023 = 1
        }
    }
    y = pDraw->iY + pDraw->y; // current line position within the GIF canvas
    iWidth = pDraw->iWidth;
    if (iWidth > epaper.width())
        iWidth = epaper.width();
    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) { // restore to background color
        for (x=0; x<iWidth; x++) {
            if (s[x] == pDraw->ucTransparent)
                s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) { // if transparency used
        uint8_t c, ucTransparent = pDraw->ucTransparent;
        for(x=0; x < iWidth; x++) {
            c = *s++; // each source pixel is always 1 byte (even for 1-bit images)
            if (c != ucTransparent)
                DrawPixel(pDraw->iX + x, y, ucPalette[c]);
        }
    } else {
        s = pDraw->pPixels;
        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        for (x=0; x<pDraw->iWidth; x++)
            DrawPixel(pDraw->iX + x, y, ucPalette[*s++]);
    }
    if (pDraw->y == pDraw->iHeight-1) // last line, render it to the display
        // Tell FastEPD to keep the power on and only update the lines which changed (start_y, end_y)
        epaper.partialUpdate(true, center_y, center_y + gif.getCanvasHeight());
}

extern "C" void app_main(void)
{
    BB_RECT rect;
    int iFrame = 0;

    printf("Starting M5PaperS3 GIF Demo...\n");
    int rc = epaper.initPanel(BB_PANEL_M5PAPERS3);
    if (rc != BBEP_SUCCESS) {
        printf("Failed to init panel! rc = %d\n", rc);
        return;
    }

    gif.begin(LITTLE_ENDIAN_PIXELS);
    pFramebuffer = epaper.currentBuffer();
    
    epaper.fillScreen(BBEP_WHITE);
    epaper.setFont(Roboto_Black_50);
    epaper.setTextColor(BBEP_BLACK);
    epaper.getStringBox("FastEPD GIF Demo", &rect);
    epaper.setCursor((epaper.width() - rect.w)/2, 90);
    epaper.drawString("FastEPD GIF Demo", (epaper.width() - rect.w)/2, 90);
    epaper.fullUpdate(true, true); // start with a full update and leave the power ON
    epaper.setPasses(3, 5);

    int64_t start_time = esp_timer_get_time();

    if (gif.open((uint8_t *)_1bitsmallcity, sizeof(_1bitsmallcity), GIFDraw)) {
        center_x = (epaper.width() - gif.getCanvasWidth())/2;
        center_y = (epaper.height() - gif.getCanvasHeight())/2;
        printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
        while (gif.playFrame(false, NULL)) {
            iFrame++;
        }
        gif.close();
    } else {
        printf("Failed to open GIF!\n");
    }

    int64_t end_time = esp_timer_get_time();
    int time_ms = (end_time - start_time) / 1000;
    
    printf("Played %d frames in %d ms\n", iFrame, time_ms);
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    epaper.clearBlack(true);
    epaper.clearWhite(true);
    epaper.einkPower(false);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
