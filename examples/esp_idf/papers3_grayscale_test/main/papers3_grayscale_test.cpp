#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../../src/FastEPD.h"

FASTEPD epaper;

const uint8_t u8_103Grays[] = {
/* 0 */	  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,
/* 1 */		0,	0,	0,	0,	0,	0,	0,	0,	0,	1,	2,	1,	1,	1,	0,	0,	0,	0,
/* 2 */		0,	0,	0,	0,	0,	1,	1,	1,	2,	1,	1,	1,	2,	1,	0,	0,	0,	0,
/* 3 */		0,	0,	0,	0,	0,	0,	0,	0,	1,	1,	1,	2,	1,	0,	0,	0,	0,	0,
/* 4 */	  0,	0,	0,	0,	0,	0,	0,	1,	1,	1,	2,	1,	0,	0,	0,	0,	0,	0,
/* 5 */		0,	0,	0,	0,	0,	1,	0,	0,	1,	2,	0,	1,	0,	0,	0,	0,	0,	0,
/* 6 */		0,	0,	0,	0,	0,	1,  2,	0,	1,	2,	0,	1,	0,	0,	0,	0,	0,	0,
/* 7 */ 	0,	0,	0,	0,	0,	1,	1,	1,	1,	1,	1,	1,	1,	1,	2,	0,	0,	0,
/* 8 */ 	0,	0,	0,	0,	0,	0,	0,	1,	2,	2,	1,	2,	1,	0,	0,	0,	0,	0,
/* 9 */ 	0,	0,	0,	0,	0,	0,	0,	0,	1,	1,	1,	1,	1,	1,	2,	0,	0,	0,
/* 10 */	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	1,	1,	1,	1,	2,	0,	0,	0,
/* 11 */	0,	0,	0,	0,	0,	0,	1,	1,	1,	1,	1,	1,	1,	2,	1,	2,	0,	0,
/* 12 */	0,	0,	0,	0,	0,	1,	1,	1,	2,	1,	2,	0,	0,	0,	0,	0,	0,	0,
/* 13 */	0,	0,	0,	0,	0,	1,	1,	2,	2,	2,	2,	1,	2,	0,	0,	0,	0,	0,
/* 14 */	1,	1,	1,	1,	1,	1,	2,	2,	1,	2,	2,	0,	0,	0,	0,	0,	0,	0,
/* 15 */	0,	1,	1,	1,	1,	1,	1,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2
};

extern "C" void app_main(void)
{
    printf("Starting FastEPD Grayscale Demo on PaperS3\n");
    if (epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000) != BBEP_SUCCESS) {
        printf("Failed to init panel\n");
        return;
    }
    epaper.setCustomMatrix(u8_103Grays, sizeof(u8_103Grays));
    epaper.setMode(BB_MODE_4BPP);
    epaper.fillScreen(0xf);
    for (int i=0; i<800; i+=50) {
        epaper.fillRect(i, 0, 50, 250, i/50);
    }
    epaper.drawRect(0, 0, 800, 250, 0);
    epaper.setFont(FONT_12x16);
    epaper.setTextColor(BBEP_BLACK, BBEP_WHITE);
    for (int i=0; i<16; i++) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", i);
        epaper.drawString(buf, i*50+12, 252);
    }
    epaper.fullUpdate();
    while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}
