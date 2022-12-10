#include "WiFiManager.h"
#include <DNSServer.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ezButton.h>



#define   kFirmWare                   "ESP32_MQTT_PUSH"

WiFiManager manager;   
WiFiClient espClient;
PubSubClient client(espClient);

// CONFIGURACION MQTT 
const char* mqtt_server = "<servidor mqtt>";
const char* user_mqtt   = "<usuario>";
const char* pass_mqtt   = "<pass>"; 
uint16_t    mqtt_port   = 1700; //puerto mqtt

const int   PIN_RELE    = 5;
const int   PIN_BUTTON  = 21;
const int   PIN_LED_ST  = 19;
const int   PIN_CONFIG  = 25;

ezButton button(PIN_BUTTON);

const char* topico_id_push     =  "esp_32/push/1"; //topico_subscriptor
int intentos_mqtt = 0;


unsigned long tiempo_led;
unsigned long currentMillis;
//int send_push = 0;

uint8_t baseMac[6];
char macserial[13] = {0};
bool success;

//declaracion de metodos
void leds_blink(int toff, int ton);
void leds_delay(int cantidad, int tiempo);
void setup_wifi();
void reconnect_mqtt();
void reset_setting_wifi();
void send_push();


//Configuracion Inicial
void setup() {
  Serial.begin(115200);
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_LED_ST, OUTPUT);
  pinMode(PIN_CONFIG, INPUT);
  //pinMode(PIN_BUTTON, INPUT);
  button.setDebounceTime(100);
  
  digitalWrite(PIN_RELE, LOW);
  digitalWrite(PIN_LED_ST, LOW);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}
//Setup del WIFI 
void setup_wifi() {
  delay(100);
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  sprintf(macserial, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  Serial.print("->> SERIAL BASADO EN MAC: ");
  Serial.println(macserial);
  leds_delay(50,100); //5 segundos
  Serial.println();
  Serial.println("->> CONECTANDO A LA RED");
  
  manager.setTimeout(120);
  while (!manager.autoConnect(macserial,"SSS.2121")){
    Serial.print("->> RESULTADO: " );
    Serial.println(manager.autoConnect(macserial,"SSS.2121"));
    manager.autoConnect(macserial,"SSS.2121");
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (digitalRead(PIN_CONFIG)==HIGH){ 
      reset_setting_wifi();
    }
  }
  Serial.println("");
  Serial.println("->> WIFI CONECTADO EXITOSAMENTE");
  Serial.print("->> DIRECCION IP: ");
  Serial.println(WiFi.localIP());

}
// Callback del servidor que recibe la data del MQTT
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("->> ATENCION RECIBIENDO INFORMACION DEL TOPICO: ");
  Serial.println(topic);
  Serial.print("->> MENSAJE: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == topico_id_access) {
    Serial.println("->> VALIDANDO AUTORIZACION DE INGRESO");
    if(messageTemp == "ACCESO"){
      Serial.println("->> ACCESO AUTORIZADO");
      digitalWrite(PIN_RELE, HIGH);
      delay(6000);
      digitalWrite(PIN_RELE, LOW);
    }
  }
} 
//Metodo para conectar y reconetar con el servidor MQTT
void reconnect_mqtt() {
  while (!client.connected()) {
    if (digitalRead(PIN_CONFIG)==HIGH){ 
      reset_setting_wifi();
    }
    Serial.println("->> INTENTANDO CONECTAR CON EL SERVIDOR MQTT");
    // Attempt to connect
    if (client.connect(macserial,user_mqtt,pass_mqtt)) {
      Serial.println("->> CONEXION EXITOSA");
      client.subscribe(topico_id_access);
      Serial.println("->> SUBSCRIPCION EXITOSA");
      
    } else {
      Serial.print("->> ERROR, rc=");
      Serial.println(client.state());
      Serial.println("->> ERROR DE CONEXION INTENTANDO EN 5 SEGUNDOS...");
      leds_delay(40,50); //2 segundos
      intentos_mqtt = intentos_mqtt+1;
      if (intentos_mqtt >= 10){intentos_mqtt = 0; Serial.println("->> INTERNET DESCONECTADO :( INTENTANDO RECONECTAR"); setup_wifi();}
    }
  }
}
//Metodo para resetear la configuracion del dispositivo
void reset_setting_wifi(){
    Serial.println("->> BORRANDO LA CONFIGURACION ANTIGUA..");
    leds_delay(20,300); //2 segundos
    manager.resetSettings();
    success = manager.autoConnect(macserial,"SSS.2121");
    if (success){
      leds_delay(20,300); //2 segundos
      ESP.restart();
    }
}

void send_push(){
    Serial.print("->> ENVIANDO: ");
    sprintf(macserial, "%02X%02X%02X%02X%02X%02X", 0x12, 0x22, 0xA2, 0x32, 0xB2, 0x82);
    
    String  mensaje_push = "PUSH-" + String(macserial);
    Serial.println(mensaje_push);
    char charBuf[50];
    mensaje_push.toCharArray(charBuf, 50);
    client.publish(topico_id_push, charBuf );
}

//Metodo para activar los led con delay
void leds_delay(int cantidad, int tiempo){
   int i=0;
   while (i < cantidad){
      digitalWrite(PIN_LED_ST, !digitalRead(PIN_LED_ST));
      delay(tiempo);
      i+=1;
  }digitalWrite(PIN_LED_ST, LOW);
}
//Metodo para hacer un blink del led sin delay 
void leds_blink(int toff, int ton){
  currentMillis = millis();
  if (millis()%toff==0){
    tiempo_led = millis();
    digitalWrite(PIN_LED_ST, HIGH);
  }
  if (currentMillis - tiempo_led >= ton) {
    digitalWrite(PIN_LED_ST, LOW);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  button.loop();
  if (digitalRead(PIN_CONFIG)==HIGH){ 
      reset_setting_wifi();
  }
  
  if(button.isReleased()){
    send_push();
  }
  leds_blink(20000,100);
}
