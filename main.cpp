/* mbed Microcontroller Library
 * Copyright (c) 2020 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "BlockDevice.h"
#include "FATFileSystem.h"
#include "FlashIAPBlockDevice.h"
#include "bma280.h"
#include "bme280.h"
#include "ili9163c.h"
#include "lvgl.h"
#include "mbed.h"
#include "sx8650iwltrt.h"

using namespace sixtron;

/*Change to your screen resolution*/
static const uint16_t screenWidth = 128;
static const uint16_t screenHeight = 160;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

#define FLASHIAP_ADDRESS 0x08080000
#define FLASHIAP_SIZE 0x70000 // 0x61A80    // 460 kB

#define VALUE_ACCURACY_X 7 * 128 / 100
#define VALUE_ACCURACY_Y 7 * 180 / 100

/* Driver Touchscreen */
#define SX8650_SWITCHED_TIME 5ms

// Protoypes
void read_coordinates(uint16_t x, uint16_t y);
void application_setup(void);
void retrieve_calibrate_data(char *bufer);
void save_calibrate_date(char *buffer);
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                   lv_color_t *color_p);
void draw_cross(uint8_t x, uint8_t y);
static void touchpad_get_xy(lv_coord_t *x, lv_coord_t *y);
static bool touchpad_is_pressed(void);
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
void draw_cross(uint8_t x, uint8_t y);
static void sw_event_handler(lv_event_t *e);
void lv_switch_button(void);
static void set_temp(lv_obj_t *label, int32_t temp);
static void set_hum(lv_obj_t *label, int32_t hum);
static void set_pressure(lv_obj_t *label, int32_t pressure);
void lv_label(void);

// RTOS
Thread thread;
Thread thread_data;
static EventQueue event_queue;

// Preipherals
static SX8650IWLTRT sx8650iwltrt(I2C1_SDA, I2C1_SCL, &event_queue,
                                 I2CAddress::Address2);
static SPI spi(SPI1_MOSI, SPI1_MISO, SPI1_SCK);
ILI9163C display(&spi, SPI1_CS, DIO18, PWM1_OUT);
static DigitalOut led1(LED1);
static InterruptIn button(BUTTON1);
static I2C i2c(I2C1_SDA, I2C1_SCL);
static BMA280 bma280(&i2c);
static BME280 bme280(&i2c, BME280::I2CAddress::Address2);

// Create flash IAP block device
BlockDevice *bd = new FlashIAPBlockDevice(FLASHIAP_ADDRESS, FLASHIAP_SIZE);
FATFileSystem fs("fs");

// Variables
bool correct_calibrate(false);
static lv_obj_t *temp_label;
static lv_obj_t *hum_label;
static lv_obj_t *pressure_label;
lv_obj_t *sw;
lv_coord_t lv_x;
lv_coord_t lv_y;
bool touched;

void read_coordinates(uint16_t x, uint16_t y) {
  printf("-----------------\n\n");
  printf("X : %u | Y : %u \n\n", x, y);

  lv_x = lv_coord_t(x);
  lv_y = lv_coord_t(y);

  touched = true;
}

void application_setup(void) {

  // Initialize the flash IAP block device and print the memory layout
  bd->init();
  printf("Flash block device size: %llu\n", bd->size());
  printf("Flash block device read size: %llu\n", bd->get_read_size());
  printf("Flash block device program size: %llu\n", bd->get_program_size());
  printf("Flash block device erase size: %llu\n", bd->get_erase_size());

  printf("Mounting the filesystem... \n");
  fflush(stdout);
  int err = fs.mount(bd);

  printf("%s\n", (err ? "Fail :(" : "OK"));
  if (err) {
    // Reformat if we can't mount the filesystem
    // this should only happen on the first boot
    printf("No filesystem found, formatting... ");
    fflush(stdout);
    err = fs.reformat(bd);
    printf("%s\n", (err ? "Fail :(" : "OK"));
    if (err) {
      error("error: %s (%d)\n", strerror(-err), err);
    }
  }
}

void retrieve_calibrate_data(char *buffer) {
  // Open the file
  printf("Opening \"/fs/calibration.txt\"... ");
  fflush(stdout);
  FILE *f = fopen("/fs/calibration.txt", "r");
  printf("%s\n", (!f ? "Fail :(" : "OK"));

  float ax, bx, x_off, ay, by, y_off;
  fscanf(f, "%f %f %f %f %f %f", &ax, &bx, &x_off, &ay, &by, &y_off);

  sx8650iwltrt.set_calibration(ax, bx, x_off, ay, by, y_off);

  fclose(f);

  // Display the root directory
  printf("Opening the root directory... ");
  fflush(stdout);
  DIR *d = opendir("/fs/");
  printf("%s\n", (!d ? "Fail :(" : "OK"));
  if (!d) {
    error("error: %s (%d)\n", strerror(errno), -errno);
  }

  printf("root directory:\n");
  while (true) {
    struct dirent *e = readdir(d);
    if (!e) {
      break;
    }

    printf("    %s\n", e->d_name);
  }
}

void save_calibrate_date(char *buffer) {
  // Open the file
  printf("Opening \"/fs/calibration.txt\"... ");
  fflush(stdout);
  FILE *f = fopen("/fs/calibration.txt", "r+");
  printf("%s\n", (!f ? "Fail :(" : "OK"));
  if (!f) {
    // Create the file if it doesn't exist
    printf("No file found, creating a new file... ");
    fflush(stdout);
    f = fopen("/fs/calibration.txt", "w+");
    printf("%s\n", (!f ? "Fail :(" : "OK"));
    if (!f) {
      error("error: %s (%d)\n", strerror(errno), -errno);
    }
  }
  sprintf(buffer, "%f %f %f %f %f %f", sx8650iwltrt._coefficient.ax,
          sx8650iwltrt._coefficient.bx, sx8650iwltrt._coefficient.x_off,
          sx8650iwltrt._coefficient.ay, sx8650iwltrt._coefficient.by,
          sx8650iwltrt._coefficient.y_off);

  fwrite(buffer, 1, strlen(buffer), f);

  fclose(f);

  // Display the root directory
  printf("Opening the root directory... ");
  fflush(stdout);
  DIR *d = opendir("/fs/");
  printf("%s\n", (!d ? "Fail :(" : "OK"));
  if (!d) {
    error("error: %s (%d)\n", strerror(errno), -errno);
  }

  printf("root directory:\n");
  while (true) {
    struct dirent *e = readdir(d);
    if (!e) {
      break;
    }

    printf("    %s\n", e->d_name);
  }
}

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                   lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  display.setAddr(area->x1, area->y1, area->x2, area->y2);
  display.write_data_16((uint16_t *)&color_p->full, w * h);

  lv_disp_flush_ready(disp);
}

/*Get the x and y coordinates if the touchpad is pressed*/
static void touchpad_get_xy(lv_coord_t *x, lv_coord_t *y) {
  (*x) = lv_x;
  (*y) = lv_y;
}

static bool touchpad_is_pressed(void) {
  if (touched) {
    touched = false;
    return true;

  } else {
    return false;
  }
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  if (touchpad_is_pressed()) {
    data->state = LV_INDEV_STATE_PRESSED;
    touchpad_get_xy(&data->point.x, &data->point.y);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void draw_cross(uint8_t x, uint8_t y) {
  display.clearScreen(0);

  static uint16_t buf[21 * 21] = {0};

  display.setAddr(x - 10, y - 10, x + 10, y + 10);

  for (int i = 0; i < 21; i++) {
    if (i == 10) {
      for (int j = 0; j < 21; j++) {
        buf[i * 21 + j] = 0xFFFF;
      }
    } else {
      buf[i * 21 + 10] = 0xFFFF;
    }
  }

  display.write_data_16(buf, 21 * 21);
}

/**
 * Switch button
 */

static void sw_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    led1 = !led1;
  }
}

void lv_switch_button(void) {
  sw = lv_switch_create(lv_scr_act());
  lv_obj_set_pos(sw, 34, 120);
  lv_obj_add_event_cb(sw, sw_event_handler, LV_EVENT_ALL, NULL);
}

/**
 * Label
 */

static void set_temp(lv_obj_t *label, int32_t temp) {
  temp = (int32_t)bma280.temperature();
  char temp_value[20];
  sprintf(temp_value, "%" LV_PRId32, temp);
  lv_label_set_text_fmt(label, temp_value);
}

static void set_hum(lv_obj_t *label, int32_t hum) {
  hum = (int32_t)bme280.humidity();
  char hum_value[20];
  sprintf(hum_value, "%" LV_PRId32, hum);
  lv_label_set_text_fmt(label, hum_value);
}

static void set_pressure(lv_obj_t *label, int32_t pressure) {
  pressure = (int32_t)bme280.pressure();
  char press_value[20];
  sprintf(press_value, "%" LV_PRId32, pressure);
  lv_label_set_text_fmt(label, press_value);
}

void lv_label(void) {
  // lv_obj_set_flex_flow(lv_scr_act(), LV_FLEX_FLOW_COLUMN);
  // lv_obj_set_flex_align(lv_scr_act(), LV_FLEX_ALIGN_CENTER,
  // LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *title_temp_label = lv_label_create(lv_scr_act());
  lv_obj_set_pos(title_temp_label, 34, 28);
  lv_label_set_text(title_temp_label, "T:");

  temp_label = lv_label_create(lv_scr_act());
  lv_obj_set_pos(temp_label, 54, 28);
  lv_label_set_text(temp_label, "Temp");

  lv_obj_t *temp_label_unity = lv_label_create(lv_scr_act());
  lv_obj_set_pos(temp_label_unity, 74, 28);
  lv_label_set_text(temp_label_unity, "°C");

  lv_obj_t *title_hum_label = lv_label_create(lv_scr_act());
  lv_obj_set_pos(title_hum_label, 34, 56);
  lv_label_set_text(title_hum_label, "H:");

  hum_label = lv_label_create(lv_scr_act());
  lv_obj_set_pos(hum_label, 54, 56);
  lv_label_set_text(hum_label, "Hum");

  lv_obj_t *hum_label_unity = lv_label_create(lv_scr_act());
  lv_obj_set_pos(hum_label_unity, 74, 56);
  lv_label_set_text(hum_label_unity, "%");

  lv_obj_t *title_pressure_label = lv_label_create(lv_scr_act());
  lv_obj_set_pos(title_pressure_label, 34, 84);
  lv_label_set_text(title_pressure_label, "P:");

  pressure_label = lv_label_create(lv_scr_act());
  lv_obj_set_pos(pressure_label, 54, 84);
  lv_label_set_text(pressure_label, "Press");

  lv_obj_t *press_label_unity = lv_label_create(lv_scr_act());
  lv_obj_set_pos(press_label_unity, 74, 84);
  lv_label_set_text(press_label_unity, "hPa");

  lv_anim_t a_temp;
  lv_anim_init(&a_temp);
  lv_anim_set_exec_cb(&a_temp, (lv_anim_exec_xcb_t)set_temp);
  lv_anim_set_time(&a_temp, 500);
  lv_anim_set_var(&a_temp, temp_label);
  lv_anim_set_repeat_count(&a_temp, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a_temp);

  lv_anim_t a_hum;
  lv_anim_init(&a_hum);
  lv_anim_set_exec_cb(&a_hum, (lv_anim_exec_xcb_t)set_hum);
  lv_anim_set_time(&a_hum, 500);
  lv_anim_set_var(&a_hum, hum_label);
  lv_anim_set_repeat_count(&a_hum, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a_hum);

  lv_anim_t a_press;
  lv_anim_init(&a_press);
  lv_anim_set_exec_cb(&a_press, (lv_anim_exec_xcb_t)set_pressure);
  lv_anim_set_time(&a_press, 500);
  lv_anim_set_var(&a_press, pressure_label);
  lv_anim_set_repeat_count(&a_press, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a_press);
}

int main() {

  printf("Start App\n");
  spi.frequency(10000000);
  display.init();
  display.clearScreen(0);
  thread.start(callback(&event_queue, &EventQueue::dispatch_forever));
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);

  /*Initialize touchscreen component*/
  ThisThread::sleep_for(SX8650_SWITCHED_TIME);
  sx8650iwltrt.soft_reset();
  sx8650iwltrt.set_mode(Mode::PenTrg);
  sx8650iwltrt.set_rate(Rate::RATE_200_cps);
  sx8650iwltrt.set_powdly(Time::DLY_2_2US);
  sx8650iwltrt.enable_coordinates_measurement();

  // application setup
  application_setup();
  char *buffer = (char *)malloc(bd->get_erase_size());

  // Open the file
  printf("Opening \"/fs/calibration.txt\"... ");
  fflush(stdout);
  FILE *f = fopen("/fs/calibration.txt", "r+");
  printf("%s\n", (!f ? "Fail :(" : "OK"));
  if (!f) {
    // Create the file if it doesn't exist
    printf("No file found, creating a new file... ");
    fflush(stdout);
    f = fopen("/fs/calibration.txt", "w+");
    printf("%s\n", (!f ? "Fail :(" : "OK"));
    if (!f) {
      error("error: %s (%d)\n", strerror(errno), -errno);
    }
  }

  float ax;
  fscanf(f, "%f", &ax);

  fclose(f);

  if (ax != sx8650iwltrt._coefficient.ax && ax != 0) {
    retrieve_calibrate_data(buffer);
    correct_calibrate = true;
  }

  while (!correct_calibrate) {
    draw_cross(64, 80);
    sx8650iwltrt.attach_coordinates_measurement(read_coordinates);
    ThisThread::sleep_for(2000ms);
    if ((64 + VALUE_ACCURACY_X > sx8650iwltrt._coordinates.x &&
         sx8650iwltrt._coordinates.x > 64 - VALUE_ACCURACY_X) &&
        (80 + VALUE_ACCURACY_Y > sx8650iwltrt._coordinates.y &&
         sx8650iwltrt._coordinates.y > 80 - VALUE_ACCURACY_Y)) {
      printf("Calibrate done correctly ! \n\n");
      correct_calibrate = true;
      save_calibrate_date(buffer);
    } else {
      printf("Calibrate again ! \n");
      sx8650iwltrt.calibrate(draw_cross);
    }
    display.clearScreen(0);
  }

  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv; /*Descriptor of a input device driver*/
  lv_indev_drv_init(&indev_drv);   /*Basic initialization*/
  indev_drv.type = LV_INDEV_TYPE_POINTER; /*Touch pad is a pointer-like device*/
  indev_drv.read_cb = my_touchpad_read;   /*Set your driver function*/
  lv_indev_drv_register(&indev_drv);      /*Finally register the driver*/

  lv_label();
  lv_switch_button();
  sx8650iwltrt.attach_coordinates_measurement(read_coordinates);

  while (true) {
    lv_task_handler();
    lv_tick_inc(10);
    ThisThread::sleep_for(10ms);
  }
  return 0;
}