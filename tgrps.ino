//
//  Angara.Net thermo-gprs sensor
//
//  DS18B20, Arduino Uno R3, IComSat v1.1 (SIM900)
//  http://github.com/maxp/tgprs
//

// Libraries:
//
//  http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
//  http://code.google.com/p/gsm-shield-arduino/


char VERSION[] = "tgprs v0.1";

#define HOST "rs.angara.net"
#define PORT 80
#define URI  "/dat?hwid=%s&t=%s&cycle=%d"

#define INTERVAL    600    // sec

#define APN   ""
#define USER  ""
#define PASS  ""

// "GSM.h/GSM.cpp":
// GSM_TX    2
// GSM_RX    3
// GSM_RESET 8
// GSM_ON    9

#define ONEWIRE    10
#define LED_GREEN  11
#define LED_RED    12


#include <OneWire.h>
#include <SoftwareSerial.h>
#include "SIM900.h"
#include "inetGSM.h"

int cycle;

OneWire ds(ONEWIRE);
InetGSM inet;

#define IMEI_LEN 20

char tchar[10];
char imei[IMEI_LEN+1];

char ubuff[40];
char rbuff[50];


void setup() {
  pinMode(GSM_ON, OUTPUT);
  pinMode(GSM_RESET, OUTPUT);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);  
  delay(1000);
  
  Serial.begin(9600);
  Serial.println(VERSION);
  cycle = 0;
  
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);  

}

void sim_power() {
  pinMode(GSM_ON, OUTPUT);
  digitalWrite(GSM_ON, LOW);
  delay(1000);
  digitalWrite(GSM_ON, HIGH);
  delay(1200);
  digitalWrite(GSM_ON, LOW);
  delay(2500);
}

void sim_reset() {
  pinMode(GSM_RESET, OUTPUT);
  digitalWrite(GSM_RESET, LOW);
  delay(100);
  digitalWrite(GSM_RESET, HIGH);
  delay(200);
  digitalWrite(GSM_RESET, LOW);
  delay(100);
}

void read_t() {
  byte data[12];
  
  digitalWrite(LED_GREEN, HIGH);
  ds.reset(); ds.skip(); ds.write(0x44, 1); // measure
  delay(999); // delay > 750
  ds.reset(); ds.skip(); ds.write(0xBE); // read scratchpad

  Serial.print("\nscratch:");  
  for(int i=0; i < 9; i++) {
    data[i] = ds.read(); Serial.print(" "); Serial.print(data[i], HEX);
  }
  Serial.println();
  
  digitalWrite(LED_GREEN, LOW);
  
  if(data[8] != OneWire::crc8(data, 8)) {
    // t = "XXX"
    tchar[0] = 'X'; tchar[1] = 'X'; tchar[2] = 'X'; tchar[3] = 0; 
    Serial.println("!CRC\n");
    return;
  }
  
  int t = ((int)data[1] << 8) + (int)data[0]; t = t*6+t/4;  // *6.25
  
  byte sign;
  if( t < 0 ) { sign =  1; t = -t; } else { sign = 0; }
  
  int h = t %100; t = t / 100;

  if(sign) { sprintf(tchar, "-%d.%02d", t, h); }
  else { sprintf(tchar, "%d.%02d", t, h); }

  Serial.print("t: "); Serial.println(tchar); Serial.println();
}

void send_t() {
  Serial.println("send_t");   
  
  gsm.begin(9600);
 
  int rc;
  rc = gsm.SendATCmdWaitResp("AT+GSN", 500, 100, "OK", 1);  // == 1
  
  int i = 0, j = 0; 
  byte c; 
  while((c = gsm.comm_buf[i]) && i < 1000) {
    if( c <= 32 ) { i++; } else { break; }
  }
  while( (c = gsm.comm_buf[i]) && (j < IMEI_LEN) ) {
    if( c <= 32 ) { break; }
    imei[j++] = c; i++;
  }
  imei[j] = 0;
  
  Serial.print("\nimei: "); Serial.println(imei);

  digitalWrite(LED_GREEN, HIGH);  
  inet.attachGPRS(APN, USER, PASS);
  delay(2000);
  
  gsm.SimpleWriteln("AT+CIFSR");
  delay(5000);
  
  sprintf(ubuff, URI, imei, tchar, cycle);

  Serial.print("\nurl: "); Serial.println(ubuff);
  
  inet.httpGET(HOST, PORT, ubuff, rbuff, 50); 
  digitalWrite(LED_GREEN, LOW);  
  Serial.print("\nresp: "); Serial.println(rbuff);
  
  sim_reset();  
}

void pause(int sec) {
  for( int i=0; i < sec; i++){
    delay(500);
    if( i % 4 == 0 ) {
      digitalWrite(LED_RED, HIGH);
      delay(5);
      digitalWrite(LED_RED, LOW);
      delay(40);
      digitalWrite(LED_GREEN, HIGH);
      delay(5);
      digitalWrite(LED_GREEN, LOW);
      delay(450);
    }
    else {
      delay(500);
    }
  }  
}

void loop() {
  Serial.print("\ncycle: "); Serial.println(cycle);
  read_t();
  send_t();
  pause(INTERVAL);
  cycle += 1;
}

//.

