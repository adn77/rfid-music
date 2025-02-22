/*
Name:		rfid_arduino_nano.ino
Arduino Pro Mini w/ATmega328 3.3V 8 Mhz
Created:	3/21/2017 12:03:49 AM
Author:	ab
*/

/*
* --------------------------------------------------------------------------------------------------------------------
* Example sketch/program showing how to read data from a PICC to serial.
* --------------------------------------------------------------------------------------------------------------------
* This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
*
* Example sketch/program showing how to read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
* Reader on the Arduino SPI interface.
*
* When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
* then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
* you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
* will show the ID/UID, type and any data blocks it can read. Note: you may see "Timeout in communication" messages
* when removing the PICC from reading distance too early.
*
* If your reader supports it, this sketch/program will read all the PICCs presented (that is: multiple tag reading).
* So if you stack two or more PICCs on top of each other and present them to the reader, it will first output all
* details of the first and then the next PICC. Note that this may take some time as all data blocks are dumped, so
* keep the PICCs at reading distance until complete.
*
* @license Released into the public domain.
*
* Typical pin layout used:
* -----------------------------------------------------------------------------------------
*             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
*             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
* Signal      Pin          Pin           Pin       Pin        Pin              Pin
* -----------------------------------------------------------------------------------------
* RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
* SPI SS      SDA(SS)      10            53        D10        10               10
* SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
* SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
* SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

// NOTE: pinout is Arduino Nano v3
#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <LowPower.h>
#include <SPI.h>
#include <MFRC522.h>
#include "simpleserial.h"

#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above
#define ESP_ENABLE_PIN  6
#define LED_PIN			2			// Red LED

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
SoftwareSerial swSerial(8,7);
int ticks = 0;

void startecho()
{
	while (true)
	{
		if (swSerial.available())
			Serial.println(swSerial.read());
	}
}

bool sendtoESP(String cmd, String param)
{
	SimpleSerial simpleserial(swSerial);
	COMMAND command;
	bool bres = false;

	// reset ESP, blink the LED
	digitalWrite(LED_PIN, HIGH);
	digitalWrite(ESP_ENABLE_PIN, LOW);
	delay(300);
	digitalWrite(ESP_ENABLE_PIN, HIGH);
	digitalWrite(LED_PIN, LOW);
	//startecho();
	// wait for the 'y' command for 2 seconds

	simpleserial.setTimeout(200);
	for(int i=0;i<10;i++)
	{
//		Serial.println("Waiting for ESP...");
		if (!simpleserial.recieveCmd(command))
			continue;
		if (command.cmd == "u")
		{
			bres = true;
			break;
		}
		return false;
	}
	if (!bres)
		return bres;
	// ready to send command
	simpleserial.sendCmd(cmd,param);
	//startecho();
	// get answer
	simpleserial.setTimeout(200);
	for (int i = 0; i < 10; i++)
	{
		if (!simpleserial.recieveCmd(command))
			continue;

		if (command.cmd == "u")
			bres = true;
		else if (command.cmd == "d")
			break;
		else if (command.cmd == "q")
		{
			//swSerial.end();
			return bres;
		}
		else
			break;
	}
	//swSerial.end();
	return false;
}

void mfrc522_fast_Reset()
{
	digitalWrite(RST_PIN, HIGH);
	mfrc522.PCD_Reset();
	mfrc522.PCD_WriteRegister(mfrc522.TModeReg, 0x80);		// TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
	mfrc522.PCD_WriteRegister(mfrc522.TPrescalerReg, 0x43);		// 10μs.
//	mfrc522.PCD_WriteRegister(mfrc522.TPrescalerReg, 0x20);		// test

	mfrc522.PCD_WriteRegister(mfrc522.TReloadRegH, 0x00);		// Reload timer with 0x01E = 30, ie 0.3ms before timeout.
	mfrc522.PCD_WriteRegister(mfrc522.TReloadRegL, 0x1E);

	mfrc522.PCD_WriteRegister(mfrc522.TxASKReg, 0x40);		// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
	mfrc522.PCD_WriteRegister(mfrc522.ModeReg, 0x3D);		// Default 0x3F. Set the preset value for the CRC coprocessor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)

	mfrc522.PCD_AntennaOn();					// Enable the antenna driver pins TX1 and TX2 (they were disabled by the reset)
}

void setup() {
	Serial.begin(115200);		// Initialize serial communications with the PC
	Serial.println("Starting up...");
	//while (!Serial) delay(1);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
	swSerial.begin(19200);

	pinMode(ESP_ENABLE_PIN, OUTPUT);
	pinMode(RST_PIN, OUTPUT);
	pinMode(SS_PIN, OUTPUT);
	pinMode(LED_PIN, OUTPUT);

	digitalWrite(ESP_ENABLE_PIN, LOW);

	// init reader
	SPI.begin();			// Init SPI bus
	//SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

	digitalWrite(SS_PIN, HIGH);
	digitalWrite(RST_PIN, HIGH);

	Serial.println("Setup RC522...");
	mfrc522.PCD_Init();
	if (mfrc522_fastDetect())
	{
		Serial.println("Initializing ESP-01");
		// Init wifi network
		sendtoESP("i", "");	// start AP and the web to reconfigure WiFi
		delay(120000);		// allow 2 min to reconfigure WiFi
	}
	Serial.println("Resetting RC522...");
	mfrc522_fast_Reset();
	Serial.println("Setup done...");
}

long readVcc() {
	// Read 1.1V reference against AVcc
	// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
	ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	ADMUX = _BV(MUX3) | _BV(MUX2);
#else
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  

	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA, ADSC)); // measuring

	uint8_t low = ADCL; // must read ADCL first - it then locks ADCH  
	uint8_t high = ADCH; // unlocks both

	long result = (high << 8) | low;

	result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	return result; // Vcc in millivolts
}

String measurepower()
{
	return String(readVcc());
}

bool mfrc522_fastDetect()
{
	byte bufferATQA[2];
	byte bufferSize = sizeof(bufferATQA);
	MFRC522::StatusCode sta = mfrc522.PICC_RequestA(bufferATQA, &bufferSize);
	return (sta == MFRC522::STATUS_OK || sta == MFRC522::STATUS_COLLISION);
}

bool mfrc522_fastDetect2()
{
	byte bufferATQA[2];
	byte bufferSize = sizeof(bufferATQA);
	MFRC522::StatusCode sta = mfrc522.PICC_RequestA(bufferATQA, &bufferSize);
	return (sta != MFRC522::STATUS_TIMEOUT);
}

bool mfrc522_fastDetect3()
{
	byte validBits = 7;
	MFRC522::StatusCode status;
	byte command = MFRC522::PICC_CMD_REQA;
	byte waitIRq = 0x30;		// RxIRq and IdleIRq
	byte n;
	uint16_t i;

	mfrc522.PCD_ClearRegisterBitMask(MFRC522::CollReg, 0x80);			// ValuesAfterColl=1 => Bits received after collision are cleared.

	mfrc522.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Idle);		// Stop any active command.
	mfrc522.PCD_WriteRegister(MFRC522::ComIrqReg, 0x7F);				// Clear all seven interrupt request bits
	mfrc522.PCD_SetRegisterBitMask(MFRC522::FIFOLevelReg, 0x80);			// FlushBuffer = 1, FIFO initialization
	mfrc522.PCD_WriteRegister(MFRC522::FIFODataReg, 1, &command);			// Write sendData to the FIFO
	mfrc522.PCD_WriteRegister(MFRC522::BitFramingReg, validBits);			// Bit adjustments
	mfrc522.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Transceive);	// Execute the command
	mfrc522.PCD_SetRegisterBitMask(MFRC522::BitFramingReg, 0x80);			// StartSend=1, transmission of data starts

	i = 20;
	while (1) {
		n = mfrc522.PCD_ReadRegister(MFRC522::ComIrqReg);	// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
		if (n & waitIRq) {					// One of the interrupts that signal success has been set.
			break;
		}
		if (n & 0x01) {						// Timer interrupt - nothing received in 25ms
			return false;
		}
		if (--i == 0) {						// The emergency break. If all other conditions fail we will eventually terminate on this one after 35.7ms. Communication with the MFRC522 might be down.
			return false;
		}
	}
	return true;
}

void loop() {
	ticks++;
	// wake up NFC
	mfrc522.PCD_SoftPowerUp();
	// Look for new cards
	if(!mfrc522_fastDetect3()){
		// put NFC to sleep
		mfrc522.PCD_SoftPowerDown();
		if (ticks > 24 * 60 * 60)
		{
			ticks = 0;
			sendtoESP("b", measurepower());
		}
	}
	else
	{
		bool status = mfrc522.PICC_ReadCardSerial();
		Serial.println("got something");
		mfrc522.PCD_SoftPowerDown();
		if (status)
		{
			// Serial.println("ready...");
			// Now a card is selected. The UID and SAK is in mfrc522.uid.
			// Dump UID
			Serial.print(F("Card UID:"));
			String id = "";
			for (byte i = 0; i < mfrc522.uid.size; i++) {
				id = id + String(mfrc522.uid.uidByte[i], HEX);
			}
			Serial.println(id);
			bool cmdresult = sendtoESP("t", id);
	//		Serial.println(cmdresult);
		}
	}
	Serial.flush();
	swSerial.flush();
	// Enter power down state for 1 s with ADC and BOD module disabled
	LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
	//LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
}
