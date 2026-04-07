#ifndef KEYPAD_HANDLER_H
#define KEYPAD_HANDLER_H

#include <Arduino.h>

struct KeyEvent {
    int key;
    bool pressed;
    bool longPress;
    uint32_t duration;
};

class KeypadHandler {
private:
    int pins[5];
    bool lastStates[5];
    uint32_t pressTimes[5];
    KeyEvent lastEvent;
    
public:
    KeypadHandler() {
        memset(lastStates, 0, sizeof(lastStates));
        memset(pressTimes, 0, sizeof(pressTimes));
        lastEvent.pressed = false;
    }
    
    void init(int up, int down, int left, int right, int center) {
        pins[0] = up;
        pins[1] = down;
        pins[2] = left;
        pins[3] = right;
        pins[4] = center;
        
        for(int i=0; i<5; i++) {
            pinMode(pins[i], INPUT_PULLUP);
            lastStates[i] = digitalRead(pins[i]);
        }
        
        Serial.println("Keypad initialized (INPUT_PULLUP mode)");
    }
    
    KeyEvent getEvent() {
        KeyEvent event;
        event.pressed = false;
        
        for(int i=0; i<5; i++) {
            bool currentState = digitalRead(pins[i]);
            
            // Phím nhấn (LOW vì INPUT_PULLUP)
            if (currentState == LOW && lastStates[i] == HIGH) {
                event.key = pins[i];
                event.pressed = true;
                event.longPress = false;
                pressTimes[i] = millis();
                lastEvent = event;
                Serial.printf("Key %d pressed\n", pins[i]);
            }
            
            // Phím thả ra
            if (currentState == HIGH && lastStates[i] == LOW) {
                uint32_t duration = millis() - pressTimes[i];
                if (duration > 2000) {
                    // Long press
                    event.key = pins[i];
                    event.pressed = true;
                    event.longPress = true;
                    event.duration = duration;
                    lastEvent = event;
                    Serial.printf("Key %d long pressed (%d ms)\n", pins[i], duration);
                }
            }
            
            lastStates[i] = currentState;
        }
        
        return event;
    }
    
    int waitForKey(uint32_t timeout = 0) {
        uint32_t start = millis();
        
        while(true) {
            for(int i=0; i<5; i++) {
                if (digitalRead(pins[i]) == LOW) {
                    delay(50); // Debounce
                    if (digitalRead(pins[i]) == LOW) {
                        return pins[i];
                    }
                }
            }
            
            if (timeout > 0 && (millis() - start) > timeout) {
                return -1;
            }
            
            delay(10);
        }
    }
    
    String getKeyName(int key) {
        switch(key) {
            case 1: return "UP";
            case 2: return "DOWN";
            case 3: return "LEFT";
            case 38: return "RIGHT";
            case 39: return "CENTER";
            default: return "UNKNOWN";
        }
    }
};

#endif
