#ifndef DISPLAY_ST7789_H
#define DISPLAY_ST7789_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ST7789.h>

class DisplayST7789 {
private:
    Adafruit_ST7789* tft;
    int width, height;
    String lastMessage;
    unsigned long lastUpdate;
    
public:
    DisplayST7789() : tft(nullptr), width(240), height(320), lastUpdate(0) {}
    
    bool init(int mosi, int sclk, int cs, int dc, int rst) {
        SPI.begin(sclk, -1, mosi, cs);
        tft = new Adafruit_ST7789(&SPI, cs, dc, rst);
        
        tft->init(width, height);
        tft->setRotation(1);
        tft->fillScreen(ST77XX_BLACK);
        
        // Test kết nối
        tft->setTextColor(ST77XX_GREEN);
        tft->setTextSize(2);
        tft->setCursor(10, 10);
        tft->println("ST7789 OK");
        
        delay(1000);
        return true;
    }
    
    void showSplash() {
        tft->fillScreen(ST77XX_BLACK);
        tft->setTextColor(ST77XX_CYAN);
        tft->setTextSize(3);
        tft->setCursor(40, 100);
        tft->println("BRUCE");
        tft->setTextSize(1);
        tft->setCursor(30, 150);
        tft->println("ESP32-S3 MASTER");
        tft->setCursor(30, 170);
        tft->println("System Ready");
    }
    
    void showReady() {
        tft->fillScreen(ST77XX_BLACK);
        drawStatusBar();
        tft->setTextColor(ST77XX_GREEN);
        tft->setTextSize(2);
        tft->setCursor(20, 80);
        tft->println("READY");
    }
    
    void showMessage(String msg) {
        tft->fillRect(20, 200, 200, 40, ST77XX_BLACK);
        tft->setTextColor(ST77XX_YELLOW);
        tft->setTextSize(1);
        tft->setCursor(20, 210);
        tft->println(msg);
        lastMessage = msg;
        lastUpdate = millis();
    }
    
    void showData(String label, int value, String extra = "") {
        static int line = 0;
        int y = 120 + (line * 20);
        
        tft->fillRect(10, y, 220, 18, ST77XX_BLACK);
        tft->setTextColor(ST77XX_WHITE);
        tft->setTextSize(1);
        tft->setCursor(10, y);
        tft->printf("%s: %d %s", label.c_str(), value, extra.c_str());
        
        line = (line + 1) % 6;
    }
    
    void update() {
        drawStatusBar();
        
        // Xóa thông báo cũ sau 2 giây
        if (lastUpdate > 0 && (millis() - lastUpdate) > 2000) {
            tft->fillRect(20, 200, 200, 40, ST77XX_BLACK);
            lastUpdate = 0;
        }
    }
    
private:
    void drawStatusBar() {
        // Vẽ thanh trạng thái trên cùng
        tft->fillRect(0, 0, width, 20, ST77XX_BLUE);
        tft->setTextColor(ST77XX_WHITE);
        tft->setTextSize(1);
        tft->setCursor(5, 5);
        tft->print("BRUCE v1.0");
        
        // Hiển thị thời gian (giả lập)
        tft->setCursor(width - 50, 5);
        tft->print(millis() / 1000);
        tft->print("s");
    }
};

#endif
