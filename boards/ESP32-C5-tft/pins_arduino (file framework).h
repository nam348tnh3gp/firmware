/*
 * ESP32-C5 default pins - fallback for boards without variant-specific definitions
 * This file provides SDA, SCL, SCK, MISO, MOSI, SS for ESP32-C5 boards
 * in the pioarduino framework where variant include paths may not be resolved correctly.
 */

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include "soc/soc_caps.h"

#define PIN_RGB_LED 27
static const uint8_t LED_BUILTIN = SOC_GPIO_PIN_COUNT + PIN_RGB_LED;
#define BUILTIN_LED LED_BUILTIN
#define RGB_BUILTIN    LED_BUILTIN
#define RGB_BRIGHTNESS 64

static const uint8_t TX = 11;
static const uint8_t RX = 12;

static const uint8_t SDA = 0;
static const uint8_t SCL = 1;

static const uint8_t SS = 6;
static const uint8_t MOSI = 8;
static const uint8_t MISO = 9;
static const uint8_t SCK = 10;

// SPI flash/sd card pin mapping (used by RFID2, utils.cpp, etc.)
#define SPI_SCK_PIN   6
#define SPI_MOSI_PIN  7
#define SPI_MISO_PIN  2
#define SPI_SS_PIN    23

// IR TX/RX LEDs
#define TXLED 24
#define RXLED 23

// CH9329 UART keyboard (BadUSB hardware)
#define BAD_RX 18
#define BAD_TX 17

static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;

static const uint8_t LP_SDA = 2;
static const uint8_t LP_SCL = 3;
#define WIRE1_PIN_DEFINED
#define SDA1 LP_SDA
#define SCL1 LP_SCL

static const uint8_t LP_RX = 4;
static const uint8_t LP_TX = 5;

#endif
