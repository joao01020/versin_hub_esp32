#include "DisplayManager.h" 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#define COLOR_PURPLE 0x780F
#define COLOR_CYAN   0x07FF // Cor de sucesso para o olho sistêmico

DisplayManager::DisplayManager() 
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), tft(TFT_CS, TFT_DC, TFT_RST) {}

void DisplayManager::initDisplays() {
    // 1. Inicializa o barramento I2C para o OLED
    Wire.begin(21, 22); 
    
    if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
    }

    // 2. Inicializa o display TFT via SPI
    tft.init(240, 240, SPI_MODE2); 
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK); 

    // Renderiza a base do olho sistêmico
    drawSystemEye(50);

    // 3. Configurações e correção avançada para o Módulo MicroSD Azul (Level Shifter)
    Serial.println("[SD] Inicializando leitor de cartão SD...");
    
    // Força o pino CS do TFT a ficar em HIGH (desativado) para isolar o barramento SPI
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    // Configura o pino CS do SD e aplica um pulso de reinicialização elétrica no módulo azul
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH); 
    delay(20);
    digitalWrite(SD_CS, LOW);
    delay(50);
    digitalWrite(SD_CS, HIGH);

    // Mensagem inicial de diagnóstico no OLED
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("VERSIN OS v1.0");
    display.println("Checando Hardware...");
    display.display();
    delay(500);

    // Força o reset completo das definições de hardware do barramento SPI nativo do ESP32
    SPI.end();
    SPI.begin(18, 19, 23, 13); // SCK=18, MISO=19, MOSI=23, SS=13

    // Inicializa o SD com clock seguro de 4MHz para estabilizar a comutação do chip buffer da placa azul
    if (!SD.begin(SD_CS, SPI, 4000000)) {
        Serial.println("[SD] ERRO: Falha ao reconhecer o cartão SD (Módulo azul travou a linha MISO).");
        
        // Emite o feedback visual de falha no OLED
        display.println("\nSD: NAO DETECTADO!");
        display.display();
        
        // Emite o feedback visual de falha no TFT (Texto vermelho de alerta na base)
        tft.setCursor(40, 190);
        tft.setTextColor(ST77XX_RED);
        tft.setTextSize(2);
        tft.print("SD CARD ERROR");
        delay(2000); 
    } else {
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("[SD] SUCESSO! Armazenamento: %llu MB\n", cardSize);

        // Emite o feedback visual de sucesso no OLED
        display.println("\nSD: ONLINE");
        display.printf("Capac: %llu MB\n", cardSize);
        display.display();

        // Emite o feedback visual de sucesso no TFT (Texto verde de sucesso na base)
        tft.setCursor(50, 190);
        tft.setTextColor(ST77XX_GREEN);
        tft.setTextSize(2);
        tft.print("SD CARD READY");
        delay(1500);
    }

    // Libera os Chip Selects colocando-os no estado padrão pós-boot (TFT escuta, SD aguarda)
    digitalWrite(SD_CS, HIGH);
    digitalWrite(TFT_CS, LOW);

    // Restaura a cor de texto padrão do TFT para operações normais
    tft.setTextColor(ST77XX_WHITE);

    // 4. Efeito estético Scramble de inicialização do painel OLED
    String targetText = "VERSIN HUB";
    String currentText = "          "; 
    char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$*!?";
    int charsetLength = sizeof(charset) - 1;

    for (size_t i = 0; i < targetText.length(); i++) {
        if (targetText[i] == ' ') {
            currentText[i] = ' ';
            continue;
        }
        for (int scramble = 0; scramble < 5; scramble++) {
            display.clearDisplay();
            display.setTextSize(1);
            display.setCursor(34, 28);
            display.print(currentText);
            display.display();
            delay(25); 
            yield(); 
        }
        currentText[i] = targetText[i];
    }

    // Fixa o nome final pós-scramble por 1 segundo antes de liberar a tela
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(34, 28);
    display.print(targetText);
    display.display();
    delay(1000);
    yield();
}

void DisplayManager::drawSystemEye(int irisRadius) {
    tft.fillCircle(120, 120, 80, ST77XX_BLACK); 
    tft.drawCircle(120, 120, 80, COLOR_PURPLE); 
    tft.fillCircle(120, 120, irisRadius, ST77XX_BLUE);    
    tft.fillCircle(120, 120, 15, ST77XX_WHITE);   
}

void DisplayManager::drawVersinLogo(int x, int y) {
    display.fillTriangle(x, y,  x - 5, y - 15,  x, y - 35, SSD1306_WHITE);
    display.fillTriangle(x, y,  x + 5, y - 15,  x, y - 35, SSD1306_WHITE);

    display.fillTriangle(x, y,  x - 8, y - 10,   x - 14, y - 26, SSD1306_WHITE);
    display.fillTriangle(x, y,  x + 8, y - 10,   x + 14, y - 26, SSD1306_WHITE);

    display.fillTriangle(x, y,  x - 10, y - 7,   x - 22, y - 18, SSD1306_WHITE);
    display.fillTriangle(x, y,  x + 10, y - 7,   x + 22, y - 18, SSD1306_WHITE);
}

void DisplayManager::drawContractIcon(int x, int y) {
    display.drawRoundRect(x, y, 20, 26, 2, SSD1306_WHITE); 
    display.drawLine(x + 4, y + 6, x + 16, y + 6, SSD1306_WHITE); 
    display.drawLine(x + 4, y + 12, x + 16, y + 12, SSD1306_WHITE); 
    display.drawLine(x + 4, y + 18, x + 12, y + 18, SSD1306_WHITE); 
}

// --- FUNÇÕES GEOMÉTRICAS DE SUPORTE PARA O CADEADO ANITAMPER ---
void DisplayManager::drawLockClosed(int x, int y) {
    // Base do Cadeado (Corpo)
    display.fillRoundRect(x, y, 24, 20, 3, SSD1306_WHITE);
    display.fillRect(x + 10, y + 6, 4, 5, SSD1306_BLACK); // Fechadura
    display.fillCircle(x + 12, y + 13, 3, SSD1306_BLACK);
    
    // Arco Fechado
    display.drawCircleHelper(x + 12, y, 8, 1, SSD1306_WHITE); // Canto superior esquerdo
    display.drawCircleHelper(x + 12, y, 8, 2, SSD1306_WHITE); // Canto superior direito
    display.drawLine(x + 4, y, x + 4, y + 2, SSD1306_WHITE);  // Haste esquerda encaixada
    display.drawLine(x + 20, y, x + 20, y + 2, SSD1306_WHITE); // Haste direita encaixada
}

void DisplayManager::drawLockOpen(int x, int y) {
    // Base do Cadeado (Corpo)
    display.fillRoundRect(x, y, 24, 20, 3, SSD1306_WHITE);
    display.fillRect(x + 10, y + 6, 4, 5, SSD1306_BLACK); // Fechadura
    display.fillCircle(x + 12, y + 13, 3, SSD1306_BLACK);
    
    // Arco Aberto e Deslocado (Haste direita sai e sobe)
    display.drawCircleHelper(x + 12, y - 6, 8, 1, SSD1306_WHITE); 
    display.drawCircleHelper(x + 12, y - 6, 8, 2, SSD1306_WHITE); 
    display.drawLine(x + 4, y - 6, x + 4, y + 2, SSD1306_WHITE);  // Haste esquerda presa girada
    display.drawLine(x + 20, y - 6, x + 20, y - 3, SSD1306_WHITE); // Haste direita voando (aberta)
}

void DisplayManager::updateSystemVisual(bool isWifiConnected, String operation) {
    display.clearDisplay();
    
    if (operation == "CONTRATO") {
        drawContractIcon(54, 19); 
        display.display();
    } 
    else if (operation == "TAG_READ") {
        // --- EVENTO DE DESBLOQUEIO POR APROXIMAÇÃO (NFC) ---
        
        // Passo 1: Mostra o Cadeado Fechado e Alerta no TFT
        display.clearDisplay();
        drawLockClosed(52, 28);
        display.setTextSize(1);
        display.setCursor(24, 6);
        display.print("AUTENTICANDO...");
        display.display();
        
        // Resposta visual instantânea no olho redondo (Abre a íris em choque)
        tft.fillCircle(120, 120, 80, ST77XX_BLACK);
        tft.drawCircle(120, 120, 80, COLOR_CYAN);
        tft.fillCircle(120, 120, 65, COLOR_CYAN); // Íris expande
        tft.fillCircle(120, 120, 15, ST77XX_WHITE);
        delay(350);

        // Passo 2: Transição para Cadeado Aberto e Olho Verde/Ciano de Sucesso
        display.clearDisplay();
        drawLockOpen(52, 28);
        display.setCursor(28, 6);
        display.print("ACESSO VERIFICADO");
        display.display();
        
        // Contração da íris indicando foco/sucesso
        tft.fillCircle(120, 120, 80, ST77XX_BLACK);
        tft.drawCircle(120, 120, 80, COLOR_CYAN);
        tft.fillCircle(120, 120, 35, COLOR_CYAN); // Íris contrai focando
        tft.fillCircle(120, 120, 15, ST77XX_WHITE);
        
        delay(1200); // Mantém o status de liberado visível
        return; // Sai da função para não sobrescrever com o logo padrão imediatamente
    }
    else {
        drawVersinLogo(64, 48);
        display.display();
    }

    // --- ANIMAÇÃO DE PULSAÇÃO PADRÃO DO OLHO (STANDBY) ---
    if (pulseGrowing) {
        eyePulseRadius += 2;
        if (eyePulseRadius >= 56) pulseGrowing = false;
    } else {
        eyePulseRadius -= 2;
        if (eyePulseRadius <= 44) pulseGrowing = true;
    }

    if (operation == "CONTRATO") {
        tft.fillCircle(120, 120, 80, ST77XX_BLACK); 
        tft.drawCircle(120, 120, 80, COLOR_PURPLE);
        tft.fillCircle(120, 120, eyePulseRadius, COLOR_PURPLE); 
        tft.fillCircle(120, 120, 15, ST77XX_WHITE);
    } else { 
        tft.fillCircle(120, 120, 80, ST77XX_BLACK);
        tft.drawCircle(120, 120, 80, COLOR_PURPLE);
        tft.fillCircle(120, 120, eyePulseRadius, ST77XX_BLUE);    
        tft.fillCircle(120, 120, 15, ST77XX_WHITE);
    }
}