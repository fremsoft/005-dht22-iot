/*
 * Questo programma per ESP-01S su scheda IoT, legge i valori di 
 * Temperatura e Umidità da DHT22 sul GPIO2.
 * 
 * Risolve anche il problema: Reading Negative Temperatures with DHT22
 * 
 * Con questo sketch si leggono sia temperature positive che negative, 
 * senza fare uso di librerie, e li trasmette via Wi-Fi a un servizio 
 * web attraverso protocollo HTTPS (crittografato).
 * 
 * Sketch basato sul programma di esempio "BasicHTTPSClient.ino"
 * 
 * La configurazione SSID e PASSWORD è memorizzata in memoria 
 * non volatile. Se il dispositivo all'accensione non riesce 
 * a connettersi alla rete Wi-Fi, diventa Soft-AP e attiva un 
 * servizio web con SSID "DHT22 IoT" e password "12345678", 
 * disponibile all'indirizzo 192.168.10.1 e con cui si può 
 * programmare il servizio mediante lo smartphone.
 * 
 * Sketch basato sul programma di esempio "WiFiAccessPoint"
 * 
 * Visualizza il progetto integrale open-source: 
 * https://youtu.be/O72t-QcfDvM
 * 
 */

#define DEFAULT_DEVICE_ID   "DHT22-LAB"
#define DEFAULT_WEBSERVICE  "https://fremsoft.it/service-dht.php"
#define DEFAULT_IP_ADDRESS   192,168,10,1

#define MIO_SSID    "DHT22 IoT"  
#define PASSWORD    "12345678"  

#define MAX_SOFTAP_TIME_MS   5*60*1000    /* 5 minuti */

#include <Arduino.h>

#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <WiFiClientSecureBearSSL.h>
// Fingerprint for demo URL, expires on June 2, 2021, needs to be updated well before this date
const uint8_t fingerprint[20] = {0x40, 0xaf, 0x00, 0x6b, 0xec, 0x90, 0x22, 0x41, 0x8e, 0xa3, 0xad, 0xfa, 0x1a, 0xe8, 0x25, 0x41, 0x1d, 0x1a, 0x54, 0xb3};

ESP8266WiFiMulti WiFiMulti;

int status;


#define DHTPIN              2
#define DHTTYPE         DHT22

#define TIMEOUT_DHT_MS      2

// Config 
#define PARAM_SIZE 64
struct Config
{
  char ssid[PARAM_SIZE];
  char pass[PARAM_SIZE];
  char device_id[PARAM_SIZE];
  char webservice[PARAM_SIZE];
} eeprom_config;

ESP8266WebServer server(80);

float temperatura, umidita;
unsigned char raw[5];

void eseguiLettura() {

  int i;
  int n_bit;
  int value;
  
  unsigned long t;
  
  
  // Sequenza di start : tengo basso per almeno 18ms
  pinMode( DHTPIN, OUTPUT );
  digitalWrite( DHTPIN, LOW );
  delay( 19 );
  pinMode( DHTPIN, INPUT );

  // Buttiamo via il primo bit
  // aspetta che la linea dati vada a zero
  t = millis() + TIMEOUT_DHT_MS;
  while ( (digitalRead( DHTPIN ) != 0) && (millis() <= t) ) /* do nothing */;
      
  // aspetta il fronte di salita
  t = millis() + TIMEOUT_DHT_MS;
  while ( (digitalRead( DHTPIN ) == 0) && (millis() <= t) ) /* do nothing */;

  
  // ascolto 40 bit: [8bit RH%][8bit 1/10RH%][8bit T°C][8bit 1/10T°C][CHECKSUM]
  for (i=0; i<5; i++) {
    // lettura del byte i-esimo
    raw[i] = 0;
    
    for (n_bit=0; n_bit<8; n_bit++) {

      // aspetta che la linea dati vada a zero
      t = millis() + TIMEOUT_DHT_MS;
      while ( (digitalRead( DHTPIN ) != 0) && (millis() <= t) ) /* do nothing */;
      
      // aspetta il fronte di salita
      t = millis() + TIMEOUT_DHT_MS;
      while ( (digitalRead( DHTPIN ) == 0) && (millis() <= t) ) /* do nothing */;
      
      // aspetta 50uSec 
      delayMicroseconds(50);
      
      // lettura della linea dati
      value = digitalRead( DHTPIN );
      raw[i] = raw[i] << 1;
      if (value != 0) { raw[i] = raw[i] | 0x01; }
    }
  }
  
  // controllo checksum ed eventuale scrittura dei valori letti
  /*
  Serial.print (raw[0]);
  Serial.print (" ");
  Serial.print (raw[1]);
  Serial.print (" ");
  Serial.print (raw[2]);
  Serial.print (" ");
  Serial.print (raw[3]);
  Serial.print (" ");
  Serial.println(raw[4]);
  */

  if ( ((raw[0] + raw[1] + raw[2] + raw[3]) & 0xFF) == raw[4] ) {
    // la lettura è corretta
    long t_int = ((raw[2] << 8) | raw[3]);
    if ((raw[2] & 0x80) == 0) { 
      /* numero positivo */
      temperatura = t_int / 10.0;      
    }
    else {
      /* numero negativo */
      temperatura = (0-( ~t_int + 1)) / 10.0 ;
    }
          
    /*
     *  SE IL SIGNED INT E' UN INTERO A 16 BIT SI 
     *  PUO' FARE PIU' SEMPLICEMENTE COSI':
     *  
     *  temperatura = (signed int)((raw[2] << 8) | raw[3]);
     *  temperatura = temperatura / 10.0; 
     * 
     */
    umidita     = ((raw[0] << 8) | raw[1]) / 10.0;
  }
}

// Print config
void printConfig()
{
  
  Serial.println();
  Serial.print("SSID: ");        Serial.println(eeprom_config.ssid);
  Serial.print("Password: ");    Serial.println(eeprom_config.pass);
  Serial.print("Device ID: ");   Serial.println(eeprom_config.device_id);
  Serial.print("Web-service: "); Serial.println(eeprom_config.webservice);
}

// Load config from EEPROM
void loadConfig()
{
  Serial.println("Loading configuration");

  long i = 0;
  int  j;
  for (j=0; j<PARAM_SIZE; j++) { eeprom_config.ssid[j] = EEPROM.read(i++); } eeprom_config.ssid[j-1] = 0; 
  for (j=0; j<PARAM_SIZE; j++) { eeprom_config.pass[j] = EEPROM.read(i++); } eeprom_config.pass[j-1] = 0;
  for (j=0; j<PARAM_SIZE; j++) { eeprom_config.device_id[j] = EEPROM.read(i++); } eeprom_config.device_id[j-1] = 0;
  for (j=0; j<PARAM_SIZE; j++) { eeprom_config.webservice[j] = EEPROM.read(i++); } eeprom_config.webservice[j-1] = 0;
  
  printConfig();
}

// Save config to EEPROM
void saveConfig()
{
  Serial.println("Saving configuration");

  long i = 0;
  int j;
  
  /* Ciclo di ERASE */
  for (j = 0; j < 512; j++) { EEPROM.write(j, 0); }

  /* Scrittura */ 
  for (j=0; j<PARAM_SIZE; j++) { EEPROM.write(i++, eeprom_config.ssid[j]); }
  for (j=0; j<PARAM_SIZE; j++) { EEPROM.write(i++, eeprom_config.pass[j]); }
  for (j=0; j<PARAM_SIZE; j++) { EEPROM.write(i++, eeprom_config.device_id[j]); }
  for (j=0; j<PARAM_SIZE; j++) { EEPROM.write(i++, eeprom_config.webservice[j]); }

  EEPROM.commit();

  Serial.println("Configuration saved");
  printConfig();
}

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  EEPROM.begin(512);
  loadConfig();

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(eeprom_config.ssid, eeprom_config.pass);

  pinMode( DHTPIN, INPUT );

  Serial.print("Waiting for connection.");
  int tries = 0;
  while ((WiFiMulti.run() != WL_CONNECTED) && (tries++ < 3)) { Serial.print("."); }

  if (tries < 3) {
    status = 1; /* loop ok */
    Serial.println();
    Serial.println("Connected");
  }
  else {
    status = 0; /* setup Soft-AP */
    Serial.println();
    Serial.println("NOT connected");
  }
}

void loop_ok() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    // client->setFingerprint(fingerprint);
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
    client->setInsecure();

    HTTPClient https;
    
    Serial.print("[HTTPS] begin...\n");

    eseguiLettura();
    Serial.print ( raw[0], HEX );
    Serial.print ( raw[1], HEX );
    Serial.print ( "," );
    Serial.print ( raw[2], HEX );
    Serial.print ( raw[3], HEX );
    Serial.print ( "," );
    Serial.print ( raw[4], HEX );
    Serial.print ( " - " );
    Serial.print ( "Temperatura: " );
    Serial.print ( temperatura );
    Serial.print ( "°C, Umidità: " );
    Serial.print ( umidita );
    Serial.println( "%" );
  
    String url;
    url = eeprom_config.webservice;
    url = url + "?device=";
    url = url + eeprom_config.device_id;
    url = url + "&t=";
    url = url + String(temperatura);
    url = url + "&h=";
    url = url + String(umidita);
    url.replace(" ", "%20");    // urlencode molto semplice solo per gli spazi
    
    if (https.begin(*client, url)) {  // HTTPS

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %d %s\n", httpCode, https.errorToString(httpCode).c_str());
      }

      https.end();
      delay(500);
      
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }

  Serial.println("Wait 10s before next round...");
  delay(10000);
}

void handleRoot() {
  bool save_config = false;

  if (server.hasArg("ssid") && (server.arg("ssid").length() < PARAM_SIZE)) {
    String t = server.arg("ssid");
    t.trim();
    t.toCharArray(eeprom_config.ssid, PARAM_SIZE);
    save_config = true;
  }
  if (server.hasArg("pass") && (server.arg("pass").length() < PARAM_SIZE)) {
    String t = server.arg("pass");
    t.trim();
    t.toCharArray(eeprom_config.pass, PARAM_SIZE);
    save_config = true;
  }
  if (server.hasArg("device_id") && (server.arg("device_id").length() < PARAM_SIZE)) {
    String t = server.arg("device_id");
    t.trim();
    t.toCharArray(eeprom_config.device_id, PARAM_SIZE);
    save_config = true;
  }
  if (server.hasArg("webservice") && (server.arg("webservice").length() < PARAM_SIZE)) {
    String t = server.arg("webservice");
    t.trim();
    t.toCharArray(eeprom_config.webservice, PARAM_SIZE);
    save_config = true;
  }

  if (save_config) { saveConfig(); }

  String html = "<!DOCTYPE html>";
  html += "<html lang=\"it\">";
  html += "  <head>";
  html += "    <title>" + String(MIO_SSID) + "</title>";
  html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\" />";
  html += "    <meta charset=\"utf-8\" />";
  html += "  </head>";
  html += "  <body>";
  html += "    <h1><strong>Configurazione " + String(MIO_SSID) + "</h1>";
  html += "    <form action=\"/\" method=\"post\" enctype=\"multipart/form-data\" data-ajax=\"false\">";
  html += "      <label for=\"ssid\">SSID:</label>";
  html += "      <input id=\"ssid\" type=\"text\" size=\"16\" name=\"ssid\" value=\"" + String(eeprom_config.ssid) + "\" placeholder=\"" + String(MIO_SSID) + "\" />";
  html += "      <br />";
  html += "      <label for=\"pass\">Password:</label>";
  html += "      <input id=\"pass\" type=\"text\" size=\"16\" name=\"pass\" value=\"" + String(eeprom_config.pass) + "\" placeholder=\"" + String(PASSWORD) + "\" />";
  html += "      <br />";
  html += "      <label for=\"device_id\">Device ID:</label>";
  html += "      <input id=\"device_id\" type=\"text\" size=\"16\" name=\"device_id\" value=\"" + String(eeprom_config.device_id) + "\" placeholder=\"" + String(DEFAULT_DEVICE_ID) + "\" />";
  html += "      <br />";
  html += "      <label for=\"webservice\">Web Service:</label>";
  html += "      <input id=\"webservice\" type=\"text\" size=\"64\" name=\"webservice\" value=\"" + String(eeprom_config.webservice) + "\" placeholder=\"" + String(DEFAULT_WEBSERVICE) + "\" />";
  html += "      <br />";
  html += "      <input id=\"submit-button\" type=\"submit\" value=\"SAVE\" />";
  html += "    </form>";
  html += "    <br />";
  html += "    <br />";
  html += "    <br />";
  html += "    <a href=\"/reboot\">REBOOT</a>";
  html += "  </body>";
  html += "</html>";

  server.sendHeader("Connection", "close");
  server.send(200, "text/html", String(html));
}

void handleReboot() { 
  Serial.println("Reboot required!");
  
  String html = "<!DOCTYPE html>";
  html += "<html lang=\"it\">";
  html += "  <head>";
  html += "    <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />";
  html += "    <title>" + String(MIO_SSID) + "</title>";
  html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\" />";
  html += "    <script>setTimeout(function() { window.location.href = \"/\"; }, 10000);</script>";
  html += "  </head>";
  html += "  <body>";
  html += "    <h1><strong>Configurazione " + String(MIO_SSID) + "</h1>";
  html += "    Sto riavviando il dispositivo...";
  html += "  </body>";
  html += "</html>";

  server.sendHeader("Connection", "close");
  server.send(200, "text/html", String(html));

  delay(1000);
  ESP.restart();
}

void loop() {
  if (status == 1) { 
    loop_ok();
  }
  else {
    /* se all'accensione non è riuscito a configurare una rete Wi-Fi si configura in modalità Soft-AP */
    if (status == 0) {
      /* setup Soft-AP */
      delay(1000);
      Serial.println();
      Serial.print("Configuring access point...");
 
      IPAddress local_IP(DEFAULT_IP_ADDRESS);
      IPAddress gateway(192, 168, 10, 254);
      IPAddress subnet(255, 255, 255, 0);
      String result1 = (WiFi.softAPConfig(local_IP, gateway, subnet)) ? ("IP Address Ok") : ("Failed!");
      WiFi.mode(WIFI_AP);
      WiFi.hostname(MIO_SSID);
      String result2 = (WiFi.softAP(MIO_SSID, PASSWORD)) ? ("SoftAP Ready") : ("Failed!");

      Serial.println("AP Start");
      Serial.print("AP SSID: ");
      Serial.println(MIO_SSID);
      Serial.print("AP password: ");
      Serial.println(PASSWORD);
      Serial.print("Setting soft-AP configuration ... ");
      Serial.println( result1 );
      Serial.println( result2 );

      IPAddress myIP = WiFi.softAPIP();
      Serial.print("IP address: ");
      Serial.println(myIP);
      
      server.on("/", handleRoot);
      server.on("/reboot", handleReboot);
      server.begin();
      Serial.println("HTTP server started");

      status = 2;
    }
    else {
       /* per 5 minuti resta in ascolto poi si resetta */
       server.handleClient();

       if (millis() > MAX_SOFTAP_TIME_MS) { ESP.restart(); }
    }
  }
}
