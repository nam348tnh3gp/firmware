#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

#include <Arduino.h>

// ==================== NGUỒN & HỆ THỐNG ====================
#define PIN_POWER_SWITCH      21    // Công tắc nguồn (đọc trạng thái)
#define PIN_BATTERY_ADC       4     // Đo pin (nếu có)

// ==================== MÀN HÌNH ST7789 (SPI) ====================
#define TFT_MOSI              11
#define TFT_SCLK              12
#define TFT_CS                10
#define TFT_DC                14
#define TFT_RST               15
#define TFT_BLK               3     // Đèn nền (3.3V)

// ==================== THẺ NHỚ MICROSD (SPI) ====================
#define SD_CS                 9
#define SD_MOSI               11    // Chung SPI với màn hình
#define SD_MISO               13
#define SD_SCLK               12

// ==================== NRF24L01 (SPI) ====================
#define NRF_CSN               8
#define NRF_CE                7
#define NRF_MOSI              11    // Chung SPI bus
#define NRF_MISO              13
#define NRF_SCK               12

// ==================== PN532 NFC (I2C) ====================
#define PN532_SDA             4
#define PN532_SCL             5
#define PN532_IRQ             6      // Tùy chọn
#define PN532_RESET           16     // Tùy chọn

// ==================== PHÍM ĐIỀU HƯỚNG 5-WAY ====================
#define KEY_UP                1
#define KEY_DOWN              2
#define KEY_LEFT              3
#define KEY_RIGHT             38
#define KEY_CENTER            39

// ==================== HỒNG NGOẠI (IR) ====================
#define IR_RX                 21     // Mắt thu
#define IR_TX                 47     // Mắt phát

// ==================== UART GIAO TIẾP S3 <-> C5 ====================
#define UART_MASTER_TX        17     // S3 TX -> C5 RX
#define UART_MASTER_RX        18     // S3 RX <- C5 TX
#define UART_BAUDRATE         115200

// ==================== ESP32-C5 SLAVE (RIÊNG CHO C5) ====================
#ifdef CONFIG_BRUCE_SLAVE
#define SLAVE_LED             2      // LED báo hiệu
#define SLAVE_BUTTON          0      // Nút boot (nếu có)
#endif

#endif
