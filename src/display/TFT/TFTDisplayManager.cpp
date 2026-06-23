#include "TFTDisplayManager.h"

#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  33

// Inicializa ambos: TFT e TS no mesmo barramento SPI
TFTDisplayManager::TFTDisplayManager()
    : tft(TFT_CS, TFT_DC, TFT_RST), ts(TOUCH_CS)
{
}

void TFTDisplayManager::init()
{
    Serial.println("\n[TFT & TOUCH] INIT START");
    
    // Inicializa o Touch
    if (!ts.begin()) {
        Serial.println("Erro ao iniciar o TouchScreen!");
    } else {
        Serial.println("TouchScreen iniciado com sucesso!");
    }

    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_RST, OUTPUT);

    digitalWrite(TFT_RST, HIGH);
    delay(50);
    digitalWrite(TFT_RST, LOW);
    delay(50);
    digitalWrite(TFT_RST, HIGH);
    delay(200);

    tft.init(240, 320, SPI_MODE0);
    tft.invertDisplay(true); 
    tft.setRotation(1);
    
    tft.fillScreen(ST77XX_WHITE);
    Serial.println("[TFT] READY");
}

void TFTDisplayManager::checkTouch()
{
    // Verifica se há pressão na tela
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        
        // Imprime as coordenadas cruas no Serial
        Serial.print("Toque detectado! X: ");
        Serial.print(p.x);
        Serial.print(" Y: ");
        Serial.println(p.y);
        
        // Opcional: Desenhar um círculo no ponto de toque (conversão necessária conforme rotação)
        // int16_t screenX = map(p.x, 200, 3700, 0, 320); 
        // int16_t screenY = map(p.y, 200, 3700, 0, 240);
        // tft.fillCircle(screenX, screenY, 5, ST77XX_RED);
    }
}

void TFTDisplayManager::showHelloWorld()
{
    tft.fillScreen(ST77XX_WHITE);
    tft.setCursor(20, 80);
    tft.setTextColor(ST77XX_BLACK);
    tft.setTextSize(3);
    tft.print("VERSIN");
}