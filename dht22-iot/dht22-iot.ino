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
 * Visualizza il progetto integrale open-source: 
 * https://youtu.be/O72t-QcfDvM
 * 
 */

#define DEVICE_ID   "DHT22-LAB"
#define WEBSERVICE  "https://fremsoft.it/service-dht.php"
#define MIO_SSID    "********"  // your SSID here 
#define PASSWORD    "********"  // the password here


#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClientSecureBearSSL.h>
// Fingerprint for demo URL, expires on June 2, 2021, needs to be updated well before this date
const uint8_t fingerprint[20] = {0x40, 0xaf, 0x00, 0x6b, 0xec, 0x90, 0x22, 0x41, 0x8e, 0xa3, 0xad, 0xfa, 0x1a, 0xe8, 0x25, 0x41, 0x1d, 0x1a, 0x54, 0xb3};

ESP8266WiFiMulti WiFiMulti;


#define DHTPIN              2
#define DHTTYPE         DHT22

#define TIMEOUT_DHT_MS      2

float temperatura, umidita;


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

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(MIO_SSID, PASSWORD);

  pinMode( DHTPIN, INPUT );

}


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


void loop() {
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
    url = WEBSERVICE;
    url = url + "?device=";
    url = url + DEVICE_ID;
    url = url + "&t=";
    url = url + String(temperatura);
    url = url + "&h=";
    url = url + String(umidita);
    
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
