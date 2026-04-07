#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "pins_config.h"
#include "drivers/display_st7789.h"
#include "drivers/sdcard_manager.h"
#include "drivers/nrf24_bruce.h"
#include "drivers/nfc_pn532.h"
#include "drivers/ir_controller.h"
#include "modules/keypad_handler.h"

// ==================== BIẾN TOÀN CỤC ====================
HardwareSerial MasterSerial(1);  // UART1 cho giao tiếp với C5
DisplayST7789 display;
SDCardManager sdCard;
NRF24Bruce nrf24;
NFCPN532 nfc;
IRController ir;
KeypadHandler keypad;

// Timer cho các tác vụ
hw_timer_t* timer = NULL;
volatile bool flag_scan_nrf = false;
volatile bool flag_read_nfc = false;

// ==================== INTERRUPT TIMER ====================
void IRAM_ATTR onTimer() {
    static uint8_t counter = 0;
    counter++;
    if (counter % 10 == 0) flag_scan_nrf = true;   // Mỗi 1 giây (100ms * 10)
    if (counter % 50 == 0) flag_read_nfc = true;   // Mỗi 5 giây
}

// ==================== KHỞI TẠO ====================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== BRUCE FIRMWARE - ESP32-S3 MASTER ===\n");
    
    // 1. Khởi tạo giao tiếp với C5 Slave
    MasterSerial.begin(UART_BAUDRATE, SERIAL_8N1, UART_MASTER_RX, UART_MASTER_TX);
    Serial.println("[OK] UART với ESP32-C5 đã sẵn sàng");
    
    // 2. Màn hình ST7789
    if (display.init(TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST)) {
        display.showSplash();
        Serial.println("[OK] Màn hình ST7789 khởi tạo thành công");
    } else {
        Serial.println("[ERROR] Màn hình ST7789 khởi tạo thất bại");
    }
    
    // 3. Thẻ nhớ SD
    if (sdCard.init(SD_CS, SD_MOSI, SD_MISO, SD_SCLK)) {
        Serial.println("[OK] Thẻ nhớ SD đã sẵn sàng");
        sdCard.listFiles("/");
    } else {
        Serial.println("[WARN] Không tìm thấy thẻ nhớ SD");
    }
    
    // 4. NRF24L01
    if (nrf24.init(NRF_CSN, NRF_CE, NRF_MOSI, NRF_MISO, NRF_SCK)) {
        Serial.println("[OK] Module NRF24L01 đã sẵn sàng");
        nrf24.setChannel(100);
        nrf24.setDataRate(RF24_250KBPS);
    } else {
        Serial.println("[ERROR] NRF24L01 không phản hồi");
    }
    
    // 5. PN532 NFC
    Wire.begin(PN532_SDA, PN532_SCL);
    if (nfc.init(&Wire, PN532_IRQ, PN532_RESET)) {
        Serial.println("[OK] Module NFC PN532 đã sẵn sàng");
        nfc.setPassiveActivationRetries(0xFF);
    } else {
        Serial.println("[ERROR] PN532 không phản hồi");
    }
    
    // 6. IR (Hồng ngoại)
    ir.init(IR_TX, IR_RX);
    Serial.println("[OK] Hồng ngoại IR đã sẵn sàng");
    
    // 7. Phím điều hướng
    keypad.init(KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_CENTER);
    Serial.println("[OK] Phím điều hướng 5-way đã sẵn sàng");
    
    // 8. Timer ngắt 100ms
    timer = timerBegin(0, 80, true);  // Timer 0, prescaler 80 (1us tick)
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 100000, true);  // 100ms
    timerAlarmEnable(timer);
    
    // Gửi tín hiệu sẵn sàng cho C5
    MasterSerial.println("MASTER_READY");
    
    Serial.println("\n=== HỆ THỐNG ĐÃ SẴN SÀNG ===\n");
    display.showReady();
}

// ==================== VÒNG LẶP CHÍNH ====================
void loop() {
    // 1. Xử lý phím bấm
    KeyEvent event = keypad.getEvent();
    if (event.pressed) {
        handleKeyPress(event.key);
    }
    
    // 2. Quét NRF24 (định kỳ)
    if (flag_scan_nrf) {
        flag_scan_nrf = false;
        scanNRF24Devices();
    }
    
    // 3. Đọc NFC (định kỳ)
    if (flag_read_nfc) {
        flag_read_nfc = false;
        readNFCTag();
    }
    
    // 4. Nhận dữ liệu từ C5 Slave
    if (MasterSerial.available()) {
        String data = MasterSerial.readStringUntil('\n');
        processSlaveData(data);
    }
    
    // 5. Cập nhật màn hình
    display.update();
    
    delay(10);
}

// ==================== XỬ LÝ SỰ KIỆN PHÍM ====================
void handleKeyPress(int key) {
    switch(key) {
        case KEY_UP:
            Serial.println("Phím UP");
            display.showMessage("UP Pressed");
            MasterSerial.println("CMD:UP");
            break;
        case KEY_DOWN:
            Serial.println("Phím DOWN");
            display.showMessage("DOWN Pressed");
            MasterSerial.println("CMD:DOWN");
            break;
        case KEY_LEFT:
            Serial.println("Phím LEFT");
            display.showMessage("LEFT Pressed");
            break;
        case KEY_RIGHT:
            Serial.println("Phím RIGHT");
            display.showMessage("RIGHT Pressed");
            break;
        case KEY_CENTER:
            Serial.println("Phím CENTER");
            display.showMessage("CENTER Pressed - Scanning");
            performScan();
            break;
    }
}

// ==================== QUÉT NRF24 ====================
void scanNRF24Devices() {
    static uint8_t channel = 0;
    uint8_t rxAddr[6] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    
    nrf24.setChannel(channel);
    nrf24.openReadingPipe(1, rxAddr);
    nrf24.startListening();
    
    delay(2);
    
    if (nrf24.available()) {
        char buffer[32];
        nrf24.read(&buffer, sizeof(buffer));
        Serial.printf("[NRF24] Channel %d: %s\n", channel, buffer);
        display.showData("NRF", channel, buffer);
        MasterSerial.printf("NRF_DATA:%d:%s\n", channel, buffer);
    }
    
    nrf24.stopListening();
    channel = (channel + 1) % 126;
}

// ==================== ĐỌC NFC TAG ====================
void readNFCTag() {
    uint8_t uid[7];
    uint8_t uidLength;
    
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000)) {
        Serial.print("NFC Tag UID: ");
        char uidStr[32] = "";
        for(uint8_t i=0; i<uidLength; i++) {
            Serial.printf("%02X ", uid[i]);
            sprintf(uidStr + strlen(uidStr), "%02X", uid[i]);
        }
        Serial.println();
        
        display.showData("NFC", 0, uidStr);
        MasterSerial.printf("NFC_UID:%s\n", uidStr);
        
        // Thử đọc dữ liệu từ sector 0
        uint8_t data[16];
        if (nfc.mifareclassic_ReadDataBlock(4, data)) {
            Serial.print("Block 4 data: ");
            for(int i=0; i<16; i++) Serial.printf("%02X ", data[i]);
            Serial.println();
        }
    }
}

// ==================== XỬ LÝ DỮ LIỆU TỪ C5 SLAVE ====================
void processSlaveData(String data) {
    Serial.println("[C5] " + data);
    display.showData("C5", 0, data);
    
    if (data.startsWith("RF_SCAN:")) {
        // Nhận dữ liệu quét SubGhz từ C5
        String freq = data.substring(8);
        display.showData("SubGhz", freq.toInt(), "Detected");
    }
    else if (data == "SLAVE_READY") {
        Serial.println("ESP32-C5 Slave đã sẵn sàng");
        MasterSerial.println("CMD:START_SCAN");
    }
}

// ==================== QUÉT TỔNG HỢP ====================
void performScan() {
    display.showMessage("Scanning...");
    
    // Gửi lệnh cho C5 quét SubGhz
    MasterSerial.println("SCAN_SUBGHZ:315M");
    delay(500);
    MasterSerial.println("SCAN_SUBGHZ:433M");
    delay(500);
    MasterSerial.println("SCAN_SUBGHZ:868M");
    delay(500);
    
    // Quét IR
    ir.scanAllProtocols();
    
    // Quét NFC
    readNFCTag();
    
    display.showMessage("Scan Complete!");
}
