#ifndef NRF24_BRUCE_H
#define NRF24_BRUCE_H

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

class NRF24Bruce {
private:
    RF24* radio;
    uint8_t address[6] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    bool initialized;
    
public:
    NRF24Bruce() : radio(nullptr), initialized(false) {}
    
    bool init(int csn, int ce, int mosi, int miso, int sck) {
        SPI.begin(sck, miso, mosi, csn);
        radio = new RF24(ce, csn);
        
        if (!radio->begin()) {
            Serial.println("NRF24L01 not detected!");
            return false;
        }
        
        radio->setChannel(100);
        radio->setPayloadSize(32);
        radio->setDataRate(RF24_250KBPS);
        radio->setPALevel(RF24_PA_MAX);
        radio->setAutoAck(true);
        radio->setRetries(5, 15);
        
        initialized = true;
        Serial.println("NRF24L01 initialized successfully");
        return true;
    }
    
    void setChannel(uint8_t ch) {
        if (initialized) radio->setChannel(ch);
    }
    
    void setDataRate(rf24_datarate_e rate) {
        if (initialized) radio->setDataRate(rate);
    }
    
    void startListening() {
        if (initialized) {
            radio->openReadingPipe(1, address);
            radio->startListening();
        }
    }
    
    void stopListening() {
        if (initialized) radio->stopListening();
    }
    
    bool available() {
        return initialized ? radio->available() : false;
    }
    
    void read(void* buffer, uint8_t size) {
        if (initialized) radio->read(buffer, size);
    }
    
    bool send(const void* buffer, uint8_t size) {
        if (!initialized) return false;
        
        radio->stopListening();
        radio->openWritingPipe(address);
        
        bool success = radio->write(buffer, size);
        
        radio->startListening();
        return success;
    }
    
    void startJammer() {
        if (!initialized) return;
        
        radio->stopListening();
        radio->setAutoAck(false);
        radio->setTXPower(RF24_PA_MAX);
        
        // Gửi liên tục các gói tin rác
        uint8_t junk[32];
        memset(junk, 0xFF, 32);
        
        for(int i=0; i<100; i++) {
            radio->openWritingPipe(address);
            radio->write(&junk, 32);
            delay(1);
        }
        
        radio->setAutoAck(true);
    }
    
    void scanChannels() {
        if (!initialized) return;
        
        for(uint8_t ch=0; ch<126; ch++) {
            radio->setChannel(ch);
            radio->startListening();
            delay(2);
            
            if(radio->testRPD()) {  // RPD = Received Power Detector
                Serial.printf("Activity on channel %d\n", ch);
            }
        }
        radio->startListening();
    }
    
    ~NRF24Bruce() {
        if (radio) delete radio;
    }
};

#endif
