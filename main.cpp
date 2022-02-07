/* mbed Microcontroller Library
 * Copyright (c) 2020 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "lvgl.h"
#include "ili9163c.h"
#include "swo.h"

using namespace sixtron;

/*Change to your screen resolution*/
static const uint16_t screenWidth = 128;
static const uint16_t screenHeight = 160;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

static SPI spi(SPI1_MOSI, SPI1_MISO, SPI1_SCK);
ILI9163C display(&spi, SPI1_CS, DIO18, PWM1_OUT);

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    display.setAddr(area->x1, area->y1, area->x2, area->y2);
    display.write_data_16((uint16_t *)&color_p->full, w * h);

    lv_disp_flush_ready(disp);
}

void draw_cross(lv_obj_t *canvas, uint8_t x, uint8_t y)
{
    uint8_t w = 20;
    uint8_t h = 20;

    const lv_point_t vertical[] = {{x, y - h / 2}, {x, y + h / 2}};
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = LV_COLOR_MAKE(255, 255, 255);
    line.width = 2;

    lv_canvas_draw_line(canvas, vertical, 2, &line);

    const lv_point_t horizontal[] = {{x - w / 2, y}, {x + w / 2, y}};

    lv_canvas_draw_line(canvas, horizontal, 2, &line);
}

int main()
{
    printf("Start App\n");
    display.init();

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(screenWidth, screenHeight)];

    lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas, cbuf, screenWidth, screenHeight, LV_IMG_CF_TRUE_COLOR);

    draw_cross(canvas, 10, 10);

    draw_cross(canvas, screenWidth - 10, 10);

    draw_cross(canvas, 10, screenHeight - 10);

    draw_cross(canvas, screenWidth - 10, screenHeight - 10);

    draw_cross(canvas, screenWidth / 2, screenHeight / 2);

    while (true)
    {
        lv_timer_handler(); /* let the GUI do its work */
        ThisThread::sleep_for(100ms);
    }
    return 0;
}