#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

#include <Arduino.h>

// ==================== ESP32-S3 MASTER ====================
#ifdef CONFIG_BRUCE_MASTER

// UART giao tiếp với C5
#define UART_MASTER_TX        17
#define UART_MASTER_RX        18
#define UART_BAUDRATE         115200

// Màn hình ST7789 (SPI)
#define TFT_MOSI              11
#define TFT_SCLK              12
#define TFT_CS                10
#define TFT_DC                14
#define TFT_RST               15
#define TFT_BLK               3

// Thẻ nhớ MicroSD (SPI)
#define SD_CS                 9
#define SD_MOSI               11
#define SD_MISO               13
#define SD_SCLK               12

// Phím 5-way (dùng INPUT_PULLUP)
#define KEY_UP                1
#define KEY_DOWN              2
#define KEY_LEFT              3
#define KEY_RIGHT             38
#define KEY_CENTER            39

// LED báo trạng thái
#define LED_STATUS            48

#endif

// ==================== ESP32-C5 SLAVE ====================
#ifdef CONFIG_BRUCE_SLAVE

// UART giao tiếp với S3
#define UART_SLAVE_RX         17      // Nối với TX S3 (IO17)
#define UART_SLAVE_TX         18      // Nối với RX S3 (IO18)

// NRF24L01+PA+LNA (SPI - dùng bus riêng)
#define NRF_CSN               14
#define NRF_CE                13
#define NRF_MOSI              25
#define NRF_MISO              26
#define NRF_SCK               27

// CC1101 (SPI - chung bus với NRF24)
#define CC1101_CS             15
#define CC1101_GDO0           28
#define CC1101_MOSI           25
#define CC1101_MISO           26
#define CC1101_SCK            27

// PN532 NFC (I2C)
#define PN532_SDA             4
#define PN532_SCL             5

// Hồng ngoại IR
#define IR_RX                 23      // Mắt thu
#define IR_TX                 24      // Mắt phát (cấp 5V)

// LED báo
#define LED_STATUS            2

// Tần số mặc định
#define CC1101_DEFAULT_FREQ   433.92
#define NRF24_DEFAULT_CHANNEL 100

#endif

#endif
