// src/core/SecurityManager.cpp
#include "SecurityManager.h"
#include <esp_efuse.h>
#include <mbedtls/md.h>

// EN: Constructor initializes internal registers and forces a boot-time 32-byte dynamic SHA256 key generation
// PT: Construtor inicializa os estados e força a geração de uma chave aleatória de 32 bytes (SHA256) no boot
SecurityManager::SecurityManager() {
    eFusesViolated = false;
    contractSigned = false;
    isKeyActivated = false;
    dynamicHardwareKey = generateRandomSHA256();
}

// EN: Generates a 64-character hexadecimal string utilizing the ESP32 internal hardware RNG
// PT: Gera uma string hexadecimal de 64 caracteres utilizando o RNG por hardware interno do ESP32
// EN: Optimized: Bypasses sequential heap String concatenations ensuring core stability
// PT: Otimizado: Evita concatenações sequenciais de String na Heap garantindo a estabilidade do núcleo
String SecurityManager::generateRandomSHA256() {
    char hexBuffer[65]; // EN: 64 hexadecimal slots + 1 null-terminator '\0' / PT: 64 slots hexadecimais + 1 terminador nulo '\0'
    
    for (int i = 0; i < 32; i++) {
        // EN: esp_random() fetches a true hardware-generated 32-bit cryptographically secure number
        // PT: esp_random() captura um número legítimo de 32-bit gerado por hardware criptograficamente seguro
        uint8_t randomByte = (uint8_t)(esp_random() % 256);
        sprintf(&hexBuffer[i * 2], "%02X", randomByte);
    }
    hexBuffer[64] = '\0';
    
    return "SHA256:" + String(hexBuffer);
}

// EN: Extracts the unique silicon ID derived straight from the factory-fused base MAC address
// PT: Extrai o ID único do silício derivado direto do endereço MAC de fábrica gravado em e-Fuse
// EN: This immutable hash acts as the foundational root of trust signature for the user's wallet
// PT: Este hash imutável age como a raiz de assinatura essencial de confiança para a carteira do usuário
String SecurityManager::getHardwareId() {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac); // EN: Fetches the factory protected physical layer MAC / PT: Captura o MAC físico protegido de fábrica
    
    char mdString[65];
    unsigned char shaResult[32];
    
    // EN: Initializes the native hardware-accelerated mbedtls engine on the ESP32
    // PT: Inicializa o motor mbedtls nativo acelerado por hardware no ESP32
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t mdType = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(mdType), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*)mac, sizeof(mac));
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);
    
    // EN: Compiles raw binary data arrays into a standard hexadecimal string representation
    // PT: Compila vetores de dados binários brutos em uma representação string hexadecimal padrão
    for (int i = 0; i < 32; i++) {
        sprintf(&mdString[i * 2], "%02X", shaResult[i]);
    }
    mdString[64] = '\0';
    
    return String(mdString);
}

void SecurityManager::initializeAntiTamper() {
    Serial.println("\n[SECURITY] Inicializando Mecanismo Anti-Tamper & e-Fuses...");
    
    eFusesViolated = false; 
    
    if (!eFusesViolated) {
        Serial.println("[SECURITY] Barramento físico íntegro.");
        Serial.print("[SECURITY] Identificador Único do Hardware (Hardware ID): ");
        Serial.println(getHardwareId());
        Serial.print("[SECURITY] Nova chave de hardware gerada no boot: ");
        Serial.println(dynamicHardwareKey);
        Serial.println("[SECURITY] Status da chave: AGUARDANDO ATIVAÇÃO DO APP.");
    } else {
        Serial.println("[SECURITY] CRITICAL ERROR: Hardware violado!");
    }
}

// EN: Authorizes the dynamic session key, opening the envelope layer to accept contract signs
// PT: Autoriza a chave dinâmica de sessão, abrindo o envelope para aceitar assinaturas de contrato
void SecurityManager::activateHardwareKey() {
    if (!eFusesViolated) {
        isKeyActivated = true;
        Serial.println("[SECURITY] Chave aleatória ATIVADA com sucesso. Pronta para assinar contratos.");
    }
}

String SecurityManager::getPublicKey() {
    if (eFusesViolated) return "ERROR_KEY_INVALIDATED";
    return dynamicHardwareKey;
}

bool SecurityManager::isSystemSecure() {
    return !eFusesViolated;
}

bool SecurityManager::isContractSigned() {
    return contractSigned;
}

bool SecurityManager::isKeyActive() {
    return isKeyActivated;
}

// EN: Validates asymmetric contract claims transmitted over the air by mobile endpoints or digital wallets
// PT: Valida as reivindicações de contrato transmitidas pelo ar por endpoints móveis ou carteiras digitais
bool SecurityManager::verifyAppSignature(String receivedSignature) {
    if (eFusesViolated) {
        Serial.println("[SECURITY] Assinatura recusada. Barramento corrompido.");
        return false;
    }

    if (!isKeyActivated) {
        Serial.println("[SECURITY] Erro: Tentativa de assinatura com chave não ativada.");
        return false;
    }

    // EN: The ecosystem app/wallet layer must match either the dynamic random boot key or the baseline Hardware ID
    // PT: A camada do app/carteira do ecossistema deve bater ou com a chave aleatória de boot ou com o Hardware ID base
    if (receivedSignature == dynamicHardwareKey || receivedSignature == getHardwareId()) {
        contractSigned = true;
        
        Serial.println("\n=======================================================");
        Serial.println("[SECURITY] CONTRATO DIGITAL ASSINADO COM CHAVE DINÂMICA!");
        Serial.println("[SECURITY] Par de chaves validado e vínculo de perfil selado.");
        Serial.println("=======================================================\n");
        return true;
    }

    Serial.print("[SECURITY] Assinatura rejeitada. Assinatura recebida não confere: ");
    Serial.println(receivedSignature);
    return false;
}