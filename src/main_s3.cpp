/**
 * ESP32-S3 MASTER FIRMWARE
 * Chạy trên S3: Màn hình, SD Card, Phím 5-way, Giao tiếp UART với C5
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "pins_config.h"
#include "bruce_config.h"

// ==================== KHỞI TẠO ====================
HardwareSerial MasterSerial(1);  // UART1 cho giao tiếp với C5

// Thư viện màn hình ST7789
#include <Adafruit_ST7789.h>
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Biến trạng thái
String lastCommand = "";
uint32_t lastHeartbeat = 0;
bool c5Connected = false;

// ==================== PROTOTYPE ====================
void setupDisplay();
void updateDisplay(String line1, String line2 = "", String line3 = "");
void processSlaveData(String data);
void sendCommand(String cmd);
void handleKeypad();

// ==================== KHỞI TẠO ====================
void setup() {
    Serial.begin(115200);
    DEBUG_PRINT(3, "\n\n╔════════════════════════════════════╗\n");
    DEBUG_PRINT(3, "║     BRUCE - ESP32-S3 MASTER v1.0    ║\n");
    DEBUG_PRINT(3, "╚════════════════════════════════════╝\n\n");
    
    // 1. LED báo
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, HIGH);
    
    // 2. UART với C5 Slave
    MasterSerial.begin(UART_BAUDRATE, SERIAL_8N1, UART_MASTER_RX, UART_MASTER_TX);
    DEBUG_PRINT(3, "[OK] UART with ESP32-C5\n");
    
    // 3. Màn hình
    setupDisplay();
    
    // 4. SD Card
    #if ENABLE_SDCARD
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, SD_CS);
    if (SD.begin(SD_CS)) {
        DEBUG_PRINT(3, "[OK] SD Card mounted\n");
        updateDisplay("SD Card OK", "");
    } else {
        DEBUG_PRINT(1, "[ERROR] SD Card failed\n");
        updateDisplay("SD Card ERROR", "");
    }
    #endif
    
    // 5. Phím 5-way
    #if ENABLE_KEYPAD
    pinMode(KEY_UP, INPUT_PULLUP);
    pinMode(KEY_DOWN, INPUT_PULLUP);
    pinMode(KEY_LEFT, INPUT_PULLUP);
    pinMode(KEY_RIGHT, INPUT_PULLUP);
    pinMode(KEY_CENTER, INPUT_PULLUP);
    DEBUG_PRINT(3, "[OK] 5-way Keypad\n");
    #endif
    
    // 6. Gửi tín hiệu sẵn sàng cho C5
    delay(500);
    sendCommand("STATUS");
    
    digitalWrite(LED_STATUS, LOW);
    updateDisplay("SYSTEM READY", "RF on ESP32-C5", "");
    
    DEBUG_PRINT(3, "\n[READY] Waiting for commands...\n");
}

// ==================== VÒNG LẶP CHÍNH ====================
void loop() {
    // 1. Nhận dữ liệu từ C5 Slave
    if (MasterSerial.available()) {
        String data = MasterSerial.readStringUntil('\n');
        processSlaveData(data);
    }
    
    // 2. Xử lý phím bấm
    #if ENABLE_KEYPAD
    handleKeypad();
    #endif
    
    // 3. Heartbeat mỗi 5 giây
    if (millis() - lastHeartbeat > 5000) {
        lastHeartbeat = millis();
        sendCommand("PING");
    }
    
    delay(10);
}

// ==================== MÀN HÌNH ====================
void setupDisplay() {
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft.init(240, 320);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    
    tft.setTextColor(ST77XX_CYAN);
    tft.setTextSize(3);
    tft.setCursor(60, 80);
    tft.println("BRUCE");
    
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(30, 150);
    tft.println("ESP32-S3 MASTER");
    
    tft.setCursor(30, 180);
    tft.println("RF on ESP32-C5");
    
    delay(2000);
    tft.fillScreen(ST77XX_BLACK);
}

void updateDisplay(String line1, String line2, String line3) {
    static int lastY = 0;
    
    tft.fillRect(0, lastY, 240, 20, ST77XX_BLACK);
    
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);
    tft.setCursor(5, 10);
    tft.println(line1);
    
    if (line2.length() > 0) {
        tft.setCursor(5, 30);
        tft.println(line2);
        lastY = 30;
    }
    if (line3.length() > 0) {
        tft.setCursor(5, 50);
        tft.println(line3);
        lastY = 50;
    }
}

void showMenu() {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(5, 10);
    tft.println("=== BRUCE MENU ===");
    tft.setCursor(5, 40);
    tft.println("[UP] Scan RF");
    tft.setCursor(5, 60);
    tft.println("[DOWN] Start Jammer");
    tft.setCursor(5, 80);
    tft.println("[LEFT] Read NFC");
    tft.setCursor(5, 100);
    tft.println("[RIGHT] IR Test");
    tft.setCursor(5, 120);
    tft.println("[CENTER] Status");
}

// ==================== XỬ LÝ PHÍM ====================
void handleKeypad() {
    if (digitalRead(KEY_CENTER) == LOW) {
        delay(50);
        if (digitalRead(KEY_CENTER) == LOW) {
            showMenu();
            while(digitalRead(KEY_CENTER) == LOW) delay(10);
        }
    }
    
    if (digitalRead(KEY_UP) == LOW) {
        delay(50);
        if (digitalRead(KEY_UP) == LOW) {
            updateDisplay("SCAN RF...", "");
            sendCommand("SCAN_RF");
            while(digitalRead(KEY_UP) == LOW) delay(10);
        }
    }
    
    if (digitalRead(KEY_DOWN) == LOW) {
        delay(50);
        if (digitalRead(KEY_DOWN) == LOW) {
            updateDisplay("JAMMER START", "");
            sendCommand("JAMMER_START");
            while(digitalRead(KEY_DOWN) == LOW) delay(10);
        }
    }
    
    if (digitalRead(KEY_LEFT) == LOW) {
        delay(50);
        if (digitalRead(KEY_LEFT) == LOW) {
            updateDisplay("READ NFC...", "");
            sendCommand("READ_NFC");
            while(digitalRead(KEY_LEFT) == LOW) delay(10);
        }
    }
    
    if (digitalRead(KEY_RIGHT) == LOW) {
        delay(50);
        if (digitalRead(KEY_RIGHT) == LOW) {
            updateDisplay("IR SEND TEST", "");
            sendCommand("IR_SEND:0x00FF,0x40BF"); // Samsung TV power
            while(digitalRead(KEY_RIGHT) == LOW) delay(10);
        }
    }
}

// ==================== GỬI LỆNH CHO C5 ====================
void sendCommand(String cmd) {
    MasterSerial.println(cmd);
    DEBUG_PRINT(3, "[S3→C5] %s\n", cmd.c_str());
    digitalWrite(LED_STATUS, HIGH);
    delay(20);
    digitalWrite(LED_STATUS, LOW);
}

// ==================== XỬ LÝ DỮ LIỆU TỪ C5 ====================
void processSlaveData(String data) {
    DEBUG_PRINT(2, "[C5→S3] %s\n", data.c_str());
    digitalWrite(LED_STATUS, HIGH);
    delay(50);
    digitalWrite(LED_STATUS, LOW);
    
    if (data == "PONG") {
        c5Connected = true;
        updateDisplay("C5 Connected", "");
    }
    else if (data == "STATUS:OK") {
        updateDisplay("C5 Status: OK", "NRF24+CC1101 Ready");
    }
    else if (data.startsWith("RF_SCAN:")) {
        String info = data.substring(8);
        updateDisplay("RF Detected!", info);
        // Ghi log ra SD Card
        #if ENABLE_SDCARD
        File logFile = SD.open("/rf_log.txt", FILE_APPEND);
        if (logFile) {
            logFile.print(millis());
            logFile.print(": ");
            logFile.println(info);
            logFile.close();
        }
        #endif
    }
    else if (data.startsWith("NFC_UID:")) {
        String uid = data.substring(8);
        updateDisplay("NFC Tag Found", "UID: " + uid);
    }
    else if (data.startsWith("IR_RECEIVED:")) {
        String irData = data.substring(12);
        updateDisplay("IR Received", irData);
    }
    else if (data == "JAMMER_STARTED") {
        updateDisplay("JAMMER ACTIVE", "Press CENTER to stop");
    }
    else if (data == "JAMMER_STOPPED") {
        updateDisplay("JAMMER STOPPED", "");
    }
    else if (data == "PONG") {
        // Heartbeat response
    }
    else {
        updateDisplay("C5:", data);
    }
}
