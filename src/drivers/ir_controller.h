#ifndef IR_CONTROLLER_H
#define IR_CONTROLLER_H

#include <Arduino.h>
#include <IRremote.h>

class IRController {
private:
    int txPin, rxPin;
    bool initialized;
    
public:
    IRController() : txPin(-1), rxPin(-1), initialized(false) {}
    
    void init(int tx, int rx) {
        txPin = tx;
        rxPin = rx;
        
        if (rxPin >= 0) {
            IrReceiver.begin(rxPin, ENABLE_LED_FEEDBACK);
            Serial.println("IR Receiver initialized");
        }
        
        if (txPin >= 0) {
            IrSender.begin(txPin);
            Serial.println("IR Transmitter initialized");
        }
        
        initialized = true;
    }
    
    void sendNEC(uint32_t address, uint32_t command) {
        if (!initialized || txPin < 0) return;
        
        IrSender.sendNEC(address, command, 0);
        Serial.printf("Sent IR NEC: addr=0x%08lX, cmd=0x%08lX\n", address, command);
    }
    
    void sendSony(uint32_t address, uint32_t command, uint8_t bits) {
        if (!initialized || txPin < 0) return;
        
        IrSender.sendSony(address, command, bits);
        Serial.printf("Sent IR Sony: addr=0x%08lX, cmd=0x%08lX, bits=%d\n", address, command, bits);
    }
    
    void sendRC5(uint32_t address, uint32_t command) {
        if (!initialized || txPin < 0) return;
        
        IrSender.sendRC5(address, command, 0);
        Serial.printf("Sent IR RC5: addr=0x%08lX, cmd=0x%08lX\n", address, command);
    }
    
    void scanAllProtocols() {
        if (!initialized || rxPin < 0) return;
        
        Serial.println("Scanning IR signals for 5 seconds...");
        unsigned long start = millis();
        
        while(millis() - start < 5000) {
            if (IrReceiver.decode()) {
                Serial.println("=== IR Signal Detected ===");
                Serial.printf("Protocol: %s\n", getProtocolString(IrReceiver.decodedIRData.protocol));
                Serial.printf("Address: 0x%08lX\n", IrReceiver.decodedIRData.address);
                Serial.printf("Command: 0x%08lX\n", IrReceiver.decodedIRData.command);
                Serial.printf("Raw: %d bits\n", IrReceiver.decodedIRData.numberOfBits);
                
                IrReceiver.resume();
            }
            delay(10);
        }
    }
    
    void tvBgone() {
        if (!initialized || txPin < 0) return;
        
        Serial.println("Starting TV-B-Gone mode...");
        
        // Danh sách mã tắt TV phổ biến (NEC protocol)
        uint32_t tvCodes[][2] = {
            {0x00FF, 0x40BF},  // Samsung
            {0x4CB3, 0x40BF},  // LG
            {0x04FB, 0x40BF},  // Sony
            {0x20DF, 0x10EF},  // Panasonic
            {0x40BF, 0x40BF},  // Philips
            {0x10EF, 0x10EF},  // Sharp
            {0x30CF, 0x30CF},  // Toshiba
            {0x50AF, 0x50AF}   // JVC
        };
        
        for(int i=0; i<10; i++) {
            for(int j=0; j<8; j++) {
                IrSender.sendNEC(tvCodes[j][0], tvCodes[j][1], 0);
                delay(100);
            }
            delay(500);
            Serial.printf("Round %d/10 completed\n", i+1);
        }
        
        Serial.println("TV-B-Gone completed");
    }
    
    String getProtocolString(decode_type_t protocol) {
        switch(protocol) {
            case NEC: return "NEC";
            case SONY: return "SONY";
            case RC5: return "RC5";
            case RC6: return "RC6";
            case PANASONIC: return "PANASONIC";
            case JVC: return "JVC";
            case SAMSUNG: return "SAMSUNG";
            case LG: return "LG";
            default: return "UNKNOWN";
        }
    }
    
    ~IRController() {
        if (rxPin >= 0) IrReceiver.stop();
    }
};

#endif
