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
#define calibrationValue 6.666667

typedef struct {
	EnergyMonitor one;
	EnergyMonitor two;
	EnergyMonitor three;
} Sensors;

Sensors sensors;

static byte mac[] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 };
EthernetServer server = EthernetServer(1337);

void dhcpOptionParser(const uint8_t optionType, EthernetUDP *dhcpUdpSocket) {
    uint8_t opt_len = dhcpUdpSocket->read();

    if (optionType == 15) { // domain name
        // read the value with dhcpUdpSocket->read()
    } else {
        while (opt_len--) {
            dhcpUdpSocket->read();
        }        
    }
}

void dhcpOptionProvider(const uint8_t messageType, EthernetUDP *dhcpUdpSocket) {
    uint8_t buffer[] = {
        dhcpClientIdentifier,
        0x08,
        'M','S','F','T',' ','5','.','0'
    };
    dhcpUdpSocket->write(buffer, 10);
}


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
	Ethernet.begin(mac, (DhcpOptionParser *) &dhcpOptionParser, (DhcpOptionProvider *) &dhcpOptionProvider);
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
  return random(10,20);
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

char var1, var2, var3 = "10";
char time = char(millis());
void loop() {
	Ethernet.maintain();

	EthernetClient client = server.available();
	if (client == true) {
    server.print("{'uptime': ");
    server.print(millis());
    server.print(", 'sensors': ");
    server.print(", {1:");
    server.print(readSensor(1));
    server.print(", 2: ");
    server.print(readSensor(2));
    server.print(", 3: ");
    server.print(readSensor(3));
    server.print("}}\n");
		client.stop();
	}
}
