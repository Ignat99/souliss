/**************************************************************************
	Souliss - Twin Lights
	
	It handle two lights located on two different boards and act them together
	if receive a command from the push button. Control from Android is also 
	available.
		
	Run this code on one of the following boards:
	  -	Arduino with ESP8266 Wifi module
	  
***************************************************************************/

// Configure the framework
#include "bconf/StandardArduino.h"			// Use a standard Arduino
#include "conf/wifiESP8266.h"				// Wifi module ESP8266
#include "conf/IPbroadcast.h"				// Enable communication in broadcast mode, used for ESP8266

// Include framework code and libraries
#include <SPI.h>
#include "Souliss.h"

// Define the network configuration according to your router settings
uint8_t ip_address[4]  = {192, 168, 1, 78};
uint8_t subnet_mask[4] = {255, 255, 255, 0};
uint8_t ip_gateway[4]  = {192, 168, 1, 1};
#define	Gateway_address	0xAB01
#define	Peer_address	0xAB02
#define myvNet_address	ip_address[3]		// The last byte of the IP address (77) is also the vNet address
#define	myvNet_subnet	0xFF00
#define	myvNet_supern	Gateway_address
 
#define MYLIGHT				0				// This identify the number of the logic on this node
#define PEERLIGHT			0				// This identify the number of the logic on peer node

void setup()
{	
	Initialize();
	
	// Set network parameters
	Souliss_SetAddress(Peer_address, myvNet_subnet, myvNet_supern);         	

	Set_SimpleLight(MYLIGHT);			// Define a simple LED light logic
	
	// We connect a pushbutton between 5V and pin2 with a pulldown resistor 
	// between pin2 and GND, the LED is connected to pin9 with a resistor to
	// limit the current amount
	pinMode(2, INPUT);					// Hardware pulldown required
	pinMode(9, OUTPUT);					// Power the LED
}

void loop()
{   
	// Here we start to play
	EXECUTEFAST() {						
		UPDATEFAST();	
		
		FAST_50ms() {	// We process the logic and relevant input and output every 50 milliseconds
			if(DigIn(2, Souliss_T1n_ToggleCmd, MYLIGHT))												// Use the pin2 as ON/OFF toggle command
				RemoteInput(Gateway_address, PEERLIGHT, Souliss_T1n_ToggleCmd);				// and replicate the command on the peer node
			
			Logic_SimpleLight(MYLIGHT);							// Drive the relay coil as per command
			DigOut(9, Souliss_T1n_Coil, MYLIGHT);				// Use the pin9 to give power to the coil according to the logic		
		}
		
		// Process data communication
		FAST_PeerComms();
	}
	
	EXECUTESLOW() {
		UPDATESLOW();

		SLOW_10s() {		// We handle the light timer with a 10 seconds base time
			Timer_SimpleLight(MYLIGHT);					
		} 	  
	}		
} 
