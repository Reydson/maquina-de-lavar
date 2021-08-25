/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update
*/

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

void delayEmSegundos(float time) {
  time *= 10;
  for(int i = 0; i < time; i++) {
    delay(100);
    yield();
    httpServer.handleClient();
    MDNS.update();
  }
}

void apita(int ciclos) {
  for(int i = 0; i < ciclos; i++) {
    digitalWrite(D0, HIGH);
    delayEmSegundos(1);
    digitalWrite(D0, LOW);
    delayEmSegundos(1);
  }
}

boolean tampaFechada() {
  return (!digitalRead(D6));
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
  ligaEntradaDeAgua();
  while(!cheiaDeAgua()) {
    delayEmSegundos(1);
  }
  desligaEntradaDeAgua();
}

void encherApitando() {
  ligaEntradaDeAgua();
  while(!cheiaDeAgua()) {
    apita(1);
  }
  desligaEntradaDeAgua();
}

void esvaziar() { //6 minutos
  ligaBombaEFreio();
  delayEmSegundos(360); //6 minutos
  desligaBombaEFreio();
}

void bateComMolho() {
  for(int i = 0; i < 600; i++) { //14 minutos
    giraSentidoHorario();
    delayEmSegundos(0.5);
    desligaMotor();
    delayEmSegundos(0.2);
    giraSentidoAntiHorario();
    delayEmSegundos(0.5);
    desligaMotor();
    delayEmSegundos(0.2);
  }
  desligaMotor();
  delayEmSegundos(1200); //20 minutos
}

void enxaguar() {
  for(int i = 0; i < 300; i++) { //7 minutos
    giraSentidoHorario();
    delayEmSegundos(0.5);
    desligaMotor();
    delayEmSegundos(0.2);
    giraSentidoAntiHorario();
    delayEmSegundos(0.5);
    desligaMotor();
    delayEmSegundos(0.2);
  }
  desligaMotor();
}

void centrifugar() { // 12 minutos
  ligaBombaEFreio();
  delayEmSegundos(5); //5 segundos
  for(int i = 0; i < 60; i++) { // 10 minutos
    giraSentidoHorario();
    delayEmSegundos(5);
    desligaMotor();
    delayEmSegundos(5);
  }
  desligaMotor();
  delayEmSegundos(120); //2 minutos
  desligaBombaEFreio();
}

void centrifugacaoPreEnxague() { // 7 minutos
  ligaBombaEFreio();
  delayEmSegundos(5); //5 segundos
  for(int i = 0; i < 30; i++) { // 5 minutos
    giraSentidoHorario();
    delayEmSegundos(5);
    desligaMotor();
    delayEmSegundos(5);
  }
  desligaMotor();
  delayEmSegundos(120); //2 minutos
  desligaBombaEFreio();
}

void lavagemRapida() {
  encher();
  bateComMolho(); //14 minutos batendo + 20 minutos de molho
  esvaziar(); //6 minutos
  centrifugacaoPreEnxague(); // 7 minutos
  encherApitando();
  enxaguar(); //7 minutos
  esvaziar(); //6 minutos
  centrifugar(); //12 minutos
  apita(60); //2 minutos
}

void setup(void) {
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

  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
  
  lavagemRapida();
}

void loop(void) {
  httpServer.handleClient();
  MDNS.update();
}