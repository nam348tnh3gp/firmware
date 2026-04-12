/*
 * ESP32-C5 default pins - fallback for boards without variant-specific definitions
 * This file provides SDA, SCL, SCK, MISO, MOSI, SS for ESP32-C5 boards
 * in the pioarduino framework where variant include paths may not be resolved correctly.
 */

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include "soc/soc_caps.h"

// LED
#define PIN_RGB_LED 27
static const uint8_t LED_BUILTIN = SOC_GPIO_PIN_COUNT + PIN_RGB_LED;
#define BUILTIN_LED LED_BUILTIN
#define RGB_BUILTIN    LED_BUILTIN
#define RGB_BRIGHTNESS 64

// UART0 (USB-Serial/JTAG)
static const uint8_t TX = 11;
static const uint8_t RX = 12;

// I2C (FM Radio, PN532, CH9329)
static const uint8_t SDA = 4;
static const uint8_t SCL = 5;

// SPI (Default)
static const uint8_t SS    = 23;  // TFT_CS
static const uint8_t MOSI  = 7;
static const uint8_t MISO  = 2;
static const uint8_t SCK   = 6;

// SPI pin mapping cho SD card và TFT (dùng chung SPI)
#define SPI_SCK_PIN   6
#define SPI_MOSI_PIN  7
#define SPI_MISO_PIN  2
#define SPI_SS_PIN    23

// --- TFT Display Pins (Lấy từ connections.md) ---
#define TFT_CS    23
#define TFT_DC    24
#define TFT_RST   -1  // Không có chân RST riêng, dùng chung với chân RST của ESP32-C5
#define TFT_BL    25
#define TFT_MOSI  7
#define TFT_SCLK  6

// --- Touch Screen Pins (XPT2046) ---
#define TOUCH_CS  1
// Không có IRQ riêng

// --- NRF24/CC1101/W5500 Pins (dùng chung SPI, chọn bằng switch) ---
#define NRF24_SS_PIN    9
#define CC1101_SS_PIN   9
#define W5500_SS_PIN    9
#define GDO0_CE_PIN     8

// --- SD Card Pins ---
#define SDCARD_CS       10

// --- Buttons Pins ---
#define HAS_3_BUTTONS
#define UP_BTN   0   // Prev
#define SEL_BTN  28  // Sel
#define DW_BTN   1   // Next

// --- GPS Pins (Serial2) ---
#define GPS_RX  4
#define GPS_TX  5

// --- IR Pins ---
#define IR_RX   26
#define IR_TX   3

// --- BadUSB (CH9329 UART) ---
#define BAD_RX  18
#define BAD_TX  17

// --- Flipper Zero Serial ---
#define FLIPPER_TX  11
#define FLIPPER_RX  12

// --- Độ sáng màn hình (Cho hàm analogWrite) ---
#ifndef MINBRIGHT
  #define MINBRIGHT 25  // Giá trị tối thiểu cho độ sáng (0-255)
#endif

// --- Các định nghĩa tương thích khác ---
static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;

#endif
