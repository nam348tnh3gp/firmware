#ifndef BRUCE_CONFIG_H
#define BRUCE_CONFIG_H

// ==================== BẬT/TẮT TÍNH NĂNG ====================
#define ENABLE_DISPLAY         1
#define ENABLE_SDCARD          1
#define ENABLE_KEYPAD          1

#define ENABLE_NRF24           1
#define ENABLE_CC1101          1
#define ENABLE_PN532           1
#define ENABLE_IR              1

// ==================== CẤU HÌNH DEBUG ====================
#define DEBUG_SERIAL           1
#define DEBUG_LEVEL            3   // 0:off, 1:error, 2:warn, 3:info, 4:debug

#if DEBUG_SERIAL
#define DEBUG_PRINT(level, ...) if(level <= DEBUG_LEVEL) { Serial.printf(__VA_ARGS__); }
#else
#define DEBUG_PRINT(level, ...)
#endif

#endif
