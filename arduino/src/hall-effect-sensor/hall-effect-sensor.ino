#include <Dhcp.h>
#include <Dns.h>
#include <avr/pgmspace.h>
#include <ethernet_comp.h>
#include <UIPClient.h>
#include <UIPEthernet.h>
#include <UIPServer.h>
#include <UIPUdp.h>
#include <EmonLib.h>
#include <EEPROM.h>
#include <TrueRandom.h>
// Sample data for the mac address generation
#define samplePin 0
// Calibration value for the SCT-013-000s
#define ACcalibrationValue 22
#define DCcalibrationValue 22
#define voltage 230
#define ERROR (RGB){ HIGH, LOW, LOW };
#define LOADING (RGB){ HIGH, HIGH, LOW };
#define OK (RGB){ LOW, HIGH, LOW };
#define F_CPU 16000000UL

struct RGB {
  byte r;
  byte g;
  byte b;
};

typedef struct {
  EnergyMonitor one;
  EnergyMonitor two;
  EnergyMonitor three;
} Sensors;

RGB LED = ERROR;
static byte mac[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned long previous = millis();
unsigned long current = millis();

Sensors sensors;
EthernetServer server = EthernetServer(80);

void generateMacAddress() {
  if (EEPROM.read(1) == '#') {
    for (int i = 4; i < 6; i++) {
      mac[i] = EEPROM.read(i);
    }
  } else {
    randomSeed(analogRead(samplePin));
    for (int i = 4; i < 6; i++) {
      mac[i] = random(0, 255);
      EEPROM.write(i, mac[i]);
    }
    EEPROM.write(1, '#');
  }
}

void initializeRGBdiode() {
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  setRGBLEDColor();
}

void setRGBLEDColor() {
  digitalWrite(3, LED.r);
  digitalWrite(5, LED.g);
  digitalWrite(6, LED.b);
}

void blinkRGB() {
  for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
      digitalWrite(3, HIGH);
      digitalWrite(5, HIGH);
      digitalWrite(6, HIGH);
    } else {
      digitalWrite(3, LOW);
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
    }
    delay(100);
  }
}

void setup () {
  Serial.begin(57600);
  initializeRGBdiode();
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  if (digitalRead(7) && digitalRead(8)) { 
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
    blinkRGB();
  }

  generateMacAddress();

  if (Ethernet.begin(mac) == 0) {
    LED = ERROR;
    setRGBLEDColor();
  }
  server.begin();

  configureSensors();
  LED = OK;
  setRGBLEDColor();
}

void configureSensors() {
  if (digitalRead(7) && !digitalRead(8)) {
    sensors.one.current(3, DCcalibrationValue);
    sensors.two.current(5, DCcalibrationValue);
    sensors.three.current(4, DCcalibrationValue);
  } else {
    sensors.one.current(3, ACcalibrationValue);
    sensors.two.current(5, ACcalibrationValue);
    sensors.three.current(4, ACcalibrationValue);
  }
}

int readSensor(int sensor) {
  switch (sensor) {
    case 1:
      return sensors.one.calcIrms(1480) * voltage;
      break;
    case 2:
      return sensors.two.calcIrms(1480) * voltage;
      break;
    case 3:
      return sensors.three.calcIrms(1480) * voltage;
      break;
  }
}
char* intToCharFloat(int value) {
  char *buf = (char *) malloc(sizeof(char) * 6);
  dtostrf(value/100.0, 4, 2, buf);
  return buf;
}

void loop() {
  /* Ugly hack to solve that it goes bad after >2000 seconds of requests */
  current = millis();
  if (current-previous > 30000) {
    Enc28J60.init(mac);
    previous = millis();
  }
  setRGBLEDColor();
  Ethernet.maintain();

  EthernetClient client = server.available();
  if (client == true) {
    char reply[110];
    char* sensor1_value = intToCharFloat(readSensor(1));
    char* sensor2_value = intToCharFloat(readSensor(2));
    char* sensor3_value = intToCharFloat(readSensor(3));
    
    server.println(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Closed\r\n"));
    sprintf(reply, "uptime %lu\ncurrent{sensor=\"1\"} %s\ncurrent{sensor=\"2\"} %s\ncurrent{sensor=\"3\"} %s\n", millis()/1000, sensor1_value, sensor2_value, sensor3_value);
    server.print(reply);
    client.stop();
    free(sensor1_value);
    free(sensor2_value);
    free(sensor3_value);
  }
}
