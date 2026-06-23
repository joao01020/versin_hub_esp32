#ifndef TFTDISPLAYMANAGER_H
#define TFTDISPLAYMANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>

// Defina o pino CS do Touch conforme seu main.cpp
#define TOUCH_CS_PIN 15 

class TFTDisplayManager
{
public:
    TFTDisplayManager();

    void init();
    void checkTouch();
    void drawScreen(const char* text);

private:
    Adafruit_ST7789 tft;
    XPT2046_Touchscreen ts;

    // Variáveis de controle do Touch (mantidas caso queira usar para cliques simples no futuro)
    bool wasTouched;
    int16_t startTouchY;
    int16_t lastTouchY;
};

#endif