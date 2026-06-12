#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>

// Pinos — ajuste RELE_PIN pro que funcionou no seu teste
#define SOLO_PIN    34
#define LDR_PIN     35
#define RELE_PIN    25
#define LED_R       26
#define LED_G       33

WebServer server(80);

int limiteMin     = 40;
int limiteMax     = 65;
bool autoMode     = true;
bool bombaLigada  = false;
unsigned long bombaLigadaEm = 0;
int tempoMaxBomba = 30000; // segurança: desliga bomba após 30s mesmo sem sensor

// ── Sensores ──────────────────────────────────────────────

int lerUmidadeSolo() {
  int raw = analogRead(SOLO_PIN);
  int pct = map(raw, 4095, 1000, 0, 100);
  return constrain(pct, 0, 100);
}

int lerLuminosidade() {
  int raw = analogRead(LDR_PIN);
  int pct = map(raw, 4095, 0, 0, 100);
  return constrain(pct, 0, 100);
}

// ── Bomba ─────────────────────────────────────────────────

void ligarBomba(bool ligar) {
  bombaLigada = ligar;
  digitalWrite(RELE_PIN, ligar ? LOW : HIGH);
  digitalWrite(LED_R,    ligar ? HIGH : LOW);
  digitalWrite(LED_G,    ligar ? LOW  : HIGH);
  if (ligar) bombaLigadaEm = millis();
  Serial.println(ligar ? ">>> Bomba LIGADA" : ">>> Bomba DESLIGADA");
}

// ── Handlers HTTP ─────────────────────────────────────────

void setCORS() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
  setCORS();
  server.send(204);
}

void handleStatus() {
  int solo = lerUmidadeSolo();
  int luz  = lerLuminosidade();

  Serial.println("[/status] solo:" + String(solo) + "% luz:" + String(luz) + "%");

  String json = "{";
  json += "\"solo\":"      + String(solo)          + ",";
  json += "\"luz\":"       + String(luz)            + ",";
  json += "\"bomba\":"     + String(bombaLigada)    + ",";
  json += "\"auto\":"      + String(autoMode)       + ",";
  json += "\"limiteMin\":" + String(limiteMin)      + ",";
  json += "\"limiteMax\":" + String(limiteMax);
  json += "}";

  setCORS();
  server.send(200, "application/json", json);
}

void handlePump() {
  if (server.hasArg("state")) {
    bool on = server.arg("state") == "1";
    ligarBomba(on);
    Serial.println("[/pump] state:" + String(on));
  }
  setCORS();
  server.send(200, "text/plain", "ok");
}

void handleConfig() {
  if (server.hasArg("min"))  limiteMin = server.arg("min").toInt();
  if (server.hasArg("max"))  limiteMax = server.arg("max").toInt();
  if (server.hasArg("auto")) autoMode  = server.arg("auto") == "1";
  Serial.println("[/config] min:" + String(limiteMin) + " max:" + String(limiteMax) + " auto:" + String(autoMode));
  setCORS();
  server.send(200, "text/plain", "ok");
}

void handleNotFound() {
  setCORS();
  server.send(404, "text/plain", "not found");
}

// ── Setup ─────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  pinMode(RELE_PIN, OUTPUT);
  pinMode(LED_R,    OUTPUT);
  pinMode(LED_G,    OUTPUT);
  digitalWrite(RELE_PIN, HIGH); // bomba desligada no boot

  // WiFiManager — se não conectar cria hotspot "IrrigaBot"
  WiFiManager wm;
  // wm.resetSettings(); // descomenta pra apagar rede salva
  wm.setConnectTimeout(20);
  wm.setConfigPortalTimeout(120);

  Serial.println("Iniciando WiFiManager...");
  bool conectou = wm.autoConnect("IrrigaBot");

  if (conectou) {
    Serial.println("Conectado! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("Falha na conexão. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  // Rotas
  server.on("/api/status",  HTTP_GET,     handleStatus);
  server.on("/api/pump",    HTTP_GET,     handlePump);
  server.on("/api/config",  HTTP_GET,     handleConfig);
  server.on("/api/status",  HTTP_OPTIONS, handleOptions);
  server.on("/api/pump",    HTTP_OPTIONS, handleOptions);
  server.on("/api/config",  HTTP_OPTIONS, handleOptions);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Servidor HTTP iniciado");
  Serial.println("Rotas: /status /pump /config");
}

// ── Loop ──────────────────────────────────────────────────

void loop() {
  server.handleClient();

  static unsigned long ultimaLeitura = 0;
  if (millis() - ultimaLeitura > 10000) {
    ultimaLeitura = millis();

    int solo = lerUmidadeSolo();
    int luz  = lerLuminosidade();
    Serial.println("Solo: " + String(solo) + "% | Luz: " + String(luz) + "% | Bomba: " + String(bombaLigada) + " | Auto: " + String(autoMode));
    Serial.println("Limites: MAX: " +String(limiteMax) +" | MIN:" + String(limiteMin));

    if (autoMode) {
      if (!bombaLigada && solo < limiteMin) {
        Serial.println("Solo seco (" + String(solo) + "%) — ligando bomba");
        ligarBomba(true);
      }
      if (bombaLigada && solo >= limiteMax) {
        Serial.println("Solo úmido (" + String(solo) + "%) — desligando bomba");
        ligarBomba(false);
      }
    }

    // Segurança: desliga bomba se ficar ligada por mais de 30s
    if (bombaLigada && millis() - bombaLigadaEm > tempoMaxBomba) {
      Serial.println("Timeout de segurança — desligando bomba");
      ligarBomba(false);
    }
  }
}