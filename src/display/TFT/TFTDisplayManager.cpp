#include "TFTDisplayManager.h"

// Configuração atualizada conforme sua nova pinagem
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  33 // Alterado para GPIO 33 conforme sua escolha

TFTDisplayManager::TFTDisplayManager()
    : tft(TFT_CS, TFT_DC, TFT_RST)
{
}

void TFTDisplayManager::init()
{
    Serial.println("\n====================");
    Serial.println("   TFT HARDWARE TEST");
    Serial.println("====================");

    // Definindo os pinos do display como saída
    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_RST, OUTPUT);

    // Sequência de Reset físico
    digitalWrite(TFT_RST, HIGH);
    delay(50);
    digitalWrite(TFT_RST, LOW);
    delay(50);
    digitalWrite(TFT_RST, HIGH);
    delay(200);

    Serial.println("\n[TFT] INIT START");
    
    // Inicialização do barramento SPI (note que o SPI já deve ter sido 
    // iniciado no main.cpp com SPI.begin(18, 19, 23))
    tft.init(240, 320, SPI_MODE0);
    
    // Configurações de visualização
    tft.invertDisplay(true); 
    tft.setRotation(1);
    
    // Teste visual
    tft.fillScreen(ST77XX_WHITE);
    tft.setTextWrap(false);
    tft.setTextColor(ST77XX_BLACK);
    tft.setTextSize(3);

    Serial.println("[TFT] READY");
}

void TFTDisplayManager::showHelloWorld()
{
    Serial.println("[TFT] DRAW TEST START");

    tft.fillScreen(ST77XX_WHITE);

    tft.setCursor(20, 40);
    tft.print("Hub");

    tft.setCursor(20, 80);
    tft.print("VERSIN");

    tft.setCursor(20, 120);
    tft.print("TFT OK");

    Serial.println("[TFT] DRAW COMPLETE");
}