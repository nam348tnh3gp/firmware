#ifndef NFC_PN532_H
#define NFC_PN532_H

#include <Arduino.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

class NFCPN532 {
private:
    PN532_I2C* pn532_i2c;
    PN532* nfc;
    bool initialized;
    
public:
    NFCPN532() : pn532_i2c(nullptr), nfc(nullptr), initialized(false) {}
    
    bool init(TwoWire* wire, int irq = -1, int reset = -1) {
        pn532_i2c = new PN532_I2C(*wire);
        nfc = new PN532(*pn532_i2c);
        
        nfc->begin();
        
        uint32_t versiondata = nfc->getFirmwareVersion();
        if (!versiondata) {
            Serial.println("PN532 not found!");
            return false;
        }
        
        Serial.printf("PN532 found - Firmware: %d.%d\n", 
                     (versiondata >> 24) & 0xFF, 
                     (versiondata >> 16) & 0xFF);
        
        nfc->SAMConfig();
        initialized = true;
        return true;
    }
    
    bool readPassiveTargetID(uint8_t cardbaudrate, uint8_t* uid, uint8_t* uidLength, uint16_t timeout) {
        if (!initialized) return false;
        return nfc->readPassiveTargetID(cardbaudrate, uid, uidLength, timeout);
    }
    
    bool mifareclassic_ReadDataBlock(uint8_t block, uint8_t* data) {
        if (!initialized) return false;
        
        uint8_t key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t currentBlock = block;
        
        // Xác thực sector
        uint8_t sector = block / 4;
        uint8_t trailerBlock = sector * 4 + 3;
        
        if (block == trailerBlock) return false; // Không đọc block trailer
        
        if (!nfc->mifareclassic_AuthenticateBlock(uid, uidLength, block, 0, key)) {
            Serial.println("Authentication failed!");
            return false;
        }
        
        return nfc->mifareclassic_ReadDataBlock(block, data);
    }
    
    bool mifareclassic_WriteDataBlock(uint8_t block, uint8_t* data) {
        if (!initialized) return false;
        
        uint8_t key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        
        if (!nfc->mifareclassic_AuthenticateBlock(uid, uidLength, block, 0, key)) {
            Serial.println("Authentication failed for write!");
            return false;
        }
        
        return nfc->mifareclassic_WriteDataBlock(block, data);
    }
    
    void setPassiveActivationRetries(uint8_t retries) {
        if (initialized) nfc->setPassiveActivationRetries(retries);
    }
    
    void emulateTag(uint8_t* uid, uint8_t uidLength) {
        if (!initialized) return;
        
        // Cấu hình chế độ emulate
        nfc->setPassiveActivationRetries(0xFF);
        
        // TODO: Implement tag emulation
        Serial.println("Tag emulation started");
    }
    
    ~NFCPN532() {
        if (nfc) delete nfc;
        if (pn532_i2c) delete pn532_i2c;
    }
};

#endif
