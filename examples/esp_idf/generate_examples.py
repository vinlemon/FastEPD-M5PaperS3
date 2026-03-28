import os
import shutil

BASE_DIR = "/Users/vins/Desktop/miniflux-paperS3/FastEPD/examples/esp_idf"
ARDUINO_DIR = "/Users/vins/Desktop/miniflux-paperS3/FastEPD/examples/Arduino"
TEMPLATE_DIR = os.path.join(BASE_DIR, "papers3_demo")

EXAMPLES = {
    "sprite_demo": {
        "assets": ["Roboto_Regular_20.h", "courierprime_14.h"],
        "arduino_folder": "sprite_demo",
        "code": """#include <stdio.h>
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
    printf("Starting FastEPD Sprite Demo on PaperS3\\n");
    sprite1.initSprite(128, 64);
    sprite2.initSprite(128, 64);
    if (epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000) != BBEP_SUCCESS) {
        printf("Failed to init panel\\n");
        return;
    }
    epaper.fillScreen(BBEP_WHITE);
    epaper.fullUpdate();
    sprite1.fillScreen(BBEP_WHITE);
    sprite1.setFont(courierprime_14);
    sprite1.setCursor(0, 24);
    sprite1.print("Sprite1");
    sprite2.fillScreen(BBEP_WHITE);
    sprite2.setFont(Roboto_Regular_20);
    sprite2.setCursor(0, 32);
    sprite2.print("Sprite2");
    for (int i=0; i<epaper.width(); i += 160) {
        epaper.drawSprite(&sprite1, i, 0);
        epaper.drawSprite(&sprite2, i, 256);
    }
    epaper.fullUpdate();
    while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}
"""
    },
    "grayscale_test": {
        "assets": [],
        "arduino_folder": "grayscale_test",
        "code": """#include <stdio.h>
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
    printf("Starting FastEPD Grayscale Demo on PaperS3\\n");
    if (epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000) != BBEP_SUCCESS) {
        printf("Failed to init panel\\n");
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
        epaper.setCursor(i*50+12, 252);
        epaper.print(i, DEC);
    }
    epaper.fullUpdate();
    while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}
"""
    },
    "antialias_font": {
        "assets": ["Roboto_Black_40.h", "Roboto_Black_80.h"],
        "arduino_folder": "antialias_font",
        "code": """#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../../src/FastEPD.h"
#include "Roboto_Black_40.h"
#include "Roboto_Black_80.h"

FASTEPD epaper;

extern "C" void app_main(void)
{
    printf("Starting FastEPD Anti-aliased Font Demo on PaperS3\\n");
    if (epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000) != BBEP_SUCCESS) {
        printf("Failed to init panel\\n");
        return;
    }
    epaper.setMode(BB_MODE_4BPP);
    epaper.clearWhite();
    epaper.setFont(Roboto_Black_40);
    epaper.setCursor(0,60);
    epaper.print("Roboto Black 40pt");
    epaper.setFont(Roboto_Black_80, true);
    epaper.setCursor(0, 240);
    epaper.print("Roboto Black 80pt aa");
    epaper.fullUpdate(true, false);
    while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}
"""
    },
    "compressed_images": {
        "assets": ["smiley.h"],
        "arduino_folder": "compressed_images",
        "code": """#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../../../../src/FastEPD.h"
#include "smiley.h"

FASTEPD epaper;

extern "C" void app_main(void)
{
    printf("Starting FastEPD Compressed Images Demo on PaperS3\\n");
    if (epaper.initPanel(BB_PANEL_M5PAPERS3, 20000000) != BBEP_SUCCESS) {
        printf("Failed to init panel\\n");
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
"""
    }
}

for name, config in EXAMPLES.items():
    new_project = f"papers3_{name}"
    new_dir = os.path.join(BASE_DIR, new_project)
    
    # Copy from template
    if os.path.exists(new_dir):
        shutil.rmtree(new_dir)
    shutil.copytree(TEMPLATE_DIR, new_dir)
    
    # Remove old main c
    try:
        os.remove(os.path.join(new_dir, "main", "papers3_demo.c"))
    except:
        pass
    if os.path.exists(os.path.join(new_dir, "build")):
        shutil.rmtree(os.path.join(new_dir, "build"))
        
    # Write new main cpp
    cpp_path = os.path.join(new_dir, "main", f"{new_project}.cpp")
    with open(cpp_path, 'w') as f:
        f.write(config["code"])
        
    # Copy assets
    for asset in config["assets"]:
        src_asset = os.path.join(ARDUINO_DIR, config["arduino_folder"], asset)
        dst_asset = os.path.join(new_dir, "main", asset)
        if os.path.exists(src_asset):
            shutil.copy(src_asset, dst_asset)
            
    # Modify CMakeLists
    cmakelists_main = os.path.join(new_dir, "main", "CMakeLists.txt")
    with open(cmakelists_main, 'r') as f:
        content = f.read()
    with open(cmakelists_main, 'w') as f:
        f.write(content.replace("papers3_demo.c", f"{new_project}.cpp"))
        
    cmakelists_root = os.path.join(new_dir, "CMakeLists.txt")
    with open(cmakelists_root, 'r') as f:
        content = f.read()
    with open(cmakelists_root, 'w') as f:
        f.write(content.replace("papers3_demo", new_project))

print("All projects generated successfully.")
