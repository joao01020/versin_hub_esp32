#include "TFTDisplayManager.h"
#include <Arduino.h>

#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  33

// Inicializa variáveis no construtor
TFTDisplayManager::TFTDisplayManager()
    : tft(TFT_CS, TFT_DC, TFT_RST), ts(TOUCH_CS_PIN),
      wasTouched(false), startTouchY(0), lastTouchY(0)
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
    tft.setRotation(1); // Modo paisagem (320x240)
    
    // --- TELA DE INICIALIZAÇÃO (ANIMAÇÃO) ---
    tft.fillScreen(ST77XX_WHITE);
    tft.setTextSize(3);
    
    // O truque para TFT: passar a cor de fundo (Branco) como segundo parâmetro
    // Assim ele sobrescreve os espaços vazios sem precisar de "clearDisplay"
    tft.setTextColor(ST77XX_BLACK, ST77XX_WHITE); 

    String targetText = "VERSIN";
    String currentText = "          "; // 10 espaços (mesmo tamanho do targetText)

    // Calcula a posição centralizada com base no tamanho do targetText
    int textLength = targetText.length();
    int xPos = (320 - (textLength * 18)) / 2;
    int yPos = (240 - 24) / 2;

    // Loop da animação letra por letra
    for (size_t i = 0; i < targetText.length(); i++) {

        if (targetText[i] == ' ') {
            currentText[i] = ' ';
            continue;
        }

        for (int scramble = 0; scramble < 5; scramble++) {
            tft.setCursor(xPos, yPos);
            tft.print(currentText);
            
            delay(25);
            yield();
        }

        currentText[i] = targetText[i];
    }
    
    // Garante a impressão da última letra para completar a palavra
    tft.setCursor(xPos, yPos);
    tft.print(currentText);
    
    // Aguarda 2 segundos para o usuário ler o texto formado
    delay(2000); 
    
    // --- TELA PRINCIPAL ---
    drawScreen("HUB");
    
    Serial.println("[TFT] READY");
}

void TFTDisplayManager::checkTouch()
{
    // Verifica se a tela está sendo tocada
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        
        // Mapeia o valor Y cru (0 a 4095 do XPT2046) para os pixels da tela
        int16_t screenY = map(p.y, 200, 3700, 0, 240); 
        
        if (!wasTouched) {
            // Salva a posição Y no momento exato em que o dedo encostou na tela
            startTouchY = screenY;
            wasTouched = true;
        }
        
        // Atualiza a posição enquanto o dedo arrasta
        lastTouchY = screenY;
        
    } else {
        // Entra aqui quando o dedo SOLTA a tela
        if (wasTouched) {
            // Como não há mais menu, apenas registramos o clique
            Serial.println("Tela tocada!");
            
            // Reseta a flag para o próximo toque
            wasTouched = false;
        }
    }
}

// Função genérica para exibir texto centralizado
void TFTDisplayManager::drawScreen(const char* text)
{
    tft.fillScreen(ST77XX_WHITE);
    // Aqui voltamos ao padrão sem cor de fundo, pois demos um fillScreen antes
    tft.setTextColor(ST77XX_BLACK);
    tft.setTextSize(3);

    // Lógica simples para tentar centralizar o texto horizontalmente e verticalmente.
    // Com setTextSize(3), a largura de cada caractere é aproximadamente 18px e a altura 24px.
    int textLength = strlen(text);
    int xPos = (320 - (textLength * 18)) / 2;
    int yPos = (240 - 24) / 2;
    
    // Define a posição do cursor com base no cálculo e desenha o texto
    tft.setCursor(xPos, yPos);
    tft.print(text);
}