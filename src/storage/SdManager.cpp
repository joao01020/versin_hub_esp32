// storage/SdManager.cpp
#include "SdManager.h"

// Nota: Como a maior parte das funções leves e de inicialização 
// foi estruturada diretamente inline no arquivo .h para otimizar o ganho de escopo 
// do compilador do ESP32, o arquivo .cpp serve para consolidar a estrutura 
// e permitir futuras expansões de funções pesadas (como escrita de arquivos de log).

// Caso você precise implementar funções complexas de arquivos no futuro,
// a estrutura base do escopo da classe fica selada de forma limpa aqui:

/*
void SdManager::escreverLog(const char* caminho, const char* mensagem) {
    if (!_isInitialized) return;
    File arquivo = SD.open(caminho, FILE_WRITE);
    if (arquivo) {
        arquivo.println(mensagem);
        arquivo.close();
    }
}
*/