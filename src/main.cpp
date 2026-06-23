#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <XPT2046_Touchscreen.h> // Adicionado para o controle do Touch Screen

#include "communication/NetworkManager.h"
#include "communication/BleManager.h"
#include "core/SecurityManager.h"

#include "display/OLED/DisplayManager.h"
#include "display/TFT/TFTDisplayManager.h"

#include "secrets.h"

// ======================================================
// RFID
// ======================================================

#define RFID_SDA_PIN 4
#define RFID_RST_PIN 25

// ======================================================
// TOUCH SCREEN (XPT2046) - Configuração de Pinos Atualizada
// ======================================================

#define TOUCH_CS_PIN  15
#define TOUCH_IRQ_PIN 13

// ======================================================
// Botões
// ======================================================
#define BOTAO_1 26
#define BOTAO_2 27

// ======================================================
// Credenciais
// ======================================================

const char* apSsidConfig     = SECRET_AP_SSID;
const char* apPasswordConfig = SECRET_AP_PASS;

const char* supabaseUrl      = SECRET_SUPABASE_URL;
const char* supabaseAnonKey  = SECRET_SUPABASE_KEY;

// ======================================================
// Objetos globais
// ======================================================

TFTDisplayManager tftDisplay;
DisplayManager hubDisplay;

NetworkManager network;
SecurityManager security;
BleManager ble;

MFRC522 rfid(RFID_SDA_PIN, RFID_RST_PIN);
XPT2046_Touchscreen ts(TOUCH_CS_PIN, TOUCH_IRQ_PIN); // Instanciando o controlador Touch

WiFiClientSecure client;

unsigned long lastSupabaseUpdate = 0;
const unsigned long supabaseInterval = 10000;

// ======================================================

void printHashHex(const uint8_t* hash, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (hash[i] < 0x10)
            Serial.print("0");

        Serial.print(hash[i], HEX);
    }

    Serial.println();
}

int getWiFiQuality()
{
    long rssi = WiFi.RSSI();

    if (rssi <= -100)
        return 0;

    if (rssi >= -50)
        return 100;

    return 2 * (rssi + 100);
}

// ======================================================
void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=================================");
    Serial.println(" INICIANDO VERSIN HUB");
    Serial.println("=================================");

    // Configurar botões como entrada com pull-up
    pinMode(BOTAO_1, INPUT_PULLUP);
    pinMode(BOTAO_2, INPUT_PULLUP);

    // Configuração preventiva dos pinos CS para evitar colisão antes do SPI iniciar
    pinMode(5, OUTPUT); 
    digitalWrite(5, HIGH); // Desativa TFT CS (GPIO 5)
    
    pinMode(TOUCH_CS_PIN, OUTPUT);
    digitalWrite(TOUCH_CS_PIN, HIGH); // Desativa TOUCH CS (GPIO 15)

    pinMode(RFID_SDA_PIN, OUTPUT);
    digitalWrite(RFID_SDA_PIN, HIGH); // Desativa RFID CS (GPIO 4)

    // -------------------------------------------------
    // SPI (UMA ÚNICA VEZ) - Definição Geral de Barramento
    // SCK = 18 | MISO = 19 | MOSI = 23
    // -------------------------------------------------

    SPI.begin(18, 19, 23);

    // -------------------------------------------------
    // OLED
    // -------------------------------------------------

    hubDisplay.initDisplays();
    hubDisplay.updateSystemVisual(false, "STARTING");

    // -------------------------------------------------
    // TFT
    // -------------------------------------------------

    tftDisplay.init();
    

    // -------------------------------------------------
    // TOUCH SCREEN
    // -------------------------------------------------

    Serial.println("[TOUCH] Inicializando...");
    ts.begin();
    Serial.println("[TOUCH] Online");

    // -------------------------------------------------
    // RFID
    // -------------------------------------------------

    delay(50);

    Serial.println("[RFID] Inicializando...");

    rfid.PCD_Init();

    byte version = rfid.PCD_ReadRegister(rfid.VersionReg);

    if (version == 0x00 || version == 0xFF)
    {
        Serial.println("[RFID] Leitor nao respondeu.");
    }
    else
    {
        Serial.printf("[RFID] Online - Versao 0x%02X\n", version);
    }

    // -------------------------------------------------
    // Segurança
    // -------------------------------------------------

    security.initializeAntiTamper();

    // -------------------------------------------------
    // Portal WiFi
    // -------------------------------------------------

    network.startAccessPoint(
        apSsidConfig,
        apPasswordConfig
    );

    hubDisplay.updateSystemVisual(
        false,
        "PROV_AP"
    );

    // -------------------------------------------------
    // HTTPS
    // -------------------------------------------------

    client.setInsecure();

    Serial.println();
    Serial.println("Sistema pronto.");
    Serial.println();
}

// ======================================================
void loop()
{
    // Lógica dos Botões
    if (digitalRead(BOTAO_1) == LOW) {
        Serial.println("[BOTAO] Botao 1 (GPIO 26) pressionado!");
        delay(200); // Debounce simples para evitar multiplas leituras
    }

    if (digitalRead(BOTAO_2) == LOW) {
        Serial.println("[BOTAO] Botao 2 (GPIO 27) pressionado!");
        delay(200); // Debounce simples
    }

    // --------------------------------
    // TOUCH SCREEN
    // --------------------------------
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        Serial.printf("[TOUCH] Toque em: X=%d, Y=%d, Pressao=%d\n", p.x, p.y, p.z);
    }

    network.handlePortal();
    network.listenForResponses();

    // --------------------------------
    // RFID
    // --------------------------------

    if (
        rfid.PICC_IsNewCardPresent() &&
        rfid.PICC_ReadCardSerial()
    )
    {
        Serial.print("[RFID] UID: ");

        String uidStr;

        for (byte i = 0; i < rfid.uid.size; i++)
        {
            if (rfid.uid.uidByte[i] < 0x10)
                uidStr += "0";

            uidStr += String(
                rfid.uid.uidByte[i],
                HEX
            );
        }

        uidStr.toUpperCase();

        Serial.println(uidStr);

        hubDisplay.updateSystemVisual(
            WiFi.status() == WL_CONNECTED,
            "TAG_READ"
        );

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }

    // --------------------------------
    // WiFi e Supabase
    // --------------------------------

    unsigned long currentMillis = millis();

    if (WiFi.status() == WL_CONNECTED)
    {
        static bool displayOnline = false;

        if (!displayOnline)
        {
            hubDisplay.updateSystemVisual(
                true,
                "ONLINE"
            );

            displayOnline = true;
        }

        if (
            currentMillis - lastSupabaseUpdate >=
            supabaseInterval
        )
        {
            lastSupabaseUpdate = currentMillis;

            HTTPClient http;

            http.begin(
                client,
                supabaseUrl
            );

            http.addHeader(
                "Content-Type",
                "application/json"
            );

            http.addHeader(
                "apikey",
                supabaseAnonKey
            );

            http.addHeader(
                "Authorization",
                "Bearer " + String(supabaseAnonKey)
            );

            char jsonPayload[128];

            snprintf(
                jsonPayload,
                sizeof(jsonPayload),
                "{\"status\":\"online\",\"sinal\":%d}",
                getWiFiQuality()
            );

            int httpResponseCode =
                http.PATCH(
                    (uint8_t*)jsonPayload,
                    strlen(jsonPayload)
                );

            if (httpResponseCode > 0)
            {
                Serial.printf(
                    "[Supabase] PATCH OK: %d\n",
                    httpResponseCode
                );
            }
            else
            {
                Serial.printf(
                    "[Supabase] PATCH erro: %s\n",
                    http.errorToString(
                        httpResponseCode
                    ).c_str()
                );
            }

            http.end();
        }
    }
}