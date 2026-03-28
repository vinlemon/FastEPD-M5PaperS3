//
// FastEPD gray matrix editor for M5PaperS3
// ESP-IDF port of the Arduino gray_matrix_editor example.
//
// Provides an interactive serial (UART) interface to edit and test
// the 16-level grayscale waveform table in real-time.
//
// Written by Larry Bank (bitbank@pobox.com) - original Arduino version
// Ported to ESP-IDF for M5PaperS3
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "../../../../src/FastEPD.h"

static const char *TAG = "gray_matrix";

FASTEPD epaper;
uint8_t ucTemp[64];
uint8_t u8_last[16 * 48]; // to allow UNDO of a single change

// Starting gray matrix for M5PaperS3 - edit as needed
uint8_t u8_graytable[] = {
/* 0 */  2, 2, 1, 1, 1, 1, 1, 1, 0,
/* 1 */  1, 1, 1, 2, 2, 2, 1, 1, 0,
/* 2 */  2, 1, 1, 1, 1, 1, 1, 2, 0,
/* 3 */  2, 2, 2, 1, 1, 1, 1, 2, 0,
/* 4 */  2, 2, 2, 2, 1, 1, 1, 2, 0,
/* 5 */  2, 1, 1, 1, 1, 2, 1, 2, 0,
/* 6 */  2, 2, 1, 1, 1, 2, 1, 2, 0,
/* 7 */  2, 2, 2, 1, 1, 2, 1, 2, 0,
/* 8 */  1, 1, 1, 1, 2, 2, 1, 2, 0,
/* 9 */  1, 1, 1, 1, 1, 1, 2, 2, 0,
/* 10 */ 2, 2, 1, 1, 1, 1, 2, 2, 0,
/* 11 */ 2, 2, 2, 1, 1, 1, 2, 2, 0,
/* 12 */ 1, 1, 1, 1, 1, 2, 2, 2, 0,
/* 13 */ 2, 1, 1, 1, 1, 2, 2, 2, 0,
/* 14 */ 2, 2, 2, 2, 2, 1, 2, 2, 0,
/* 15 */ 2, 2, 2, 2, 2, 2, 2, 2, 0
};

static int passes; // Number of passes (columns) derived from table size

const char *szCMDs[] = {"HELP", "LIST", "SHOW", "COPY", "SWAP", "EDIT", "UNDO", "CODE", NULL};
enum {
    CMD_HELP = 0,
    CMD_LIST,
    CMD_SHOW,
    CMD_COPY,
    CMD_SWAP,
    CMD_EDIT,
    CMD_UNDO,
    CMD_CODE,
    CMD_COUNT
};

// UART config
#define UART_NUM     UART_NUM_0
#define BUF_SIZE     (1024)
#define MAX_WAIT_MS  120000  // 120 seconds timeout

// Read a line from UART. Returns true on timeout, false if complete line received.
bool getCmd(char *szString)
{
    char *d = szString;
    d[0] = 0;
    int waited = 0;

    while (waited < MAX_WAIT_MS) {
        uint8_t c;
        int len = uart_read_bytes(UART_NUM, &c, 1, pdMS_TO_TICKS(50));
        if (len > 0) {
            if (c == 0x0a || c == 0x0d) { // LF or CR = end of line
                *d = 0;
                return false;
            }
            *d++ = (char)c;
        }
        waited += 50;
    }
    return true; // timeout
}

int delimitedValue(char *szText, int *pValue, int iLen)
{
    int i = 0, j, val;
    while (i < iLen && (szText[i] == ' ' || szText[i] == ',')) i++;
    j = i;
    while (j < iLen && szText[j] != ' ' && szText[j] != ',') j++;
    if (szText[i] == '0' && (szText[i+1] == 'x' || szText[i+1] == 'X')) {
        sscanf(&szText[i+2], "%x", &val);
    } else {
        val = atoi(&szText[i]);
    }
    *pValue = val;
    return j;
}

int tokenizeCMD(char *szText, int iLen)
{
    int i = 0;
    char szTemp[16];
    memcpy(szTemp, szText, iLen);
    szTemp[iLen] = 0;
    while (szCMDs[i]) {
        if (strcasecmp(szCMDs[i], szTemp) == 0) return i;
        i++;
    }
    return -1;
}

int parseCmd(char *szText, int *pData)
{
    int iLen, i, iCount, iValue;
    int iStart = 0;

    iLen = strlen(szText);
    if (iLen == 0) return 0;

    i = delimitedValue(szText, &iValue, iLen - iStart);
    pData[0] = tokenizeCMD(szText, i);
    if (pData[0] < 0 || pData[0] >= CMD_COUNT) {
        printf("Invalid command: %.*s\n", i, szText);
        printf("Type HELP for a list of commands\n");
        return 0;
    }
    iCount = 1;
    iStart += i;
    while (iStart < iLen) {
        i = delimitedValue(&szText[iStart], &iValue, iLen - iStart);
        pData[iCount++] = iValue;
        iStart += i;
    }
    return iCount;
}

void showHelp()
{
    printf("Interactive FastEPD gray matrix editor for M5PaperS3\n");
    printf("(case insensitive, decimal numbers assumed, space or comma delimited)\n");
    printf("HELP - This command list\n");
    printf("LIST n - show a row of gray matrix values\n");
    printf("SHOW - Display the gray matrix on the EPD panel\n");
    printf("COPY n m - copy row n to row m\n");
    printf("SWAP n m - swap the contents of row n with row m\n");
    printf("EDIT n 0 1 2 2 1 0 0... write new values for row n\n");
    printf("UNDO - undo the last change (1 step only)\n");
    printf("CODE - generate the code for the current gray matrix\n");
}

void ListCode(void)
{
    uint8_t *s = u8_graytable;
    printf("Current Matrix:\n");
    printf("const uint8_t u8_graytable[] = {\n");
    for (int i = 0; i < 16; i++) {
        printf("/* %d */  ", i);
        for (int j = 0; j < passes; j++) {
            if (i == 15 && j == passes-1) {
                printf("%d", *s++);
            } else {
                printf("%d, ", *s++);
            }
        }
        printf("\n");
    }
    printf("};\n");
}

void ShowMatrix(void)
{
    int rc = epaper.setCustomMatrix(u8_graytable, sizeof(u8_graytable));
    if (rc != BBEP_SUCCESS) {
        printf("setCustomMatrix returned %d\n", rc);
        return;
    }
    epaper.fillScreen(0xf);
    for (int i = 0; i < 800; i += 50) {
        epaper.fillRect(i, 0, 50, 250, i/50);
    }
    epaper.drawRect(0, 0, 800, 250, 0);
    epaper.setFont(FONT_12x16);
    epaper.setTextColor(BBEP_BLACK);
    for (int i = 0; i < 16; i++) {
        epaper.setCursor(i*50 + 12, 252);
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", i);
        epaper.drawString(buf, i*50 + 12, 252);
    }
    epaper.fullUpdate(CLEAR_SLOW, false);
    printf("Display updated.\n");
}

void executeCmd(int *pData, int iCount)
{
    uint8_t *s, *d;

    switch (pData[0]) {
        case CMD_HELP:
            showHelp();
            break;
        case CMD_CODE:
            ListCode();
            break;
        case CMD_SHOW:
            ShowMatrix();
            break;
        case CMD_LIST:
            if (pData[1] < 0 || pData[1] > 15) {
                printf("LIST: Row must be in the range 0 to 15\n");
                return;
            }
            printf("EDIT %d ", pData[1]);
            s = &u8_graytable[passes * pData[1]];
            for (int i = 0; i < passes; i++) printf("%d ", s[i]);
            printf("\n");
            break;
        case CMD_COPY:
            if (pData[1] < 0 || pData[1] > 15 || pData[2] < 0 || pData[2] > 15) {
                printf("COPY: rows must be in range 0..15\n");
                return;
            }
            memcpy(u8_last, u8_graytable, 16 * passes);
            s = &u8_graytable[passes * pData[1]];
            d = &u8_graytable[passes * pData[2]];
            memcpy(d, s, passes);
            printf("Copying row %d to %d\n", pData[1], pData[2]);
            break;
        case CMD_SWAP:
            if (pData[1] < 0 || pData[1] > 15 || pData[2] < 0 || pData[2] > 15) {
                printf("SWAP: rows must be in range 0..15\n");
                return;
            }
            memcpy(u8_last, u8_graytable, 16 * passes);
            s = &u8_graytable[passes * pData[1]];
            d = &u8_graytable[passes * pData[2]];
            memcpy(ucTemp, s, passes);
            memcpy(s, d, passes);
            memcpy(d, ucTemp, passes);
            printf("Swapping row %d with %d\n", pData[1], pData[2]);
            break;
        case CMD_EDIT:
            for (int i = 0; i < iCount-2; i++) {
                if (pData[i+2] < 0 || pData[i+2] > 2) {
                    printf("EDIT: values must be 0=neutral, 1=darken, 2=lighten\n");
                    return;
                }
            }
            if (pData[1] < 0 || pData[1] > 15 || iCount != passes+2) {
                printf("EDIT: Row must be 0..15, and provide exactly %d values\n", passes);
                return;
            }
            memcpy(u8_last, u8_graytable, 16 * passes);
            d = &u8_graytable[pData[1] * passes];
            for (int i = 0; i < passes; i++) d[i] = pData[i+2];
            printf("EDIT: Row %d updated\n", pData[1]);
            break;
        case CMD_UNDO:
            memcpy(u8_graytable, u8_last, 16 * passes);
            printf("UNDO - last change undone\n");
            break;
    }
}

extern "C" void app_main(void)
{
    // Initialize UART0 for interactive console
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(2000)); // wait for CDC serial to stabilize

    passes = sizeof(u8_graytable) / 16;

    printf("Initializing M5PaperS3 panel...\n");
    int rc = epaper.initPanel(BB_PANEL_M5PAPERS3);
    if (rc != BBEP_SUCCESS) {
        printf("Failed to init panel! rc = %d\n", rc);
        return;
    }
    epaper.setMode(BB_MODE_4BPP);
    ShowMatrix();

    memcpy(u8_last, u8_graytable, 16 * passes); // baseline for first UNDO

    printf("Ready! Enter a command or HELP\n");

    char szText[256];
    int iData[32], iCount;

    while (1) {
        if (getCmd(szText)) {
            printf("Enter a command or HELP\n");
            continue;
        }
        iCount = parseCmd(szText, iData);
        if (iCount > 0) {
            executeCmd(iData, iCount);
        }
    }
}
