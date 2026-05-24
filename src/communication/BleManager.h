// src/communication/BleManager.h
#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// EN: Unique UUIDs for the Versin Ecosystem (Generated to prevent network interface collisions)
// PT: UUIDs únicos para o ecossistema Versin (Gerados para evitar conflitos de interface de rede)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class BleManager : public BLECharacteristicCallbacks {
public:
    // EN: Initializes the Bluetooth Low Energy server layer and begins broadcasting the device identity
    // PT: Inicializa a camada do servidor Bluetooth Low Energy e inicia a transmissão da identidade do dispositivo
    void initBle();

    // EN: Refreshes the internal data registry containing the runtime encryption public keys
    // PT: Atualiza o registro interno de dados contendo as chaves públicas de criptografia em runtime
    void updateBleKeyPayload();
    
    // EN: Callback triggered synchronously when the mobile application writes data to the BLE descriptor
    // PT: Callback executado sincronicamente quando o aplicativo móvel escreve dados no descritor BLE
    void onWrite(BLECharacteristic *pCharacteristic) override;
};

#endif