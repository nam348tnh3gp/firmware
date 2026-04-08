/**
 * ESP32-C5 SLAVE FIRMWARE
 * Chạy trên C5: NRF24L01, CC1101, PN532, IR
 * Nhận lệnh từ S3 Master qua UART
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "pins_config.h"
#include "bruce_config.h"

// ==================== THƯ VIỆN ====================
#include <RF24.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <IRremote.h>

// ==================== KHỞI TẠO MODULES ====================
HardwareSerial SlaveSerial(1);  // UART1 giao tiếp với S3

// NRF24L01
RF24 radio(NRF_CE, NRF_CSN);

// CC1101 (custom)
class CC1101Driver {
private:
    int _csPin;
public:
    bool init(int csPin) {
        _csPin = csPin;
        pinMode(_csPin, OUTPUT);
        digitalWrite(_csPin, HIGH);
        SPI.begin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, _csPin);
        // TODO: Implement CC1101 init
        return true;
    }
    void setFrequency(float mhz) {}
    void setModeReceive() {}
    void setModeTransmit() {}
    int8_t getRSSI() { return random(-90, -30); }
    bool available() { return false; }
    int receive(uint8_t* buf, uint8_t maxLen) { return 0; }
    bool send(uint8_t* data, uint8_t len) { return true; }
} cc1101;

// PN532 NFC
PN532_I2C pn532_i2c(Wire);
PN532 nfc(pn532_i2c);

// IR
IRrecv irReceiver(IR_RX);
IRsend irSender(IR_TX);

// ==================== BIẾN TOÀN CỤC ====================
bool nrf24OK = false;
bool cc1101OK = false;
bool nfcOK = false;
bool jammerActive = false;
uint32_t lastJammerTime = 0;

// ==================== PROTOTYPE ====================
void processCommand(String cmd);
void scanRF();
void startJammer();
void stopJammer();
void readNFC();
void irSend(String data);
void irReceive();

// ==================== KHỞI TẠO ====================
void setup() {
    Serial.begin(115200);
    DEBUG_PRINT(3, "\n\n╔════════════════════════════════════╗\n");
    DEBUG_PRINT(3, "║     BRUCE - ESP32-C5 SLAVE v1.0     ║\n");
    DEBUG_PRINT(3, "╚════════════════════════════════════╝\n\n");
    
    // LED báo
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, HIGH);
    
    // UART với S3 Master
    SlaveSerial.begin(UART_BAUDRATE, SERIAL_8N1, UART_SLAVE_RX, UART_SLAVE_TX);
    DEBUG_PRINT(3, "[OK] UART with ESP32-S3\n");
    
    // SPI Bus cho RF
    SPI.begin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CS);
    DEBUG_PRINT(3, "[OK] SPI Bus initialized\n");
    
    // ===== NRF24L01 =====
    #if ENABLE_NRF24
    if (radio.begin()) {
        radio.setChannel(NRF24_DEFAULT_CHANNEL);
        radio.setPALevel(RF24_PA_MAX);
        radio.setDataRate(RF24_250KBPS);
        radio.setPayloadSize(32);
        radio.openReadingPipe(1, (uint8_t*)"BRUCE");
        radio.startListening();
        nrf24OK = true;
        DEBUG_PRINT(3, "[OK] NRF24L01 initialized\n");
    } else {
        DEBUG_PRINT(1, "[ERROR] NRF24L01 not detected\n");
    }
    #endif
    
    // ===== CC1101 =====
    #if ENABLE_CC1101
    if (cc1101.init(CC1101_CS)) {
        cc1101.setFrequency(CC1101_DEFAULT_FREQ);
        cc1101.setModeReceive();
        cc1101OK = true;
        DEBUG_PRINT(3, "[OK] CC1101 at %.2f MHz\n", CC1101_DEFAULT_FREQ);
    } else {
        DEBUG_PRINT(1, "[ERROR] CC1101 not detected\n");
    }
    #endif
    
    // ===== PN532 NFC =====
    #if ENABLE_PN532
    Wire.begin(PN532_SDA, PN532_SCL);
    nfc.begin();
    uint32_t version = nfc.getFirmwareVersion();
    if (version) {
        nfc.SAMConfig();
        nfcOK = true;
        DEBUG_PRINT(3, "[OK] PN532 found - FW %d.%d\n", 
                   (version >> 24) & 0xFF, (version >> 16) & 0xFF);
    } else {
        DEBUG_PRINT(1, "[ERROR] PN532 not found\n");
    }
    #endif
    
    // ===== IR =====
    #if ENABLE_IR
    irReceiver.enableIRIn();
    irSender.begin();
    DEBUG_PRINT(3, "[OK] IR Receiver & Transmitter\n");
    #endif
    
    digitalWrite(LED_STATUS, LOW);
    
    // Báo sẵn sàng cho S3
    delay(500);
    SlaveSerial.println("STATUS:OK");
    
    DEBUG_PRINT(3, "\n[READY] Waiting for commands from S3...\n");
}

// ==================== VÒNG LẶP CHÍNH ====================
void loop() {
    // 1. Nhận lệnh từ S3 Master
    if (SlaveSerial.available()) {
        String cmd = SlaveSerial.readStringUntil('\n');
        cmd.trim();
        processCommand(cmd);
    }
    
    // 2. Xử lý Jammer (chạy ngầm)
    if (jammerActive) {
        if (millis() - lastJammerTime > 100) {
            lastJammerTime = millis();
            // Gửi gói tin rác liên tục
            uint8_t junk[32];
            memset(junk, 0xFF, 32);
            radio.stopListening();
            radio.write(&junk, 32);
            radio.startListening();
        }
    }
    
    // 3. Quét IR tự động (nếu có tín hiệu)
    #if ENABLE_IR
    if (irReceiver.decode()) {
        String irData = String(irReceiver.decodedIRData.address, HEX) + "," +
                        String(irReceiver.decodedIRData.command, HEX);
        SlaveSerial.println("IR_RECEIVED:" + irData);
        irReceiver.resume();
    }
    #endif
    
    delay(10);
}

// ==================== XỬ LÝ LỆNH ====================
void processCommand(String cmd) {
    DEBUG_PRINT(2, "[CMD] %s\n", cmd.c_str());
    digitalWrite(LED_STATUS, HIGH);
    delay(20);
    digitalWrite(LED_STATUS, LOW);
    
    if (cmd == "STATUS") {
        SlaveSerial.println("STATUS:OK");
        SlaveSerial.printf("NRF24:%s,CC1101:%s,PN532:%s\n", 
                          nrf24OK ? "OK" : "FAIL",
                          cc1101OK ? "OK" : "FAIL", 
                          nfcOK ? "OK" : "FAIL");
    }
    else if (cmd == "PING") {
        SlaveSerial.println("PONG");
    }
    else if (cmd == "SCAN_RF") {
        scanRF();
    }
    else if (cmd == "JAMMER_START") {
        startJammer();
    }
    else if (cmd == "JAMMER_STOP") {
        stopJammer();
    }
    else if (cmd == "READ_NFC") {
        readNFC();
    }
    else if (cmd.startsWith("IR_SEND:")) {
        String data = cmd.substring(8);
        irSend(data);
    }
    else if (cmd == "IR_RECEIVE") {
        irReceive();
    }
    else if (cmd.startsWith("NRF_SET_CH:")) {
        int ch = cmd.substring(11).toInt();
        radio.setChannel(ch);
        SlaveSerial.printf("NRF_CH:%d\n", ch);
    }
    else if (cmd.startsWith("CC1101_SET_FREQ:")) {
        float freq = cmd.substring(16).toFloat();
        cc1101.setFrequency(freq);
        SlaveSerial.printf("CC1101_FREQ:%.2f\n", freq);
    }
    else {
        SlaveSerial.println("UNKNOWN_CMD");
    }
}

// ==================== QUÉT RF ====================
void scanRF() {
    DEBUG_PRINT(2, "[SCAN] Scanning RF bands...\n");
    SlaveSerial.println("RF_SCAN:START");
    
    // Quét CC1101 các tần số phổ biến
    float freqs[] = {315.0, 433.92, 868.0, 915.0};
    const char* bands[] = {"315M", "433M", "868M", "915M"};
    
    for (int i = 0; i < 4; i++) {
        cc1101.setFrequency(freqs[i]);
        cc1101.setModeReceive();
        delay(50);
        
        int8_t rssi = cc1101.getRSSI();
        if (rssi > -80) {
            DEBUG_PRINT(2, "[SCAN] %s: RSSI=%d dBm\n", bands[i], rssi);
            SlaveSerial.printf("RF_SCAN:%s,RSSI=%d\n", bands[i], rssi);
        }
        
        // Thử nhận dữ liệu
        if (cc1101.available()) {
            uint8_t buffer[64];
            int len = cc1101.receive(buffer, 64);
            if (len > 0) {
                SlaveSerial.print("RF_DATA:");
                for (int j = 0; j < len; j++) {
                    SlaveSerial.printf("%02X", buffer[j]);
                }
                SlaveSerial.println();
            }
        }
    }
    
    // Quét NRF24
    for (int ch = 0; ch < 126; ch++) {
        radio.setChannel(ch);
        radio.startListening();
        delay(2);
        
        if (radio.testRPD()) {
            DEBUG_PRINT(2, "[SCAN] NRF24 Channel %d active\n", ch);
            SlaveSerial.printf("RF_SCAN:NRF_CH%d\n", ch);
        }
    }
    
    SlaveSerial.println("RF_SCAN:DONE");
}

// ==================== JAMMER ====================
void startJammer() {
    DEBUG_PRINT(2, "[JAMMER] Starting...\n");
    jammerActive = true;
    SlaveSerial.println("JAMMER_STARTED");
}

void stopJammer() {
    DEBUG_PRINT(2, "[JAMMER] Stopping...\n");
    jammerActive = false;
    SlaveSerial.println("JAMMER_STOPPED");
}

// ==================== NFC ====================
void readNFC() {
    DEBUG_PRINT(2, "[NFC] Waiting for tag...\n");
    
    uint8_t uid[7];
    uint8_t uidLength;
    
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 2000)) {
        char uidStr[32] = "";
        for (int i = 0; i < uidLength; i++) {
            sprintf(uidStr + strlen(uidStr), "%02X", uid[i]);
        }
        DEBUG_PRINT(2, "[NFC] Tag UID: %s\n", uidStr);
        SlaveSerial.printf("NFC_UID:%s\n", uidStr);
    } else {
        SlaveSerial.println("NFC:NONE");
    }
}

// ==================== IR ====================
void irSend(String data) {
    // Parse "0x00FF,0x40BF" format
    int comma = data.indexOf(',');
    if (comma > 0) {
        uint32_t address = strtoul(data.substring(0, comma).c_str(), NULL, 16);
        uint32_t command = strtoul(data.substring(comma + 1).c_str(), NULL, 16);
        
        irSender.sendNEC(address, command, 0);
        DEBUG_PRINT(2, "[IR] Sent NEC: 0x%08lX, 0x%08lX\n", address, command);
        SlaveSerial.printf("IR_SENT:%s\n", data.c_str());
    }
}

void irReceive() {
    DEBUG_PRINT(2, "[IR] Listening for 5 seconds...\n");
    uint32_t start = millis();
    
    while (millis() - start < 5000) {
        if (irReceiver.decode()) {
            String result = String(irReceiver.decodedIRData.address, HEX) + "," +
                           String(irReceiver.decodedIRData.command, HEX);
            SlaveSerial.printf("IR_RECEIVED:%s\n", result.c_str());
            irReceiver.resume();
            return;
        }
        delay(10);
    }
    
    SlaveSerial.println("IR_RECEIVED:NONE");
}
