#include <Ethernet.h>
#include <EmonLib.h>
#include <EEPROM.h>
#include <TrueRandom.h>
// Sample data for the mac address generation
#define samplePin 0
// Calibration value for the SCT-013-000s
#define calibrationValue 6.666667

typedef struct {
	EnergyMonitor one;
	EnergyMonitor two;
	EnergyMonitor three;
} Sensors;

Sensors sensors;

static byte mac[] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 };
EthernetServer server = EthernetServer(1337);

void setup () {
	Serial.begin(57600);
	Serial.println("Booting up...");

	if (EEPROM.read(1) == '#') {
		for (int i = 3; i < 6; i++) {
	    	mac[i] = EEPROM.read(i);
	    }
	} else {	
		randomSeed(analogRead(samplePin));
		for (int i = 3; i < 6; i++) {
			mac[i] = random(0, 255);
			EEPROM.write(i, mac[i]);
		}
		EEPROM.write(1, '#');
	}
  // We need to supply Client ID to the DHCP Request so that we have some way to identify and find the circuit
	Ethernet.begin(mac);
	server.begin();

	configureSensors();
	Serial.println(" done");
	Serial.println("Ready to serve!");
	return;
}

void configureSensors() {
	sensors.one.current(3, calibrationValue);
	sensors.two.current(5, calibrationValue);
	sensors.three.current(4, calibrationValue);
}


static int readSensor(int sensor, int voltage = 230) {
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
char reply;
void loop() {
	Ethernet.maintain();

	EthernetClient client = server.available();
	if (client == true) {
		sprintf(reply, "{'sensors': 'uptime': '%d', {1: %d, 2: %d, 3: %d'}}",millis(),readSensor(1), readSensor(2), readSensor(3));
		server.write(reply);
		client.stop();
	}
}
