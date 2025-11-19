#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const int Trigger = 13;
const int Echo = 12;

// Pinos
const int LedVerde = 34;
const int LedAmarelo = 36;
const int LedVermelho = 35;
const int Buzzer = 14;

// Parâmetros
const int DistanciaCorreta = 70;
const int PosturaErrada = 20;
const long Descanso = 20000;

// Estados
unsigned long workSessionStartTime = 0;
bool onBreak = false;
bool isWorking = false;

// Configuração Inicial
// Inicia a comunicação serial, configura pinos e conecta ao Wi-Fi.
void setup() {
  Serial.begin(115200);

  pinMode(Trigger, OUTPUT);
  pinMode(Echo, INPUT);
  pinMode(LedVerde, OUTPUT);
  pinMode(LedAmarelo, OUTPUT);
  pinMode(LedVermelho, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// O Ciclo Principal
// Executa continuamente. Gerencia o estado de trabalho/pausa, checa a postura e o timer.
void loop() {
  if (!onBreak) {
    long distance = getDistance();
    Serial.print("Distancia: ");
    Serial.print(distance);
    Serial.println(" cm");

    checkPosture(distance);
    checkBreakTime();

  } else {    
    long distance = getDistance();
    if (distance > DistanciaCorreta) {
      Serial.println("Usuário saiu para a pausa. Reiniciando ciclo.");
      onBreak = false;
      digitalWrite(LedVermelho, LOW);
    }
  }

  delay(1000);
}

// Leitura do Sensor
// Envia o pulso ultrassônico e calcula a distância em centímetros.
long getDistance() {
  digitalWrite(Trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger, LOW);

  long duration = pulseIn(Echo, HIGH);
  return (duration * 0.0343) / 2;
}

// Verificação de Postura
// Analisa a distância, define o estado (Ausente/Incorreta/Correta), aciona LEDs e gerencia 
// o início do timer de trabalho.
void checkPosture(long dist) {
  if (dist > DistanciaCorreta) {
    digitalWrite(LedVerde, LOW);
    digitalWrite(LedAmarelo, LOW);
    Serial.println("Status: Ausente");
    if (isWorking) {
      isWorking = false;
      Serial.println("Sessão de trabalho interrompida.");
    }
  } 
  else if (dist < PosturaErrada) {
    digitalWrite(LedVerde, LOW);
    digitalWrite(LedAmarelo, HIGH);
    Serial.println("Status: Postura Incorreta!");
    sendHTTPStatus("POSTURA_INCORRETA");
    if (isWorking) {
      isWorking = false;
      Serial.println("Sessão de trabalho interrompida (postura ruim).");
    }
  } 
  else {
    digitalWrite(LedVerde, HIGH);
    digitalWrite(LedAmarelo, LOW);
    Serial.println("Status: Postura Correta");
    sendHTTPStatus("POSTURA_OK");
    
    if (!isWorking) {
      isWorking = true;
      workSessionStartTime = millis();
      Serial.println("Iniciando contagem de tempo de trabalho...");
    }
  }
}

// Pausa Inteligente
// Verifica se o tempo de trabalho focado foi atingido e dispara o alarme no Buzzer/LED Vermelho.
void checkBreakTime() {
  unsigned long currentTime = millis();

  if (isWorking && (currentTime - workSessionStartTime > Descanso)) {
    Serial.println("Status: Hora da Pausa!");
    onBreak = true;
    isWorking = false; 

    digitalWrite(LedVermelho, HIGH);
    digitalWrite(LedVerde, LOW);
    digitalWrite(LedAmarelo, LOW);

    digitalWrite(Buzzer, HIGH);
    delay(500);
    digitalWrite(Buzzer, LOW);

    sendHTTPStatus("HORA_DA_PAUSA");
  }
}

// Envio de Dados HTTP
// Conecta ao servidor externo e envia o status atual do sistema via requisição POST.
void sendHTTPStatus(String status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String serverEndpoint = "http://httpbin.org/post";
    String jsonPayload = "{\"status\":\"" + status + "\"}";

    http.begin(serverEndpoint);
    http.addHeader("Content-Type", "application/json");

    Serial.print("HTTP: Enviando status '");
    Serial.print(status);
    Serial.println("'...");

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.print("HTTP OK (");
      Serial.print(httpResponseCode);
      Serial.println(")");
    } else {
      Serial.print("HTTP ERRO! Codigo: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Erro: WiFi não conectado. Envio HTTP ignorado.");
  }
}