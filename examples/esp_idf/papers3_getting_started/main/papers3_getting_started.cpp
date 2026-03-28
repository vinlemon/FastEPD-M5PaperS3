#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../../src/FastEPD.h"
#include "../../../../Fonts/Roboto_Black_40.h"

FASTEPD epaper;

extern "C" void app_main(void)
{
    BB_RECT rect;
    int i, j;
    
    // initialize the I/O and memory
    printf("Initializing M5PaperS3 panel...\n");
    int rc = epaper.initPanel(BB_PANEL_M5PAPERS3);
    if (rc != BBEP_SUCCESS) {
        printf("Failed to init panel! rc = %d\n", rc);
        return;
    }

    // The default drawing mode is 1-bit per pixel
    epaper.clearWhite(true); // fill the current image buffer and eink panel with white
    epaper.setFont(Roboto_Black_40);
    epaper.setTextColor(BBEP_BLACK);
    epaper.setCursor(0, 80); // TTF means Y==0-> baseline of characters, not the top
    for (i=0; i<8; i++) { // show how fast partial updates are
        epaper.getStringBox("Hello FastEPD!", &rect);
        // Tell FastEPD to only update the lines which changed (faster)
        epaper.drawString("Hello FastEPD!", 0, 80);
        epaper.partialUpdate(true, rect.y, rect.y + rect.h - 1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // now switch to 4-bpp grayscale mode
    epaper.setMode(BB_MODE_4BPP);
    epaper.fillScreen(15); // fill with 4-bit white (0=black, 15=white)
    
    // Display a grayscale spectrum
    for (i=0, j=0; i<epaper.height(); i+=epaper.height()/16, j++) {
      epaper.fillRect(0, i, epaper.width(), epaper.height()/16, j);
    }
    epaper.fullUpdate(true);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    epaper.setMode(BB_MODE_1BPP); // now switch back to 1-bpp mode and do a regional update
    epaper.fillRect(100, 100, 200, 200, BBEP_WHITE);
    epaper.drawRect(100, 100, 200, 200, BBEP_BLACK);
    epaper.setFont(FONT_12x16);
    epaper.setTextColor(BBEP_BLACK);
    epaper.drawString("1-bpp", 170, 160);
    epaper.drawString("Regional Update", 110, 192);
    rect.x = rect.y = 100;
    rect.h = rect.w = 200;
    epaper.fullUpdate(true, false, &rect); // update only the rectangle
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Now update another rectangle in grayscale mode
    epaper.setMode(BB_MODE_4BPP);
    epaper.fillRect(600, 100, 200, 200, 0x8);
    epaper.setFont(FONT_12x16);
    epaper.setTextColor(0xe);
    epaper.drawString("4-bpp", 670, 160);
    epaper.drawString("Regional Update", 610, 192);
    rect.x = 600;
    rect.y = 100;
    rect.h = rect.w = 200;
    epaper.fullUpdate(true, false, &rect); // update only the rectangle
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
