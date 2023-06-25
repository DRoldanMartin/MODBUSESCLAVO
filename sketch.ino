h/********** Librerias MODBUS TCP-IP **********/
#include <SPI.h>
#include <Ethernet.h>
#include "MgsModbus.h"

/********** Libreria PROTOTHREADS **********/
#include <pt.h>

/********** Librerias PANTALLA LCD **********/
#include <LiquidCrystal.h>


LyquidCrystal lcd(7, 6, 5, 4, 3, 2);
float nivel_limite = 28.00;
float nivel_del_agua;
int BOYA_MINIMO = 0;
int BOYA_MAXIMO = 9;
int Electrovalvula = 8;
int Estado_de_alimentacion = 55;
float umbral_ALTO;
float umbral_BAJO;
float umbral_MARCHA;
float umbral_PARO;
int peticion_BOMBA;
const int EchoPin = 54;
const int TriggerPin = 56;
float distancia;
int Estado;

struct pt hilo1;
struct pt hilo2;
struct pt hilo3;
struct pt hilo4;

MgsModbus Mb;

/********** Configuracion de Red **********/
byte mac[] = {0x90, 0xA2, 0xDA, 0x0E, 0x94, 0xB5};
IPAddress ip(192, 168, 1, 40);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(80);
EthernetClient client;


void setup(){
    
    Ethernet.begin(mac, ip, gateway, subnet);
    Serial.begin(9600);
    server.begin();

    pinMode(BOYA_MAXIMO, INPUT);
    pinMode(BOYA_MINIMO, INPUT);
    pinMode(Estado_de_alimentacion, INPUT);
    pinMode(Electrovalvula, OUTPUT);
    pinMode(EchoPin, INPUT);
    pinMode(TriggerPin, OUTPUT);
    digitalWrite(TriggerPin, LOW);

    PT_INIT(&hilo1);
    PT_INIT(&hilo2);
    PT_INIT(&hilo3);
    PT_INIT(&hilo4);

    lcd.begin(16, 2);
    lcd.print(F("    DEPOSITO"));


    Mb.MbData[0] = 0;
    Mb.MbData[1] = 0;
    Mb.MbData[2] = 0;
    Mb.MbData[3] = 0;
    Mb.MbData[4] = 0;
    Mb.MbData[5] = 0;
    Mb.MbData[6] = 0;
    Mb.MbData[7] = 0;
    Mb.MbData[8] = 0;
    Mb.MbData[9] = 0;
    Mb.MbData[10] = 0;
    Mb.MbData[11] = 0;
}


void loop(){

    float ti;
    float d;
    digitalWrite(TriggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(TriggerPin, LOW);
    ti = pulseIn(EchoPin, LOW);
    d = ti/59;
    nivel_del_agua = nivel_limite - d;


    Mb.MbData[0] = digitalRead(0);
        BOYA_MINIMO = digitalRead(0);
    
    Mb.MbData[1] = digitalRead(9);
        BOYA_MAXIMO = digitalRead(9);
    
    Mb.MbData[2] = digitalRead(55);
        Estado_de_alimentacion = digitalRead(55);
    

    float(umbral_ALTO, bitRead(Mb.MbData[3],0));
    float(umbral_BAJO, bitRead(Mb.MbData[4],0));
    float(umbral_MARCHA, bitRead(Mb.MbData[5],0));
    float(umbral_PARO, bitRead(Mb.MbData[6],0));

    Mb.MbData[7] = nivel_del_agua;
    Mb.MbData[8] = peticion_BOMBA;
    Mb.MbsRun();

    pantalla_LCD(&hilo1);
    deposito(&hilo2);
    servidor_WEB(&hilo3);
    ultrasonidos(&hilo4);
}


/********** FUNCION PANTALLA LCD **********/
void pantalla_LCD(struct pt *pt){

    float ti;
    float d;
    digitalWrite(TrggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(TriggerPin, LOW);
    ti = pulseIn(EchoPin, HIGH);
    d = ti/59;
    nivel_del_agua = nivel_limite - d;

    PT_BEGIN(pt);
    static long t = 0;
    lcd.setCursor(1, 1);
    lcd.print(F("Nivel:"));
    lcd.setCursor(8, 1);
    lcd.print(nivel_del_agua);
    lcd.setCursor(13, 1);
    lcd.print(F("cm"));
    t = millis();
    PT_WAIT_WHILE(pt, (millis()-t)<3000);
    PT_END(pt);
}


/********** FUNCION DEL DEPOSITO **********/
void deposito(struct pt *pt){

    PT_BEGIN(pt);
    static long t = 0;
    float ti;
    float d;
    digitalWrite(TrggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(TriggerPin, LOW);
    ti = pulseIn(EchoPin, HIGH);
    d = ti/59;
    nivel_del_agua = nivel_limite - d;

    if (nivel_del_agua >= Mb.MbData[3]){
        t = millis();
        PT_WAIT_WHILE(pt, (millis()-t)<5000);
        digitalWrite(Electrovalvula, LOW);
        peticion_BOMBA = 0;
        Estado = 1;
    }

    if (nivel_del_agua <= Mb.MbData[4]){
        t = millis();
        PT_WAIT_WHILE(pt, (millis()-t)<5000);
        digitalWrite(Electrovalvula, HIGH);
        peticion_BOMBA = 1;
        Estado = 0;
    }

    if (Mb:MbData[1] == 1){
        t = millis();
        PT_WAIT_WHILE(pt, (millis()-t)<3000);
        digitalWrite(Electrovalvula, LOW);
        peticion_BOMBA = 0;
        Estado = 1;
    }

    if (Mb:MbData[0] == 0){
        t = millis();
        PT_WAIT_WHILE(pt, (millis()-t)<3000);
        digitalWrite(Electrovalvula, HIGH);
        peticion_BOMBA = 1;
        Estado = 0;
    }

    PT_END(pt);
}


/********** FUNCION DEL SERVIDOR WEB **********/
void servidor_WEB(struct pt *pt){
    PT_BEGIN(pt);
    EthernetClient client = server.avaliable();
    if (client) {
        Serial.println(F("new client"));
        boolean currentLineIsBlank = true;
        String cadena = "";
        
        while (client.connected()){
            if (client.available()){
                char c = client.read();
                Serial.write(c);
                cadena.concat(c);

                if (c == '\n' && currentLineIsBlank){
                    client.printlb(F("HTTP/1.1 200 OK"));
                    client.printlb(F("Content-Type: text/html"));
                    client.printlb(F("Connection: close"));
                    client.printlb(F("Refresh: 1"));
                    client.printlb(F("HTTP/1.1 200 OK"));
                    client.printlb();
                    client.printlb(F("<!DOCTYPE HTML>"));
                    client.printlb(F("<html>"));
                    client.printlb(F("<head>"));
                    client.printlb(F("</head>"));
                    client.printlb(F("<body>"));
                    client.printlb(F("<h1 align='center>DEPOSITO</h1><h3 align='center'>Nivel de agua</h3>"));

                    float ti;
                    float d;
                    digitalWrite(TrggerPin, HIGH);
                    delayMicroseconds(10);
                    digitalWrite(TriggerPin, LOW);
                    ti = pulseIn(EchoPin, HIGH);
                    d = ti/59;
                    nivel_del_agua = nivel_limite - d;

                    Serial.println(nivel_del_agua);
                    client.printlb(F("<br />"));
                    client.printlb(F("<head><title>Deposito</title></head>"));
                    client.printlb(F("<body><h1></h1><p>Nivel del agua: "));
                    client.printlb(F(nivel_del_agua));
                    client.printlb(F("<body><h1></h1><p>Umbral Alto: "));
                    client.printlb(Mb.MbData[3]);
                    client.printlb(F("<body><h1></h1><p>Umbral Bajo: "));
                    client.printlb(Mb.MbData[4]);
                    client.printlb(F("<body><h1></h1><p>Estado de la electrovalvula: "));
                    client.printlb(Estado);
                    client.printlb(F("<body>"));
                    client.printlb(F("<p><em> La pagina se actualzia cada 5 segundos.</em></p></body></html>"));
                    break;
                }

                if (c == '\n'){
                    currentLineIsBlank = true;
                }

                else if (c != '\r'){
                    currentLineIsBlank = false;
                }
            }
        }

        static long t = 0;
        PT_WAIT_WHILE(pt, (millis()-t)<1000);
        client.stop();
    }

    PT_END(pt);
}


/********** FUNCION DEL SENSOR ULTRASONIDOS **********/
void ultrasonidos(struct pt *pt){

    PT_BEGIN(pt);
    static long t = 0;
    float ti;
    float d;
    digitalWrite(TrggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(TriggerPin, LOW);
    ti = pulseIn(EchoPin, HIGH);
    d = ti/59;
    nivel_del_agua = nivel_limite - d;

    static long t = 0;
    PT_WAIT_WHILE(pt, (millis()-t)<10000);
    PT_END(pt);
}