#include <Dhcp.h>
#include <Dns.h>
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
#define ACcalibrationValue 6.666667
#define DCcalibrationValue 6.666667
#define ERROR (RGB){ HIGH, LOW, LOW };
#define LOADING (RGB){ HIGH, HIGH, LOW };
#define OK (RGB){ LOW, HIGH, LOW };

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

Sensors sensors;
EthernetServer server = EthernetServer(80);

void generateMacAddress() {
  Serial.println(F("Checking if we have a mac address in EEPROM"));
  if (EEPROM.read(1) == '#') {
    Serial.println(F("Found address in EEPROM, using that"));
    for (int i = 4; i < 6; i++) {
      mac[i] = EEPROM.read(i);
    }
  } else {
    Serial.println(F("Did not find address in EEPROM, generating a new one"));
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

void setup () {
  Serial.begin(57600);
  Serial.println(F("Booting up..."));
  initializeRGBdiode();
  pinMode(7, INPUT);
  pinMode(8, INPUT);

  if (digitalRead(7) && digitalRead(8)) {
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
  }
  generateMacAddress();
  if (Ethernet.begin(mac, (DhcpOptionParser *) &dhcpOptionParser, (DhcpOptionProvider *) &dhcpOptionProvider) == 0) {
    LED = ERROR;
    setRGBLEDColor();
  }
  server.begin();

  configureSensors();
  LED = OK;
  setRGBLEDColor();
  Serial.println(F("Finished booting, going into normal operations"));
}

void dhcpOptionParser(const uint8_t optionType, EthernetUDP *dhcpUdpSocket) {
  uint8_t opt_len = dhcpUdpSocket->read();
  while (opt_len--) {
    dhcpUdpSocket->read();
  }
}

void dhcpOptionProvider(const uint8_t messageType, EthernetUDP *dhcpUdpSocket) {
  char m[5];
  snprintf(m, 5, "%02x%02x", mac[4], mac[5]);
  uint8_t buffer[] = {
    dhcpClientIdentifier,
    0x06,
    'p', 'p', m[0], m[1], m[2], m[3],
  };
  dhcpUdpSocket->write(buffer, 8);
}

void configureSensors() {
  if (digitalRead(7) && !digitalRead(8)) {
    Serial.println(F("Setting up sensors in DC Mode"));
    sensors.one.current(3, DCcalibrationValue);
    sensors.two.current(5, DCcalibrationValue);
    sensors.three.current(4, DCcalibrationValue);
  } else {
    Serial.println(F("Setting up sensors in AC Mode"));
    sensors.one.current(3, ACcalibrationValue);
    sensors.two.current(5, ACcalibrationValue);
    sensors.three.current(4, ACcalibrationValue);
  }
}

static int readSensor(int sensor, int voltage = 230) {
  return random(10, 20);
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

void loop() {
  setRGBLEDColor();
  Ethernet.maintain();

  EthernetClient client = server.available();
  if (client == true) {
    char reply[64];
    server.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Closed\r\n\r\n"));
    sprintf(reply, "{\"uptime\": %lu, \"sensors\": {\"1\": %i, \"2\": %i, \"3\": %i}}\n", millis()/1000, readSensor(1), readSensor(2), readSensor(3));
    server.print(reply);
    client.stop();
  }
}
