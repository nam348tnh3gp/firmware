/**
 * ESP32-C5 SLAVE FIRMWARE
 * Chạy trên board ESP32-C5, nhận lệnh từ S3 Master qua UART
 */

#include <Arduino.h>
#include <SPI.h>
#include "pins_config.h"

// ==================== KHỞI TẠO ====================
HardwareSerial MasterSerial(1);  // UART1 giao tiếp với S3

// Cấu hình RF (SubGhz - giả lập, có thể thay bằng thư viện CC1101)
#define RF_FREQ_315 315000000
#define RF_FREQ_433 433920000
#define RF_FREQ_868 868000000

// LED báo trạng thái
#ifdef SLAVE_LED
#define LED_BUILTIN SLAVE_LED
#else
#define LED_BUILTIN 2
#endif

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    Serial.println("\n\n=== ESP32-C5 SLAVE FIRMWARE ===\n");
    
    // Khởi tạo UART với Master (S3)
    MasterSerial.begin(UART_BAUDRATE, SERIAL_8N1, UART_MASTER_RX, UART_MASTER_TX);
    Serial.println("[OK] UART initialized");
    
    // Khởi tạo SPI cho các module RF (nếu có)
    SPI.begin();
    
    // Báo hiệu sẵn sàng cho Master
    MasterSerial.println("SLAVE_READY");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    
    Serial.println("[OK] Slave ready, waiting for commands...\n");
}

void loop() {
    // Nhận lệnh từ Master (ESP32-S3)
    if (MasterSerial.available()) {
        String command = MasterSerial.readStringUntil('\n');
        command.trim();
        
        Serial.print("[CMD] Received: ");
        Serial.println(command);
        
        // Xử lý lệnh
        String response = processCommand(command);
        
        // Gửi phản hồi về Master
        if (response.length() > 0) {
            MasterSerial.println(response);
            Serial.print("[RESP] Sent: ");
            Serial.println(response);
        }
        
        // Blink LED khi nhận lệnh
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
    }
    
    delay(10);
}

// ==================== XỬ LÝ LỆNH ====================
String processCommand(String cmd) {
    String response = "";
    
    if (cmd == "STATUS") {
        response = "STATUS:ESP32-C5_Slave_OK";
    }
    else if (cmd == "SCAN_SUBGHZ:315M") {
        response = scanSubGhz(RF_FREQ_315);
    }
    else if (cmd == "SCAN_SUBGHZ:433M") {
        response = scanSubGhz(RF_FREQ_433);
    }
    else if (cmd == "SCAN_SUBGHZ:868M") {
        response = scanSubGhz(RF_FREQ_868);
    }
    else if (cmd == "START_JAMMER") {
        startJammer();
        response = "JAMMER_STARTED";
    }
    else if (cmd == "STOP_JAMMER") {
        stopJammer();
        response = "JAMMER_STOPPED";
    }
    else if (cmd.startsWith("SEND_RAW:")) {
        String rawData = cmd.substring(9);
        response = sendRawRF(rawData);
    }
    else if (cmd == "HELP") {
        response = "CMDS: STATUS, SCAN_SUBGHZ:<freq>, START_JAMMER, STOP_JAMMER, SEND_RAW:<hex>";
    }
    else {
        response = "UNKNOWN_CMD";
    }
    
    return response;
}

// ==================== QUÉT SUBGHZ ====================
String scanSubGhz(uint32_t frequency) {
    Serial.printf("[RF] Scanning at %lu Hz...\n", frequency);
    
    // TODO: Implement thực tế với module CC1101
    // Hiện tại giả lập quét
    
    delay(500);  // Giả lập thời gian quét
    
    // Random phát hiện tín hiệu (demo)
    if (random(0, 100) > 70) {
        uint32_t detectedFreq = frequency;
        int rssi = random(-90, -30);
        
        char result[64];
        sprintf(result, "RF_SCAN:%lu,RSSI:%d", detectedFreq, rssi);
        return String(result);
    }
    
    return "RF_SCAN:NONE";
}

// ==================== JAMMER ====================
void startJammer() {
    Serial.println("[RF] Starting jammer...");
    
    // TODO: Implement jammer với CC1101
    // Hiện tại chỉ bật LED nhấp nháy
    for(int i=0; i<10; i++) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(100);
    }
}

void stopJammer() {
    Serial.println("[RF] Stopping jammer...");
    digitalWrite(LED_BUILTIN, LOW);
}

// ==================== GỬI RAW RF ====================
String sendRawRF(String hexData) {
    Serial.printf("[RF] Sending raw data: %s\n", hexData.c_str());
    
    // TODO: Parse hex và gửi qua CC1101
    
    return "RAW_SENT:" + hexData;
}
