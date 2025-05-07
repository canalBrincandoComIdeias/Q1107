#include <WiFi.h>
#include <MQTT.h>
#include "user.h"  //Informar a rede WiFi, a senha, usuário MQTT e a senha

//const char ssid[] = "xxx";
//const char pass[] = "xxx";

#define MEU_DISPOSITIVO "bi_q1107"
#define TOPICO_ALERTA "bi_q1107/alerta"

//Servidor MQTT
const char servidor[] = "54.233.221.233";
//const char servidor[] = "mqtt.monttudo.com";  //(caso o IP não funcione, use a identificação pelo domínio)

//Usuário MQTT
const char deviceName[] = MEU_DISPOSITIVO;
const char mqttUser[] = "todos";
const char mqttPass[] = "cortesi@BrincandoComIdeias";

#define pinLEDPlaca 8
#define pinLEDVivo 7
#define pinSirene 4
#define pinLuz 3
#define pinBotao 5

#define TEMPO_AVISO 5000
#define TEMPO_PULSO 100

WiFiClient net;
MQTTClient client;

int estado = 0;  //0=StandBy 1=Aviso 2=Alarme
int estadoAnt = 1;
unsigned long delayAviso;
unsigned long delayVivo;
bool silencio = false;

void connect() {
  Serial.print("->wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(";");

  Serial.print("->mqtt...");
  while (!client.connect(deviceName, mqttUser, mqttPass)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(";");
  Serial.println("->conectado;");
}

void messageReceived(String &topic, String &payload) {
  Serial.println(topic + ":" + payload);

  if (topic == TOPICO_ALERTA) {
    estado = payload.toInt();
  }

  // Nota: Não use o "client" nessa função para publicar, subscrever ou
  // dessubscrever porque pode causar problemas quando outras mensagens são recebidas
  // enquanto são feitos envios ou recebimentos. Então, altere uma variavel global,
  // coloque numa fila e trate essa fila no loop depois de chamar `client.loop()`.
}

void setup() {
  pinMode(pinLEDPlaca, OUTPUT);
  pinMode(pinLEDVivo, OUTPUT);
  pinMode(pinSirene, OUTPUT);
  pinMode(pinLuz, OUTPUT);
  pinMode(pinBotao, INPUT_PULLDOWN);

  digitalWrite(pinLEDPlaca, LOW);
  digitalWrite(pinLEDVivo, HIGH);
  digitalWrite(pinSirene, LOW);
  digitalWrite(pinLuz, LOW);

  //Serial1.begin(9600, SERIAL_8N1, 20, 21);  //VELOCIDADE, CONFIGURACAO_SERIAL, RX, TX
  Serial.begin(9600);
  WiFi.begin(ssid, pass);

  // Observação: Nomes de domínio locais (por exemplo, "Computer.local" no OSX)
  // não são suportados pelo Arduino. Você precisa definir o endereço IP diretamente.
  client.begin(servidor, net);
  client.onMessage(messageReceived);

  connect();
  client.subscribe(TOPICO_ALERTA);
  digitalWrite(pinLEDPlaca, HIGH);
  digitalWrite(pinLEDVivo, LOW);
}

void loop() {
  client.loop();
  delay(10);  //corrige alguns problemas com a estabilidade do WiFi

  if (!client.connected()) {
    digitalWrite(pinLEDPlaca, LOW);
    digitalWrite(pinLEDVivo, HIGH);
    connect();
    client.subscribe(TOPICO_ALERTA);
    digitalWrite(pinLEDPlaca, HIGH);
    digitalWrite(pinLEDVivo, LOW);
  }

  if ((estado == 0) && (estadoAnt != 0)) {
    digitalWrite(pinSirene, LOW);
    digitalWrite(pinLuz, LOW);
    silencio = false;
  }

  if ((estado == 1) && (estadoAnt != 1)) {
    digitalWrite(pinLuz, HIGH);
    silencio = false;
  }
  if (estado == 1) {
    if (!silencio) {
      if ((millis() - delayAviso) > TEMPO_AVISO) {
        delayAviso = millis();
        digitalWrite(pinSirene, HIGH);
        delay(TEMPO_PULSO);
        digitalWrite(pinSirene, LOW);
      }
    }
  }

  if ((estado == 2) && (estadoAnt != 2)) {
    silencio = false;
    digitalWrite(pinLuz, HIGH);
    digitalWrite(pinSirene, HIGH);
  }

  if (estado != 0) {
    if (digitalRead(pinBotao)) {
      digitalWrite(pinSirene, LOW);
      silencio = true;
    }
  }

  if ((millis() - delayVivo) > 15000) {
    delayVivo = millis();
    digitalWrite(pinLEDVivo, HIGH);
    delay(50);
    digitalWrite(pinLEDVivo, LOW);
  }

  estadoAnt = estado;

  //client.subscribe(topico);
  //client.unsubscribe(topico);
  //client.publish(topico, valor);
}
