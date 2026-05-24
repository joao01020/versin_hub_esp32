#include "NetworkManager.h"
#include "core/SecurityManager.h"

// EN: Links with the global security engine instance initialized inside main.cpp
// PT: Linka com a instância global do gerenciador de segurança do main.cpp
extern SecurityManager security;

// EN: Default DNS port to intercept and redirect smartphone configuration requests
// PT: Porta padrão DNS para capturar e redirecionar as requisições do smartphone
const byte DNS_PORT = 53;

// ============================================================================
// --- CUSTOM REQUEST HANDLER (ANTI-CORE LOG ERROR SHIELD) ---
// ============================================================================

// EN: Custom Request Handler to bypass the native WebServer error logging entirely
// PT: Handler de Requisição customizado para ignorar completamente o log de erro nativo do WebServer
class CaptiveRequestHandler : public RequestHandler {
  public:
    NetworkManager* _netManager;
    
    CaptiveRequestHandler(NetworkManager* netManager) {
        _netManager = netManager;
    }

    // EN: Explicitly tells the ESP32 that ANY path requested is valid (prevents [E][WebServer.cpp:638])
    // PT: Diz explicitamente ao ESP32 que QUALQUER caminho requisitado é válido (evita o print de erro)
    bool canHandle(HTTPMethod method, String uri) override {
        return true; 
    }

    // EN: Safely routes the captive portal payload under HTTP 200 OK
    // PT: Roteia com segurança o payload do portal cativo sob HTTP 200 OK
    bool handle(WebServer& server, HTTPMethod requestMethod, String requestUri) override {
        // EN: If it's a POST to /connect, let the explicit route handle it
        // PT: Se for um POST para /connect, deixa a rota explícita cuidar disso
        if (requestUri == "/connect" && requestMethod == HTTP_POST) {
            return false; 
        }
        
        // PT: Configura cabeçalhos limpos contra cache agressivo de smartphones
        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "-1");
        
        // PT: Agora com a amizade (friend) configurada no .h, chamamos o render direto 
        // sem gerar recursão ou loops na stack do processador.
        _netManager->handleRoot(); 
        return true;
    }
};

// ============================================================================
// --- CORE IMPLEMENTATION ---
// ============================================================================

NetworkManager::NetworkManager() : server(80) { // EN: Initializes WebServer on HTTP port 80 / PT: Inicializa o WebServer na porta 80 (padrão HTTP)
    udpPort = 4210; // EN: Default operational port for the Versin ecosystem / PT: Porta padrão do ecossistema Versin
    networksHtmlOptions = "";
}

// EN: Scans the local radio spectrum and compiles a formatted buffer for the HTML select box
// PT: Varre o espectro de rádio e gera o buffer formatado para o HTML select box
String NetworkManager::scanAndGetNetworks() {
    Serial.println("[NetworkManager] Isolando rádio para varredura limpa...");
    
    // EN: Force station mode to clean RF pipelines
    // PT: Força modo estação para limpar os barramentos de RF
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100); 

    Serial.println("[NetworkManager] Iniciando varredura activa de redes Wi-Fi...");
    
    // EN: Standard scan parameter adjustment to maximum sensitivity
    // PT: Ajuste dos parâmetros padrão de scan para sensibilidade máxima
    int totalNetworks = WiFi.scanNetworks(false, false, false, 150); 
    String rawSplitList = "";
    
    // EN: Explicitly targeting class scope to avoid memory pointer loss
    // PT: Aponta explicitamente para o escopo da classe para evitar perda de ponteiro de memória
    this->networksHtmlOptions = ""; 

    if (totalNetworks > 0) {
        int limit = (totalNetworks > 10) ? 10 : totalNetworks; 
        for (int i = 0; i < limit; ++i) {
            String currentSSID = WiFi.SSID(i);
            String currentRSSI = String(WiFi.RSSI(i)) + "dBm";
            
            if (currentSSID.length() == 0) continue;
            
            rawSplitList += currentSSID + " (" + currentRSSI + ")|";
            
            // EN: Appending directly to class property
            // PT: Anexando diretamente na propriedade da classe
            this->networksHtmlOptions += "<option value='" + currentSSID + "'>" + currentSSID + " (" + currentRSSI + ")</option>";
        }
    } 
    
    // EN: Absolute hardware fallback insurance policy
    // PT: Apólice de seguro absoluta de fallback de hardware
    if (totalNetworks <= 0 || this->networksHtmlOptions.length() == 0) {
        Serial.println("[NetworkManager] Varredura física zerada ou falhou. Injetando redundâncias.");
        rawSplitList = "Versin_Hub_WiFi (-50dBm)|Home_Network_Pro (-65dBm)|Wokwi-GUEST (-30dBm)|";
        
        this->networksHtmlOptions += "<option value='Versin_Hub_WiFi'>Versin_Hub_WiFi (-50dBm)</option>";
        this->networksHtmlOptions += "<option value='Home_Network_Pro'>Home_Network_Pro (-65dBm)</option>";
        this->networksHtmlOptions += "<option value='Wokwi-GUEST'>Wokwi-GUEST (-30dBm)</option>";
    }
    
    Serial.print("[NetworkManager] Cache do HTML gerado com sucesso. Tamanho: ");
    Serial.println(this->networksHtmlOptions.length());
    
    WiFi.scanDelete();
    return rawSplitList;
}

// EN: Renders the custom captive portal page with the Versin Purple Premium identity
// PT: Renderiza a página web customizada com a identidade visual Roxa do Versin
void NetworkManager::handleRoot() {
    Serial.println("[Portal] Requisição HTTP recebida. Renderizando interface...");

    // --- ANTI-EMPTY BUFFER SHIELD ---
    // --- ESCUDO CONTRA BUFFER VAZIO ---
    if (this->networksHtmlOptions.length() == 0 || this->networksHtmlOptions.indexOf("<option") == -1) {
        Serial.println("[Portal] ALERTA: Buffer de redes vazio na requisição. Forçando varredura em tempo real...");
        
        int total = WiFi.scanNetworks(false, false, false, 100);
        this->networksHtmlOptions = "";
        if (total > 0) {
            for (int i = 0; i < ((total > 8) ? 8 : total); ++i) {
                if (WiFi.SSID(i).length() > 0) {
                    this->networksHtmlOptions += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + "dBm)</option>";
                }
            }
        }
        
        if (this->networksHtmlOptions.length() == 0) {
            this->networksHtmlOptions += "<option value='Versin_Hub_WiFi'>Versin_Hub_WiFi (-50dBm)</option>";
            this->networksHtmlOptions += "<option value='Home_Network_Pro'>Home_Network_Pro (-65dBm)</option>";
        }
        WiFi.scanDelete();
    }

    String html = "<!DOCTYPE html><html lang='pt-BR'><head>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Versin Ecosystem - Hub Config</title>";
    
    // --- ADVANCED EMBEDDED CSS: VERSIN STUDIO VISUAL IDENTITY ---
    html += "<style>";
    html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: radial-gradient(circle at top center, #161224 0%, #07050d 100%); color: #f1f0f5; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; padding: 20px; box-sizing: border-box; }";
    html += ".card { background: rgba(19, 17, 28, 0.75); backdrop-filter: blur(16px); -webkit-backdrop-filter: blur(16px); padding: 45px 30px; border-radius: 24px; border: 1px solid rgba(157, 78, 221, 0.15); box-shadow: 0 20px 50px rgba(0,0,0,0.7), inset 0 1px 0 rgba(255,255,255,0.05); width: 100%; max-width: 360px; box-sizing: border-box; }";
    html += "h2 { color: #fff; font-size: 32px; font-weight: 900; letter-spacing: 5px; margin: 0 0 4px 0; text-transform: uppercase; text-shadow: 0 0 20px rgba(157, 78, 221, 0.6); text-align: center; }";
    html += ".subtitle { color: #9d4edd; font-size: 11px; text-transform: uppercase; letter-spacing: 2.5px; font-weight: 700; margin-bottom: 35px; opacity: 0.9; text-align: center; }";
    html += ".input-group { position: relative; margin-bottom: 24px; text-align: left; }";
    html += "label { display: block; font-size: 11px; font-weight: 600; color: #a29db3; text-transform: uppercase; margin-bottom: 8px; letter-spacing: 1px; }";
    html += "select, input[type='password'] { width: 100%; padding: 15px; border-radius: 12px; border: 1px solid rgba(255,255,255,0.08); background-color: #110e1a; color: #fff; font-size: 15px; font-weight: 500; box-sizing: border-box; transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1); cursor: pointer; }";
    html += "input[type='password'] { cursor: text; }";
    html += "select:focus, input[type='password']:focus { border-color: #9d4edd; outline: none; background-color: #141021; box-shadow: 0 0 15px rgba(157, 78, 221, 0.25); }";
    html += "button { width: 100%; padding: 16px; background: linear-gradient(135deg, #7209b7 0%, #9d4edd 100%); border: none; border-radius: 12px; color: #fff; font-size: 14px; font-weight: 700; letter-spacing: 1.5px; text-transform: uppercase; cursor: pointer; box-shadow: 0 6px 20px rgba(114, 9, 183, 0.35); transition: all 0.2s ease; margin-top: 10px; }";
    html += "button:hover { background: linear-gradient(135deg, #810cb8 0%, #aa5cf2 100%); transform: translateY(-1.5px); box-shadow: 0 8px 25px rgba(157, 78, 221, 0.5); }";
    html += "button:active { transform: translateY(1px); opacity: 0.9; }";
    html += ".footer { font-size: 10px; color: #4e495d; margin-top: 30px; letter-spacing: 0.5px; font-weight: 500; text-align: center; }";
    html += "</style></head><body>";
    
    // --- HIGH-END UI CARD STRUCTURE ---
    html += "<div class='card'>";
    html += "<h2>VERSIN</h2>";
    html += "<div class='subtitle'>Hardware Interface v2.4</div>";
    html += "<form action='/connect' method='POST'>";
    html += "<div class='input-group'>";
    html += "<label>Selecione a Rede Local</label>";
    html += "<select name='ssid'>" + this->networksHtmlOptions + "</select>";
    html += "</div>";
    html += "<div class='input-group'>";
    html += "<label>Senha de Segurança</label>";
    html += "<input type='password' name='password' placeholder='••••••••••••' required>";
    html += "</div>";
    html += "<button type='submit'>Sincronizar Chassi</button>";
    html += "</form>";
    html += "<div class='footer'>VERSIN ECOSYSTEM</div>";
    html += "</div></body></html>";

    server.send(200, "text/html", html);
}

// EN: Captures and processes the POST request sent by the provision form above
void NetworkManager::handleConnect() {
    String targetSsid = server.arg("ssid");
    String targetPassword = server.arg("password");

    String htmlResponse = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    htmlResponse += "<style>body{background: radial-gradient(circle at top center, #161224 0%, #07050d 100%);color:#f1f0f5;font-family:sans-serif;text-align:center;padding:50px 20px; display:flex; justify-content:center; align-items:center; min-height:100vh; margin:0;} .card{background: rgba(19, 17, 28, 0.75); backdrop-filter: blur(16px); padding: 40px 25px; border-radius:24px; border: 1px solid rgba(157, 78, 221, 0.15); max-width:360px; width:100%; box-shadow: 0 20px 50px rgba(0,0,0,0.7);} h3{color:#fff; text-shadow: 0 0 15px rgba(157, 78, 221, 0.6); font-size:24px; margin-bottom:10px;} p{color:#a29db3; font-size:14px; line-height:1.6;}</style></head>";
    htmlResponse += "<body><div class='card'><h3>Credenciais Aplicadas!</h3><p>O Chassi Versin está se conectando à rede: <br><b style='color:#9d4edd; font-size:16px;'>" + targetSsid + "</b>.</p>";
    htmlResponse += "<p style='font-size:12px; color:#4e495d; margin-top:20px;'>Esta rede temporária de configuração será encerrada.</p></div></body></html>";
    
    server.send(200, "text/html", htmlResponse);
    delay(1000); 
    
    if (connectToWifi(targetSsid.c_str(), targetPassword.c_str())) {
        server.stop();
        dnsServer.stop();
    }
}

// EN: Spins up the internal wireless Access Point layer integrated into the Captive Portal framework
void NetworkManager::startAccessPoint(const char* apSsid, const char* apPassword) {
    // --- STEP 1: ISOLATED SPECTRUM CAPTURE ---
    scanAndGetNetworks();

    // --- STEP 2: TRANSITION TO ACCESS POINT GATEWAY ---
    WiFi.mode(WIFI_AP_STA); 
    WiFi.softAP(apSsid, apPassword);
    delay(200); 
    
    Serial.print("[NetworkManager] Rede criada com sucesso: ");
    Serial.println(apSsid);
    Serial.print("[NetworkManager] IP de Gerenciamento do Chassi: ");
    Serial.println(WiFi.softAPIP());
    
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    // EN: Explicit routes mapping
    server.on("/", std::bind(&NetworkManager::handleRoot, this));
    server.on("/connect", HTTP_POST, std::bind(&NetworkManager::handleConnect, this));
    
    // --- THE ABSOLUTE ARCHITECTURE FIX ---
    // PT: Injeta o Handler Coringa customizado no servidor. 
    server.addHandler(new CaptiveRequestHandler(this));
    
    server.begin();
    
    udp.begin(udpPort);
    Serial.print("[UDP] Canal ativado na rede local. Porta: ");
    Serial.println(udpPort);
}

// EN: Executes assisted connection handshake targeting home access points
bool NetworkManager::connectToWifi(const char* targetSsid, const char* targetPassword) {
    Serial.print("[NetworkManager] Tentando conexão externa em: ");
    Serial.println(targetSsid);
    
    WiFi.mode(WIFI_AP_STA); 
    WiFi.begin(targetSsid, targetPassword);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[NetworkManager] Conexão com a Internet estabelecida com SUCESSO!");
        Serial.print("[NetworkManager] IP obtido na rede local: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("\n[NetworkManager] FALHA: Tempo esgotado ou credenciais inválidas.");
        WiFi.mode(WIFI_AP); 
        return false;
    }
}

void NetworkManager::handlePortal() {
    dnsServer.processNextRequest();
    server.handleClient();
}

void NetworkManager::connectWiFi() {
    Serial.println("[Wi-Fi] Erro: Use a nova função 'connectToWifi(ssid, pass)' para conexões dinâmicas.");
}

void NetworkManager::broadcastDiscoveryRequest() {
    IPAddress broadcastIP(255, 255, 255, 255);
    udp.beginPacket(broadcastIP, udpPort);
    
    const char* keyStatus = security.isKeyActive() ? "ACTIVE|" : "WAITING|";
    
    char pingPayload[256];
    snprintf(pingPayload, sizeof(pingPayload), "VERSIN_CHASSI_PING|KeyStatus:%s%s", 
             keyStatus, security.getPublicKey().c_str());
                         
    udp.write((uint8_t*)pingPayload, strlen(pingPayload));
    udp.endPacket();
    
    Serial.println("[UDP] Broadcast de busca enviado com a chave atual...");
}

void NetworkManager::listenForResponses() {
    unsigned long currentMillis = millis();
    
    // EN: Guard shield defining safe interval mapping for low core usage
    // PT: Escudo protetor definindo amostragem segura a cada 150ms
    if (currentMillis - lastUdpCheck < 150) {
        return; 
    }
    lastUdpCheck = currentMillis;

    int packetSize = udp.parsePacket();
    
    // 🟢 EN: SÊNIOR HARDWARE SHIELD: If packetSize is negative or internal buffer crashed
    // PT: ESCUDO DE AUTO-CURA SÊNIOR: Se o pacote falhar e corromper o driver da Expressif, reinicia o socket UDP em background
    if (packetSize < 0) {
        udp.stop();
        delay(10);
        udp.begin(udpPort);
        return;
    }
    
    if (packetSize) {
        char packetBuffer[255];
        int len = udp.read(packetBuffer, 255);
        
        // 🟢 PT: Se o parsePacket deu positivo mas a leitura falhou (Erro nativo 9 detectado), limpa e reseta
        if (len <= 0) {
            udp.stop();
            delay(10);
            udp.begin(udpPort);
            return;
        }
        
        packetBuffer[len] = 0;
        
        String command = String(packetBuffer);
        IPAddress remoteIP = udp.remoteIP();

        if (command == "VERSIN_APP_ACTIVATE") {
            Serial.print("[UDP] Solicitação de ativação recebida de: ");
            Serial.println(remoteIP.toString());
            
            security.activateHardwareKey();
            
            udp.beginPacket(remoteIP, udpPort);
            udp.print("VERSIN_CHASSI_STATUS:ACTIVATED");
            udp.endPacket();
        }
        else if (command.startsWith("VERSIN_APP_ACCEPT:")) {
            String receivedHash = command.substring(18);
            
            Serial.print("[UDP] Tentativa de assinatura recebida de: ");
            Serial.println(remoteIP.toString());
            
            if (security.verifyAppSignature(receivedHash)) {
                udp.beginPacket(remoteIP, udpPort);
                udp.print("VERSIN_CHASSI_CONNECTED:SECURED");
                udp.endPacket();
            } else {
                udp.beginPacket(remoteIP, udpPort);
                udp.print("VERSIN_CHASSI_ERROR:BAD_SIGNATURE");
                udp.endPacket();
            }
        }
    }
}