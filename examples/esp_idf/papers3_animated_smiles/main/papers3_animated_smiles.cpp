#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"

#include "../../../../src/FastEPD.h"
#include "smiley.h"

FASTEPD epaper;

// holds the position and direction of the objects
#define NUM_SMILEYS 8
int xArray[NUM_SMILEYS];
int yArray[NUM_SMILEYS];
int xDir[NUM_SMILEYS];
int yDir[NUM_SMILEYS];

extern "C" void app_main(void)
{
    printf("Starting FastEPD Animated Smiles on PaperS3\n");

    if (epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000) != BBEP_SUCCESS) {
        printf("Failed to init panel\n");
        return;
    }

    epaper.clearWhite(); // start with a white display (and buffer)
    epaper.setPasses(3); // fewer passes = faster updates

    // Create 8 random starting points
    for (int i=0; i<NUM_SMILEYS; i++) {
        xArray[i] = (esp_random() % (epaper.width() - 104)) + 4;
        yArray[i] = (esp_random() % (epaper.height() - 104)) + 4;
        xDir[i] = (esp_random() % 2) ? -4 : 4;
        yDir[i] = (esp_random() % 2) ? -4 : 4;
    }

    // Infinite loop since we are demonstrating dynamism!
    while (1) {
        for (int i=0; i<NUM_SMILEYS; i++) { // draw current posiitions
            epaper.loadG5Image(smiley, xArray[i], yArray[i], BBEP_WHITE, BBEP_BLACK, 1.0f);
            // update the positions
            xArray[i] += xDir[i];
            if (xArray[i] < 4 || xArray[i] > epaper.width() - 104) xDir[i] = -xDir[i]; // bounce off X
            yArray[i] += yDir[i];
            if (yArray[i] < 4 || yArray[i] > epaper.height() - 104) yDir[i] = -yDir[i]; // bounce off Y
        }
        // Show on the display
        epaper.partialUpdate(true);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
