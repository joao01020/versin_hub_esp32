// src/main.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SD.h> 
#include <SPI.h>        
#include <MFRC522.h>    

#include "communication/NetworkManager.h"
#include "core/SecurityManager.h"
#include "communication/BleManager.h" 
#include "display/DisplayManager.h"     
#include "storage/SdManager.h"

// Chama o arquivo de credenciais seguras locais
#include "secrets.h" 

// CONFIGURAÇÃO DE PINOS DO BARRAMENTO SPI COMPARTILHADO ALINHADO COM A BANCADA
#define SD_CS_PIN     13   
#define RFID_SDA_PIN   4   
#define RFID_RST_PIN  27   

// Puxa as definições do arquivo secrets.h
const char* apSsidConfig = SECRET_AP_SSID;
const char* apPasswordConfig = SECRET_AP_PASS; 

const char* supabaseUrl = SECRET_SUPABASE_URL;
const char* supabaseAnonKey = SECRET_SUPABASE_KEY;

NetworkManager network;
SecurityManager security;
BleManager ble; 
DisplayManager hubDisplay;
SdManager sdOpts(SD_CS_PIN);
MFRC522 rfid(RFID_SDA_PIN, RFID_RST_PIN); 

WiFiClientSecure client;

unsigned long lastSupabaseUpdate = 0;
const unsigned long supabaseInterval = 10000; 

void printHashHex(const uint8_t* hash, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (hash[i] < 0x10) Serial.print("0");
    Serial.print(hash[i], HEX);
  }
  Serial.println();
}

int getWiFiQuality() {
  long rssi = WiFi.RSSI();
  if (rssi <= -100) return 0;
  if (rssi >= -50) return 100;
  return 2 * (rssi + 100);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- INICIANDO VERSIN HUB BOOT ---");
  
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH); 

  // 1. Inicializa o Display primeiro para podermos mostrar o progresso nele
  hubDisplay.initDisplays(); 
  hubDisplay.updateSystemVisual(false, "STARTING"); // Tela de "Iniciando..."

  // 2. Configuração de Hardware e Isolamento dos Pinos de Seleção (CS)
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(RFID_SDA_PIN, OUTPUT);
  
  // Coloca ambos em HIGH para garantir que ninguém tente falar antes da hora
  digitalWrite(SD_CS_PIN, HIGH);
  digitalWrite(RFID_SDA_PIN, HIGH);
  delay(50); // Pequena pausa para estabilização elétrica dos chips

  // 3. Inicializa o barramento SPI compartilhando as fileiras centrais da protoboard
  SPI.begin(18, 19, 23); 

  // 4. Inicialização Secura do Cartão SD usando o Barramento Ativo
  Serial.print("[SD] Tentando inicializar o armazenamento...\n"); 
  
  if (!SD.begin(SD_CS_PIN, SPI)) { 
    Serial.println("[SD] ERRO: Falha física ou cartão ausente!");
    hubDisplay.updateSystemVisual(false, "SD_ERROR"); // Mostra erro do SD na tela
  } else {
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("[SD] SUCESSO! Armazenamento: %llu MB\n", cardSize);
  }

  // 5. Inicialização do RFID
  Serial.println("[RFID] Inicializando leitor NFC...");
  rfid.PCD_Init();
  
  // Teste rápido para validar se o chip do NFC responde no barramento
  byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
  if (v == 0x00 || v == 0xFF) {
    Serial.println("[RFID] AVISO: Leitor NFC não respondeu no barramento compartilhado.");
  } else {
    Serial.printf("[RFID] SUCESSO! MFRC522 Online. Versão do Chip: 0x%02X\n", v);
  }

  security.initializeAntiTamper();
  
  // 6. Configuração e ativação do Portal de Acesso
  network.startAccessPoint(apSsidConfig, apPasswordConfig);
  hubDisplay.updateSystemVisual(false, "PROV_AP");
  
  client.setInsecure(); 
  digitalWrite(2, LOW);
  Serial.println("--- FIM DO SETUP: SISTEMA PRONTO ---\n");
}

void loop() {
  network.handlePortal();
  network.listenForResponses();

  // --- ESCUTA FORÇADA DO NFC ---
  // Inicializa o chip a cada ciclo do loop para reviver a antena caso ela durma
  rfid.PCD_Init();
  delay(5); 

  // Leitura em background do RFID usando controle automático de barramento
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("\n[RFID] Tag Detectada! UID: ");
    String uidStr = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uidStr += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      uidStr += String(rfid.uid.uidByte[i], HEX);
    }
    
    // Tratamento seguro da String para maiúsculo
    uidStr.toUpperCase();
    Serial.println(uidStr);
    
    // Dispara a atualização visual instantânea do evento de leitura
    hubDisplay.updateSystemVisual(WiFi.status() == WL_CONNECTED, "TAG_READ");
    
    // Finaliza a leitura e desativa a criptografia para liberar a fila
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  unsigned long currentMillis = millis();
  
  if (WiFi.status() == WL_CONNECTED) {
    
    static bool displaySetOnline = false;
    if (!displaySetOnline) {
      hubDisplay.updateSystemVisual(true, "ONLINE");
      displaySetOnline = true;
    }

    if (currentMillis - lastSupabaseUpdate >= supabaseInterval) {
      lastSupabaseUpdate = currentMillis;

      HTTPClient http;
      http.begin(client, supabaseUrl);
      
      http.addHeader("Content-Type", "application/json");
      http.addHeader("apikey", supabaseAnonKey);
      http.addHeader("Authorization", "Bearer " + String(supabaseAnonKey));

      char jsonPayload[128];
      snprintf(jsonPayload, sizeof(jsonPayload), 
               "{\"status\":\"online\",\"sinal\":%d}", 
               getWiFiQuality());
      
      int httpResponseCode = http.PATCH((uint8_t*)jsonPayload, strlen(jsonPayload));
      
      if (httpResponseCode > 0) {
        Serial.printf("[Supabase] Telemetria updated! Code: %d\n", httpResponseCode);
      } else {
        Serial.printf("[Supabase] Falha ao enviar PATCH: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      
      http.end();
    }
  }
}