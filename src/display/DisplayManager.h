#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ST7789.h> 
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <SD.h> // EN: Native Secure Digital storage library / PT: Biblioteca nativa de armazenamento do Cartão SD

class DisplayManager {
private:
    Adafruit_SSD1306 display; 
    Adafruit_ST7789 tft;      
    
    static const int TFT_CS  = 15;
    static const int TFT_DC  = 2;
    static const int TFT_RST = 4;
    static const int SD_CS   = 13; // EN: Exclusive Chip Select for SD Module / PT: Chip Select exclusivo para o Módulo SD

    // Variáveis para controle das animações
    int eyePulseRadius = 50;
    bool pulseGrowing = true;

    // Funções internas de desenho dos elementos visuais
    void drawSystemEye(int irisRadius); 
    void drawVersinLogo(int x, int y); 
    void drawContractIcon(int x, int y);
    
    // Novas funções geométricas para a animação do cadeado Anti-Tamper
    void drawLockClosed(int x, int y);
    void drawLockOpen(int x, int y);

public:
    DisplayManager();
    
    void initDisplays(); 
    void updateSystemVisual(bool isWifiConnected, String operation); 
};

#endif // DISPLAY_MANAGER_H