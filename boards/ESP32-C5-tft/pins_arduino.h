/* TFT definitions */
#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT (uint8_t)1
#define USER_SETUP_LOADED 1

/* ---------------------   */
// Setup for ST7789 170x320
#define ST7789_DRIVER 1
#define TFT_RGB_ORDER TFT_BGR
#define TFT_WIDTH 170
#define TFT_HEIGHT 320
#define TFT_INVERSION_ON
/* ---------------------   */

// Setup for ST7789 240x320 (COMMENT TOÀN BỘ)
// #define ST7789_DRIVER 1
// #define TFT_WIDTH 240
// #define TFT_HEIGHT 320
// #define TFT_RGB_ORDER TFT_BGR

/* ---------------------   */
// Setup for ILI9341 320x240 (COMMENT TOÀN BỘ)
// #define ILI9341_DRIVER 1
// #define TFT_HEIGHT 320
// #define TFT_WIDTH 240

/* ---------------------   */
// Common TFT definitions
#define TFT_BACKLIGHT_ON 1
#define TFT_BL 25
#define TFT_RST -1
#define TFT_DC 24
#define TFT_MISO 2
#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_CS 23
#define TOUCH_CS 1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000
