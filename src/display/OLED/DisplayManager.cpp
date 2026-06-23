#include "DisplayManager.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// --------------------------------------------------
// Construtor
// --------------------------------------------------

DisplayManager::DisplayManager()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
}

// --------------------------------------------------
// Verifica dispositivos conectados ao barramento I2C
// --------------------------------------------------

void DisplayManager::checkHardware() {

    Serial.println("Escaneando barramento I2C...");

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Hardware:");

    int linha = 12;

    for (uint8_t address = 1; address < 127; address++) {

        Wire.beginTransmission(address);

        if (Wire.endTransmission() == 0) {

            Serial.printf("Encontrado: 0x%02X\n", address);

            display.setCursor(0, linha);
            display.print("I2C: 0x");
            display.println(address, HEX);
            display.display();

            linha += 10;

            // Evita escrever fora da tela
            if (linha > 56) {
                delay(1000);

                display.clearDisplay();
                display.setCursor(0, 0);
                display.println("Hardware:");

                linha = 12;
            }
        }
    }

    delay(1000);
}

// --------------------------------------------------
// Inicialização do OLED
// --------------------------------------------------

void DisplayManager::initDisplays() {

    // Inicializa I2C
    Wire.begin(21, 22);

    // Inicializa OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Falha ao inicializar o OLED");
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    // Tela inicial
    display.setCursor(0, 10);
    display.println("VERSIN OS v1.0");
    display.println("Checando Hardware...");
    display.display();

    delay(500);

    // Verifica hardware conectado
    checkHardware();

    // Animação de inicialização
    String targetText = "VERSIN HUB";
    String currentText = "          ";

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

    // Tela final
    display.clearDisplay();
    display.setCursor(34, 28);
    display.print(targetText);
    display.display();

    delay(1000);
}

// --------------------------------------------------
// Atualização visual do sistema
// --------------------------------------------------

void DisplayManager::updateSystemVisual(bool isWifiConnected, String operation) {

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("VERSIN HUB");

    display.setCursor(0, 16);
    display.print("WiFi: ");

    if (isWifiConnected) {
        display.println("ONLINE");
    } else {
        display.println("OFFLINE");
    }

    display.setCursor(0, 32);
    display.print("Status:");

    display.setCursor(0, 44);
    display.println(operation);

    display.display();
}