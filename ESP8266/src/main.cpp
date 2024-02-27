#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#ifndef STASSID
#define STASSID "Barros2"
#define STAPSK  "25222639"
#endif

const char* host = "lavadora";
const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

boolean lavando = false;
boolean centrifugando = false;

String tarefaAtual = "Máquina pronta";
int tempoTotal = 0;
int tempoTranscorrido = 0;

void desabilitaWatchdog() {
  yield();
  ESP.wdtDisable(); // Software WDT OFF
  *((volatile uint32_t*) 0x60000900) &= ~(1); // Hardware WDT OFF
}

void inicializaPortas() {
  pinMode(D0, OUTPUT); //Buzzer
  pinMode(D1, OUTPUT); //Motor sentido horário
  pinMode(D2, OUTPUT); //Motor sentido anti-horário
  pinMode(D7, OUTPUT); //Bomba + freio
  pinMode(D8, OUTPUT); //Entrada

  pinMode(D5, INPUT); //Pressostato
  pinMode(D6, INPUT); //Tampa

  digitalWrite(D0, LOW);
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(D7, LOW);
  digitalWrite(D8, LOW);
}

void mudaTarefa (String tarefa) {
  tarefaAtual = tarefa;
  Serial.println("Tarefa alterada para: " + tarefa);
}

void delayEmSegundos(float tempo) {
  tempo *= 100;
  for(int i = 0; i < tempo; i++) {
    delay(10);
    yield();
    httpServer.handleClient();
    MDNS.update();
  }
}

void apita(float tempo) { //tempo em segundos
  if(!lavando) { return; }
  tempoTotal = tempo;
  unsigned long inicio = millis();
  tempo *= 1000;
  while((millis() - inicio) < tempo) {
    tempoTranscorrido = millis() - inicio;
    digitalWrite(D0, HIGH);
    delayEmSegundos(1);
    digitalWrite(D0, LOW);
    delayEmSegundos(1);
    if(!lavando) { return; }
  }
}

void apitoLongo() {
    digitalWrite(D0, HIGH);
    delayEmSegundos(2);
    digitalWrite(D0, LOW);
}

boolean cheiaDeAgua() {
  return (digitalRead(D5));
}

void giraSentidoHorario() {
  digitalWrite(D2, LOW);
  digitalWrite(D1, HIGH);
}

void giraSentidoAntiHorario() {
  digitalWrite(D1, LOW);
  digitalWrite(D2, HIGH);
}

void desligaMotor() {
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
}

void ligaEntradaDeAgua() {
  digitalWrite(D8, HIGH);
}

void desligaEntradaDeAgua() {
  digitalWrite(D8, LOW);
}

void ligaBombaEFreio() {
  digitalWrite(D7, HIGH);
}

void desligaBombaEFreio() {
  digitalWrite(D7, LOW);
}

void encher() {
  if(!lavando) { return; }
  mudaTarefa("Enchendo");
  tempoTotal = 0;
  unsigned long inicio = millis();
  ligaEntradaDeAgua();
  while(!cheiaDeAgua()) {
    tempoTranscorrido = millis() - inicio;
    delayEmSegundos(1);
    if(!lavando) { 
      desligaEntradaDeAgua();
      return;
    }
  }
  desligaEntradaDeAgua();
}

void encheApitando() {
  if(!lavando) { return; }
  mudaTarefa("Enchendo e apitando, coloque o amaciante");
  tempoTotal = 0;
  unsigned long inicio = millis();
  ligaEntradaDeAgua();
  while(!cheiaDeAgua()) {
    tempoTranscorrido = millis() - inicio;
    apita(1);
    if(!lavando) { 
      desligaEntradaDeAgua();
      return;
    }
  }
  desligaEntradaDeAgua();
}

void esvaziar(float tempo) {
  if(!lavando) { return; }
  mudaTarefa("Esvaziando");
  tempoTotal = tempo;
  ligaBombaEFreio();
  unsigned long inicio = millis();
  tempo *= 1000;
  while((millis() - inicio) < tempo) {
    tempoTranscorrido = millis() - inicio;
    delayEmSegundos(1);
    if(!lavando) { 
      desligaBombaEFreio();
      return;
    }
  }
  desligaBombaEFreio();
}

void bateARoupa(float tempo) {
  if(!lavando) { return; }
  mudaTarefa("Batendo");
  tempoTotal = tempo;
  unsigned long inicio = millis();
  tempo *= 1000;
  while((millis() - inicio) < tempo) {
    tempoTranscorrido = millis() - inicio;
    giraSentidoHorario();
    delayEmSegundos(0.5);
    desligaMotor();
    delayEmSegundos(0.2);
    giraSentidoAntiHorario();
    delayEmSegundos(0.5);
    desligaMotor();
    delayEmSegundos(0.2);
    /*if(!cheiaDeAgua()) {
      encher();
      inicio = millis();
    }*/
    if(!lavando) { return; }
  }
}

void molho(float tempo) {
  if(!lavando) { return; }
  mudaTarefa("Molho");
  tempoTotal = tempo;
  unsigned long inicio = millis();
  tempo *= 1000;
  while((millis() - inicio) < tempo) {
    tempoTranscorrido = millis() - inicio;
    delayEmSegundos(1);
    /*if(!cheiaDeAgua()) {
      encher();
      inicio = millis();
    }*/
    if(!lavando) { return; }
  }
}

void centrifugar(float tempo) {
  if(!lavando) { return; }
  centrifugando = true;
  mudaTarefa("Centrifugando");
  tempoTotal = tempo;
  unsigned long inicio = millis();
  tempo *= 1000;
  ligaBombaEFreio();
  delayEmSegundos(5); //5 segundos
  while((millis() - inicio) < tempo) {
    tempoTranscorrido = millis() - inicio;
    giraSentidoHorario();
    delayEmSegundos(5);
    desligaMotor();
    delayEmSegundos(5);
    if(!lavando) {
      mudaTarefa("Parando máquina, aguarde...");
      delayEmSegundos(120); //2 minutos
      desligaBombaEFreio();
      mudaTarefa("Lavagem cancelada");
      centrifugando = false;
      return;
    }
  }
  delayEmSegundos(120); //2 minutos
  desligaBombaEFreio();
  centrifugando = false;
}

String geraJSON(boolean sucesso, String mensagem ) {
    String  sucessoStr = sucesso ? "true" : "false";
    return ("{\"success\": " + sucessoStr + ", \"message\": \"" + mensagem + "\"}");
}

void index() {
  httpServer.send(200, "application/json", geraJSON(lavando, tarefaAtual));
}

void cancelarLavagem() {
  if(lavando) {
    mudaTarefa("Lavagem cancelada");
    
    lavando = false;
    httpServer.send(200, "application/json", geraJSON(true, "Lavagem cancelada com sucesso"));
  } else {
    httpServer.send(200, "application/json", geraJSON(false, "Não há lavagem em andamento"));
  }
}

void lavagemRapida() {
  if(lavando || centrifugando) {
    httpServer.send(200, "application/json", geraJSON(false, "Máquina já em lavagem ou parando o motor."));
    return;
  }
  httpServer.send(200, "application/json", geraJSON(true, ""));
  lavando = true;
  encher(); //tempo indeterminado
  bateARoupa(600); //10 minutos
  molho(600); //10 minutos
  bateARoupa(600); //10 minutos
  esvaziar(360); //6 minutos
  centrifugar(300); //5 + 2 minutos (centrigugação pré-enxágue)
  encheApitando(); //tempo indeterminado
  bateARoupa(600); //10 minutos (enxágue)
  esvaziar(360); //6 minutos
  centrifugar(600); //10 + 2 minutos
  apita(60); //1 minuto
  if(lavando) {
    mudaTarefa("Lavagem rápida finalizada");
  }
  lavando = false;
}

void apenasEsvaziarEnxaguarECentrifugar() {
  if(lavando || centrifugando) {
    httpServer.send(200, "application/json", geraJSON(false, "Máquina já em lavagem ou parando o motor."));
    return;
  }
  httpServer.send(200, "application/json", geraJSON(true, ""));
  lavando = true;
  esvaziar(360); //6 minutos
  centrifugar(300); //5 + 2 minutos (centrigugação pré-enxágue)
  encheApitando(); //tempo indeterminado
  bateARoupa(600); //10 minutos (enxágue)
  esvaziar(360); //6 minutos
  centrifugar(600); //10 + 2 minutos
  apita(60); //1 minuto
  if(lavando) {
    mudaTarefa("Esvaziamento, enxague e centrifugação finalizados");
  }
  lavando = false;
}

void apenasEnxaguarECentrifugar() {
  if(lavando || centrifugando) {
    httpServer.send(200, "application/json", geraJSON(false, "Máquina já em lavagem ou parando o motor."));
    return;
  }
  httpServer.send(200, "application/json", geraJSON(true, ""));
  lavando = true;
  encheApitando(); //tempo indeterminado
  bateARoupa(600); //10 minutos (enxágue)
  esvaziar(360); //6 minutos
  centrifugar(600); //10 + 2 minutos
  apita(60); //1 minuto
  if(lavando) {
    mudaTarefa("Enxague e centrifugação finalizados");
  }
  lavando = false;
}

void apenasCentrifugar() {
  if(lavando || centrifugando) {
    httpServer.send(200, "application/json", geraJSON(false, "Máquina já em lavagem ou parando o motor."));
    return;
  }
  httpServer.send(200, "application/json", geraJSON(true, ""));
  lavando = true;
  centrifugar(600); //10 + 2 minutos
  apita(60); //1 minuto
  if(lavando) {
    mudaTarefa("Centrifugação finalizada");
  }
  lavando = false;
}

void apenasEsvaziar() {
  if(lavando || centrifugando) {
    httpServer.send(200, "application/json", geraJSON(false, "Máquina já em lavagem ou parando o motor."));
    return;
  }
  httpServer.send(200, "application/json", geraJSON(true, ""));
  lavando = true;
  esvaziar(360); //6 minutos
  apita(60); //1 minuto
  if(lavando) {
    mudaTarefa("Esvaziamento finalizado");
  }
  lavando = false;
}

void setup(void) {
  inicializaPortas();
  Serial.begin(115200);
  Serial.println();
  Serial.println("Inicializando máquina de lavar...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  desabilitaWatchdog();

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("Falha na conexão wi-fi, tentando novamente...");
  }

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);

  httpServer.on("/", [](){index();}); //status
  httpServer.on("/cancelarLavagem", [](){cancelarLavagem();});
  httpServer.on("/lavagemRapida", [](){lavagemRapida();});
  httpServer.on("/apenasEsvaziarEnxaguarECentrifugar", [](){apenasEsvaziarEnxaguarECentrifugar();});
  httpServer.on("/apenasEnxaguarECentrifugar", [](){apenasEnxaguarECentrifugar();});
  httpServer.on("/apenasCentrifugar", [](){apenasCentrifugar();});
  httpServer.on("/apenasEsvaziar", [](){apenasEsvaziar();});

  apitoLongo();
}

void loop(void) {
  httpServer.handleClient();
  MDNS.update();
}