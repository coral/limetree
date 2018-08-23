/***
 * hello
 */

#include <adafruit_neopixel.h>
#include <bluefruit.h>
#include <wire.h>
#include <spi.h>
#include <adafruit_pn532.h>


#define pn532_ss		(16)
#define numpixels		32

adafruit_pn532 nfc(pn532_ss);

bleuart bleuart;

// function prototypes for packetparser.cpp
uint8_t readpacket (bleuart *ble_uart, uint16_t timeout);
float   parsefloat (uint8_t *buffer);
void    printhex   (const uint8_t * data, const uint32_t numbytes);

// packet buffer
extern uint8_t packetbuffer[];

uint8_t bg = 2;

adafruit_neopixel pixels = adafruit_neopixel(numpixels, 27, neo_grb + neo_khz800);

void setup(void)
{
	serial.begin(115200);

	bluefruit.begin();
	
	// set max power. accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
	bluefruit.settxpower(4);
	bluefruit.setname("atpscan");

	// configure and start the ble uart service
	bleuart.begin();

	// set up and start advertising
	startadv();

	serial.begin(115200);
	serial.println("starting!");

	nfc.begin();

	uint32_t versiondata = nfc.getfirmwareversion();
	if (! versiondata) {
		serial.print("didn't find pn53x board");
		while (1); // halt
	}
	// got ok data, print it out!
	serial.print("found chip pn5"); serial.println((versiondata>>24) & 0xff, hex); 
	serial.print("firmware ver. "); serial.print((versiondata>>16) & 0xff, dec); 
	serial.print('.'); serial.println((versiondata>>8) & 0xff, dec);
	
	// configure board to read rfid tags
	nfc.samconfig();

	//start neopixel
	pixels.begin();
	
}

void startadv(void)
{
	// advertising packet
	bluefruit.advertising.addflags(ble_gap_adv_flags_le_only_general_disc_mode);
	bluefruit.advertising.addtxpower();
	
	// include the ble uart (aka 'nus') 128-bit uuid
	bluefruit.advertising.addservice(bleuart);

	// secondary scan response packet (optional)
	// since there is no room for 'name' in advertising packet
	bluefruit.scanresponse.addname();

	/* start advertising
	 * - enable auto advertising if disconnected
	 * - interval:  fast mode = 20 ms, slow mode = 152.5 ms
	 * - timeout for fast mode is 30 seconds
	 * - start(timeout) with timeout = 0 will advertise forever (until connected)
	 */
	
	bluefruit.advertising.restartondisconnect(true);
	bluefruit.advertising.setinterval(32, 244);    // in unit of 0.625 ms
	bluefruit.advertising.setfasttimeout(30);      // number of seconds in fast mode
	bluefruit.advertising.start(0);                // 0 = don't stop advertising after n seconds  
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
	uint8_t uidlength; 

	for(int i=0;i<numpixels;i++){
		pixels.setpixelcolor(i, pixels.color(0,0,bg));
	}
	pixels.show();

	
	success = nfc.readpassivetargetid(pn532_mifare_iso14443a, uid, &uidlength);

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

		uint8_t message[uidlength+4];
		message[0] = 0x00;
		message[1] = uidlength;
		message[2] = 0x00;
		for (int n=0; n<uidlength+1;n++)
		{ 
			if(n < uidlength)
			{
				message[n+3] = uid[n];
			} else 
			{
				message[n+3] = 0xff;
			}
			
		}

		//write this out
		bleuart.write( message, sizeof(message) );

		//print to console
		nfc.printhex(message, sizeof(message));

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
			
			pixels.setpixelcolor(i, pixels.color(0,x,0));
		}
		pixels.show();
		delay(40);
	}
}



/***
 * obligatory adafruit mit stuff
 */
/*********************************************************************
 this is an example for our nrf52 based bluefruit le modules

 pick one up today in the adafruit shop!

 adafruit invests time and resources providing this open source code,
 please support adafruit and open-source hardware by purchasing
 products from adafruit!

 mit license, check license for more information
 all text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
