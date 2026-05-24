// storage/SdManager.h
#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

class SdManager {
private:
    uint8_t _csPin;
    bool _isInitialized;

public:
    // Construtor recebendo o pino de Chip Select (na sua bancada, o G13)
    SdManager(uint8_t csPin) : _csPin(csPin), _isInitialized(false) {}

    /**
     * @brief Inicializa o Cartão SD em modo compartilhado usando o barramento SPI da bancada.
     * @param spiBus Ponteiro para o barramento SPI ativo (padrão: &SPI)
     * @return true se o cartão foi reconhecido com sucesso, false caso contrário.
     */
    bool begin(SPIClass &spiBus = SPI) {
        // Garante que o pino de seleção comece em nível alto para não travar o barramento
        pinMode(_csPin, OUTPUT);
        digitalWrite(_csPin, HIGH);
        delay(10);

        // Força a inicialização nativa da biblioteca da biblioteca SD passando o barramento compartilhado
        if (!SD.begin(_csPin, spiBus)) {
            _isInitialized = false;
            return false;
        }

        _isInitialized = true;
        return true;
    }

    /**
     * @brief Retorna se o cartão SD está pronto para operações de leitura e escrita.
     */
    bool isReady() const {
        return _isInitialized;
    }

    /**
     * @brief Retorna o tamanho total do Cartão SD em Megabytes (MB).
     */
    uint64_t getCardSizeMB() {
        if (!_isInitialized) return 0;
        // Retorna o cálculo exato dividindo os bytes para MB
        return SD.cardSize() / (1024 * 1024);
    }

    /**
     * @brief Retorna o total de espaço utilizado no cartão em Megabytes (MB).
     */
    uint64_t getUsedBytesMB() {
        if (!_isInitialized) return 0;
        return SD.usedBytes() / (1024 * 1024);
    }
};

#endif // SD_MANAGER_H