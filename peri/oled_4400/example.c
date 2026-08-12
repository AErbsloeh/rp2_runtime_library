#include <stdio.h>
#include <string.h>
#include "hal/led/led.h"
#include "hal/i2c/i2c.h"
#include "peri/oled_4400/ssd1306.h"


/* SSD1306_WIDTH/HEIGHT are 1 bit per pixel, 8 rows packed per byte (page) */
#define OLED_BUF_LEN (SSD1306_WIDTH * SSD1306_HEIGHT / 8)
#define OLED_NUM_PAGES (SSD1306_HEIGHT / 8)

#define BAR_WIDTH 4     /* how many columns wide the bar is */
#define STEP_DELAY_MS 30


int main(){   
    // Init Phase
    static i2c_rp2_t i2c_mod = {
        .pin_sda = 0,
        .pin_scl = 1,
        .i2c_mod = i2c0,
        .fi2c_khz = 100,
        .avai_devices = 0,
        .init_done = false
    };
    scan_i2c_bus_for_device(&i2c_mod);

    init_default_led();
    stdio_init_all();
    sleep_ms(3000);

    // Pre-Phase
    static ssd1306_t oled = {
        .i2c_mod = &i2c_mod,
        .render_area = NULL,
        .init_done = false
    };
    if(ssd1306_init(&oled)) {
        printf("SSD1306 Initialized Successfully!\n");
    } else {
        printf("Failed to Initialize SSD1306. Check I2C connection.\n");
    }
    set_state_default_led(true);

    render_area_t full_area = {
        .start_col = 0,
        .end_col = SSD1306_WIDTH - 1,
        .start_page = 0,
        .end_page = OLED_NUM_PAGES - 1
    };

    uint8_t buf[OLED_BUF_LEN];
    int x = 0;

    while (true) {
        memset(buf, 0, sizeof(buf));
        for (int w = 0; w < BAR_WIDTH; w++) {
            ssd1306_draw_line(buf, x + w, 0, x + w, SSD1306_HEIGHT - 1, true);
        }
        ssd1306_render(&oled, buf, &full_area);
        x++;
        if (x + BAR_WIDTH > SSD1306_WIDTH) {
            x = 0; /* wrap around, bar starts again from the left */
        }
        toggle_state_default_led();
        sleep_ms(STEP_DELAY_MS);
    }
}