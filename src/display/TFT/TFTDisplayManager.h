#ifndef TFTDISPLAYMANAGER_H
#define TFTDISPLAYMANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

class TFTDisplayManager
{
public:
    TFTDisplayManager();

    void init();
    void showHelloWorld();

private:
    Adafruit_ST7789 tft;
};

#endif