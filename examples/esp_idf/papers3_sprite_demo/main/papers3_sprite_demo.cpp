#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../../src/FastEPD.h"
#include "Roboto_Regular_20.h"
#include "courierprime_14.h"

FASTEPD epaper;
FASTEPD sprite1, sprite2;

extern "C" void app_main(void)
{
    printf("Starting FastEPD Sprite Demo on PaperS3\n");
    sprite1.initSprite(128, 64);
    sprite2.initSprite(128, 64);
    if (epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000) != BBEP_SUCCESS) {
        printf("Failed to init panel\n");
        return;
    }
    epaper.fillScreen(BBEP_WHITE);
    epaper.fullUpdate();
    sprite1.fillScreen(BBEP_WHITE);
    sprite1.setFont(courierprime_14);
    sprite1.drawString("Sprite1", 0, 24);
    sprite2.fillScreen(BBEP_WHITE);
    sprite2.setFont(Roboto_Regular_20);
    sprite2.drawString("Sprite2", 0, 32);
    int x1 = 0, y1 = 0, dx1 = 15;
    int x2 = 0, y2 = 256, dx2 = 20;

    while(1) { 
        epaper.fillScreen(BBEP_WHITE); // Clear the frame buffer
        
        // Update positions 
        x1 += dx1;
        if (x1 < 0 || x1 > epaper.width() - 128) dx1 = -dx1;
        
        x2 += dx2;
        if (x2 < 0 || x2 > epaper.width() - 128) dx2 = -dx2;
        
        // Draw sprites at new positions
        epaper.drawSprite(&sprite1, x1, y1);
        epaper.drawSprite(&sprite2, x2, y2);
        
        // Show
        epaper.partialUpdate(true);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
