#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <time.h>

// ===== CONFIGURAÇÕES DO SENSOR =====
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== ESTRUTURA DE DADOS (Exigência AICSS) =====
struct DadosSensor {
  float temperatura;
  float umidade;
} leitura;

// ===== DADOS DA REDE E AWS =====
const char* ssid = "nome_wifi";
const char* password = "senha_wifi";
const char* aws_url = "endpoint";
const char* api_key = "chave_api";

// ===== NTP (HORÁRIO) =====
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600;
const int daylightOffset_sec = 0;

// ===== VARIÁVEIS DE CONTROLE =====
WebServer server(80);
unsigned long lastSendAWS = 0;
unsigned long intervalAWS = 3000;

// ===== FUNÇÃO PARA ENVIAR PARA A AWS =====
bool enviarParaNuvem() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.setTimeout(5000); 
  http.begin(aws_url);
  
  http.addHeader("Content-Type", "text/plain");
  http.addHeader("x-api-key", api_key);

  String payload = String(leitura.temperatura) + "," + String(leitura.umidade);
  
  int httpResponseCode = http.POST(payload);
  bool sucesso = (httpResponseCode == 200 || httpResponseCode == 201);
  
  if (sucesso) {
    Serial.println(">>> SUCESSO! Dados na AWS. Proximo envio em 60s.");
  } else {
    Serial.printf(">>> FALHA (Erro: %d). Tentando novamente em 3s...\n", httpResponseCode);
  }
  
  http.end();
  return sucesso;
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");

  // ===== INICIALIZA NTP =====
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Horario sincronizado!");

  server.on("/sensores", []() {
    String resp = String(leitura.temperatura) + "," + String(leitura.umidade);
    server.send(200, "text/plain", resp);
  });
  
  server.begin();
}

void loop() {
  server.handleClient();

  if (millis() - lastSendAWS > intervalAWS) {
    leitura.umidade = dht.readHumidity();
    leitura.temperatura = dht.readTemperature();

    if (!isnan(leitura.temperatura) && !isnan(leitura.umidade)) {

      // ===== PRINT COM HORÁRIO =====
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        Serial.printf("Temp: %.1f°C | Umidade: %.1f%% | Hora: %02d:%02d:%02d\n",
                      leitura.temperatura,
                      leitura.umidade,
                      timeinfo.tm_hour,
                      timeinfo.tm_min,
                      timeinfo.tm_sec);
      } else {
        Serial.printf("Temp: %.1f°C | Umidade: %.1f%% | Hora: (erro)\n",
                      leitura.temperatura,
                      leitura.umidade);
      }

      if (enviarParaNuvem()) {
        intervalAWS = 60000;
      } else {
        intervalAWS = 3000;
      }
    }
    lastSendAWS = millis();
  }
}