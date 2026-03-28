#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../../src/FastEPD.h"
#include "smiley.h"

FASTEPD epaper;

extern "C" void app_main(void)
{
    printf("Starting FastEPD Compressed Images Demo on PaperS3\n");
    if (epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000) != BBEP_SUCCESS) {
        printf("Failed to init panel\n");
        return;
    }
    epaper.clearWhite();
    
    int i = 0;
    float f = 0.5f;
    for (int j = 0; j < 12; j++) {
        epaper.loadG5Image(smiley, i, i, BBEP_WHITE, BBEP_BLACK, f);
        i += (int)(100.0f * f);
        f += 0.5f;
    }
    epaper.setPasses(7);
    epaper.partialUpdate(false);
    while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}
