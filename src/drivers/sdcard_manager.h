#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <FS.h>

class SDCardManager {
private:
    bool mounted;
    
public:
    SDCardManager() : mounted(false) {}
    
    bool init(int cs, int mosi, int miso, int sck) {
        SPI.begin(sck, miso, mosi, cs);
        
        if (!SD.begin(cs)) {
            Serial.println("SD Card mount failed!");
            mounted = false;
            return false;
        }
        
        mounted = true;
        Serial.printf("SD Card mounted, total space: %llu MB\n", SD.totalBytes() / (1024 * 1024));
        return true;
    }
    
    bool writeFile(const char* path, const char* data) {
        if (!mounted) return false;
        
        File file = SD.open(path, FILE_WRITE);
        if (!file) {
            Serial.printf("Failed to open %s for writing\n", path);
            return false;
        }
        
        size_t written = file.print(data);
        file.close();
        
        return written > 0;
    }
    
    String readFile(const char* path) {
        if (!mounted) return "";
        
        File file = SD.open(path);
        if (!file) {
            Serial.printf("Failed to open %s for reading\n", path);
            return "";
        }
        
        String content = file.readString();
        file.close();
        return content;
    }
    
    void listFiles(const char* dirname) {
        if (!mounted) return;
        
        File root = SD.open(dirname);
        if (!root) {
            Serial.println("Failed to open directory");
            return;
        }
        
        File file = root.openNextFile();
        while(file) {
            if(file.isDirectory()) {
                Serial.printf("DIR: %s\n", file.name());
            } else {
                Serial.printf("FILE: %s, SIZE: %d bytes\n", file.name(), file.size());
            }
            file = root.openNextFile();
        }
    }
    
    bool exists(const char* path) {
        return mounted ? SD.exists(path) : false;
    }
    
    bool removeFile(const char* path) {
        return mounted ? SD.remove(path) : false;
    }
    
    void logData(const char* tag, String data) {
        String logEntry = "[" + String(millis()) + "] " + tag + ": " + data + "\n";
        writeFile("/log.txt", logEntry.c_str());
    }
    
    ~SDCardManager() {
        if (mounted) SD.end();
    }
};

#endif
