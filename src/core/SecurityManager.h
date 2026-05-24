// src/core/SecurityManager.h
#ifndef SECURITY_MANAGER_H
#define SECURITY_MANAGER_H

#include <Arduino.h>

class SecurityManager {
private:
    // EN: Dynamic SHA256 session key (32 bytes in HEX layout) compiled during hardware boot
    // PT: Chave SHA256 dinâmica de sessão (32 bytes em layout HEX) compilada no boot do hardware
    String dynamicHardwareKey; 
    
    bool eFusesViolated;
    bool contractSigned;
    
    // EN: State register tracking if the dynamic token has been authorized by the mobile client
    // PT: Registro de estado controlando se a chave aleatória já foi ativada pelo aplicativo móvel
    bool isKeyActivated;       

    // EN: Cryptographic hardware-seeded generator parsing raw random bytes into an isolated hex stream
    // PT: Gerador criptográfico alimentado por hardware que converte bytes aleatórios brutos em uma string hex isolada
    String generateRandomSHA256(); 

public:
    SecurityManager();
    
    // EN: Verifies low-level physical continuity traces and initializes hardware security metadata logs
    // PT: Verifica traços de continuidade física de baixo nível e inicializa os logs de metadados de segurança do hardware
    void initializeAntiTamper();
    
    // EN: Validates the incoming cryptographic signature against the dynamic session token or silicon-fused root hashes
    // PT: Valida a assinatura criptográfica recebida contra o token dinâmico de sessão ou hashes raiz fundidos no silício
    bool verifyAppSignature(String receivedSignature);
    
    // EN: Arms the memory register unlocking execution paths to evaluate signature bindings
    // PT: Ativa o registro de memória liberando os caminhos de execução para avaliar vinculações de assinatura
    void activateHardwareKey(); 
    
    // EN: Extracts the immutable, factory-protected unique silicon identifier (MAC-derived SHA256)
    // PT: Extrai o identificador único e imutável do silício protegido de fábrica (SHA256 derivado do MAC)
    String getHardwareId(); 
    
    String getPublicKey();
    bool isSystemSecure();
    bool isContractSigned();
    bool isKeyActive();
};

#endif