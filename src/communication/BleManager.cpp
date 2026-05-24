// src/communication/BleManager.cpp
#include "BleManager.h"
#include "core/SecurityManager.h"

// EN: Links with the global cryptographic engine instance initialized inside main.cpp
// PT: Linka com o gerenciador criptográfico global do main.cpp
extern SecurityManager security; 
BLECharacteristic *pCharacteristicGlobal;

void BleManager::initBle() {
    Serial.println("[BLE] Inicializando Servidor Bluetooth Low Energy...");
    
    // EN: Initializes the interface with the global chassi identification string
    // PT: Inicializa o dispositivo com o nome do hardware
    BLEDevice::init("Versin_Chassi_Pro");
    BLEServer *pServer = BLEDevice::createServer();
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    // EN: Spawns the characteristics mapping Read and Write descriptors
    // PT: Cria a característica com permissão de Leitura e Escrita
    pCharacteristicGlobal = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

    // EN: Binds the custom execution callbacks mapped within this file context
    // PT: Vincula os callbacks de escrita deste arquivo
    pCharacteristicGlobal->setCallbacks(this);
    
    // EN: Asserts the initial memory state with the current hardware signature parameters
    // PT: Inicializa o valor da característica com o status atual do hardware
    updateBleKeyPayload();
    
    pService->start();
    
    // EN: Starts airwave transmission (Advertising) allowing companion smartphone discovery
    // PT: Inicia a transmissão do sinal (Advertising) para o celular encontrar
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    
    Serial.println("[BLE] Bluetooth ativado e visível como 'Versin_Chassi_Pro'");
}

// EN: Refreshes the transmission payload structure pulled by the mobile terminal over-the-air
// PT: Atualiza o payload que o celular lê ao puxar os dados do Bluetooth
void BleManager::updateBleKeyPayload() {
    String state = security.isContractSigned() ? "SECURED|" : 
                   (security.isKeyActive() ? "ACTIVE|" : "WAITING|");
    String payload = "VERSIN_BLE|" + state + security.getPublicKey();
    pCharacteristicGlobal->setValue(payload.c_str());
}

// EN: Synchronously parses and executes incoming stream commands received from the active BLE interface
// PT: Processa os comandos recebidos diretamente do app via Bluetooth
void BleManager::onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
        String command = String(value.c_str());
        Serial.print("[BLE] Comando recebido via Bluetooth: ");
        Serial.println(command);
        
        // --- COMMAND 1: ASYNCHRONOUS DESCRIPTOR KEY ACTIVATION ---
        // --- COMANDO 1: ATIVAÇÃO DA CHAVE ALEATÓRIA ---
        if (command == "VERSIN_APP_ACTIVATE") {
            security.activateHardwareKey();
            updateBleKeyPayload(); // EN: Immediately push fresh metadata state to BLE register / PT: Atualiza o novo status no BLE imediatamente
            pCharacteristicGlobal->setValue("VERSIN_CHASSI_STATUS:ACTIVATED");
        } 
        
        // --- COMMAND 2: DIGITAL CONTRACT CRYPTO-SIGNATURE EVALUATION ---
        // --- COMANDO 2: ENVIO DA ASSINATURA DO CONTRATO DIGITAL ---
        else if (command.startsWith("VERSIN_APP_ACCEPT:")) {
            String receivedHash = command.substring(18);
            
            if (security.verifyAppSignature(receivedHash)) {
                updateBleKeyPayload();
                pCharacteristicGlobal->setValue("VERSIN_CHASSI_CONNECTED:SECURED");
            } else {
                pCharacteristicGlobal->setValue("VERSIN_CHASSI_ERROR:BAD_SIGNATURE");
            }
        }
    }
}