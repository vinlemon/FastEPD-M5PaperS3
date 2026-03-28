#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../../src/FastEPD.h"
#include "JPEGDEC.h"
#include "../../../../examples/Arduino/show_jpeg/it_cartoon.h"

extern "C" {
int JPEG_openRAM(JPEGIMAGE *pJPEG, uint8_t *pData, int iDataSize, JPEG_DRAW_CALLBACK *pfnDraw);
void JPEG_setPixelType(JPEGIMAGE *pJPEG, int iType);
int JPEG_decode(JPEGIMAGE *pJPEG, int x, int y, int iOptions);
void JPEG_close(JPEGIMAGE *pJPEG);
}

JPEGIMAGE jpg;
FASTEPD epaper;

int JPEGDraw(JPEGDRAW *pDraw)
{
  int x, y, iPitch = epaper.width()/2;
  uint8_t *s, *d, *pBuffer = epaper.currentBuffer();
  for (y=0; y<pDraw->iHeight; y++) {
    d = &pBuffer[((pDraw->y + y)*iPitch) + (pDraw->x/2)];
    s = (uint8_t *)pDraw->pPixels;
    s += (y * pDraw->iWidth);
    for (x=0; x<pDraw->iWidth; x+=2) {
        *d++ = (s[0] & 0xf0) | (s[1] >> 4);
        s += 2;
    } // for x
  } // for y
  return 1;
}

extern "C" void app_main(void)
{
    printf("Initializing M5PaperS3 panel...\n");
    int rc = epaper.initPanel(BB_PANEL_M5PAPERS3);
    if (rc != BBEP_SUCCESS) {
        printf("Failed to init panel! rc = %d\n", rc);
        return;
    }

    epaper.setMode(BB_MODE_4BPP);
    epaper.fillScreen(0xf);

    if (JPEG_openRAM(&jpg, (uint8_t *)it_cartoon, sizeof(it_cartoon), JPEGDraw)) {
        printf("Opened JPEG image successfully. Decoding...\n");
        JPEG_setPixelType(&jpg, EIGHT_BIT_GRAYSCALE);
        JPEG_decode(&jpg, 0, 0, 0);
        JPEG_close(&jpg);
        epaper.fullUpdate();
        printf("Decode and update complete.\n");
    } else {
        printf("Failed to open JPEG image.\n");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50000));
    }
}
