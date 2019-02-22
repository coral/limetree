/***
 * hello
 */

#include <Adafruit_NeoPixel.h>
#include <bluefruit.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>


#define pn532_ss		(16)
#define numpixels		32

Adafruit_PN532 nfc(pn532_ss);

BLEUart bleuart;

// function prototypes for packetparser.cpp
uint8_t readPacket (BLEUart *ble_uart, uint16_t timeout);
float   parsefloat (uint8_t *buffer);
void    printHex   (const uint8_t * data, const uint32_t numBytes);

// packet buffer
extern uint8_t packetbuffer[];

uint8_t bg = 2;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numpixels, 27, NEO_GRB + NEO_KHZ800);

void setup(void)
{
	Serial.begin(115200);

	Bluefruit.begin();
	
	// set max power. accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
	Bluefruit.setTxPower(4);
	Bluefruit.setName("ATPSCAN");

	// configure and start the ble uart service
	bleuart.begin();

	// set up and start Advertising
	startAdv();

  Bluefruit.autoConnLed(false);

	Serial.begin(115200);
	Serial.println("starting!");

	nfc.begin();

	uint32_t versiondata = nfc.getFirmwareVersion();
	if (! versiondata) {
		Serial.print("didn't find pn53x board");
		while (1); // halt
	}
	// got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
	// configure board to read rfid tags
	nfc.SAMConfig();

	//start neopixel
	pixels.begin();
	
}

void startAdv(void)
{
	// Advertising packet
	Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
	Bluefruit.Advertising.addTxPower();
	
	// include the ble uart (aka 'nus') 128-bit uuid
	Bluefruit.Advertising.addService(bleuart);

	// secondary scan response packet (optional)
	// since there is no room for 'name' in Advertising packet
	Bluefruit.ScanResponse.addName();

	/* start Advertising
	 * - enable auto Advertising if disconnected
	 * - interval:  fast mode = 20 ms, slow mode = 152.5 ms
	 * - timeout for fast mode is 30 seconds
	 * - start(timeout) with timeout = 0 will advertise forever (until connected)
	 */
	
	Bluefruit.Advertising.restartOnDisconnect(true);
	Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
	Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
	Bluefruit.Advertising.start(0);                // 0 = don't stop Advertising after n seconds  
}

/**************************************************************************/
/*!
		@brief  constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{

	uint8_t success;
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // buffer to store the returned uid
	uint8_t uidLength; 

	for(int i=0;i<numpixels;i++){
		pixels.setPixelColor(i, pixels.Color(0,0,bg));
	}
	pixels.show();

	Serial.print("loop");
	success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 3000);

	if (success) {

		/*
		 * 
		 * pack the data into a message
		 * byte 0: always 0x00
		 * byte 1: the length of the byte array
		 * byte 2: always 0x00
		 * byte 3 -> n: the array data
		 * byte after length of array: 0xff to indicate end
		 * 
		 */

		uint8_t message[uidLength+4];
		message[0] = 0x00;
		message[1] = uidLength;
		message[2] = 0x00;
		for (int n=0; n<uidLength+1;n++)
		{ 
			if(n < uidLength)
			{
				message[n+3] = uid[n];
			} else 
			{
				message[n+3] = 0xff;
			}
			
		}

		//write this out
    if(Bluefruit.connected()) {
      bleuart.write( message, sizeof(message) );
    }
    

		//print to console
		//nfc.printHex(message, sizeof(message));

		//flash neopixels
		showsuccess();

		//wait until next scan
		delay(1000);
	}
}

void showsuccess()
{
	for(int x=35;x>=0;x--) {
		for(int i=0;i<numpixels;i++){
			
			pixels.setPixelColor(i, pixels.Color(0,x,0));
		}
		pixels.show();
		delay(40);
	}
}



/***
 * obligatory adafruit mit stuff
 */
/*********************************************************************
 this is an example for our nrf52 based Bluefruit le modules

 pick one up today in the adafruit shop!

 adafruit invests time and resources providing this open source code,
 please support adafruit and open-source hardware by purchasing
 products from adafruit!

 mit license, check license for more information
 all text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
