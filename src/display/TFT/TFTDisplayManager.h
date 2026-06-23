#ifndef TFTDISPLAYMANAGER_H
#define TFTDISPLAYMANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h> // Adicionado

// Defina o pino CS do Touch (ajuste conforme seu hardware, ex: GPIO 21)
#define TOUCH_CS 21 

class TFTDisplayManager
{
public:
    TFTDisplayManager();
    void init();
    void showHelloWorld();
    void checkTouch(); // Novo método para verificar toque

private:
    Adafruit_ST7789 tft;
    XPT2046_Touchscreen ts; // Objeto de toque
};

#endif