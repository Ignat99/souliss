/**************************************************************************
	Souliss - vNet Virtualized Network
    Copyright (C) 2015  Veseo

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
	
	Originally developed by Steven Sokol
	Included in vNet by		Dario Di Maio
***************************************************************************/
/*!
    \file 
    \ingroup
*/
/**************************************************************************/
#include <stdio.h>
#include <string.h>
#include <Arduino.h>

#ifndef VNET_ESP8266_H
#define VNET_ESP8266_H

#include "tools/SerialPort.h"

/**************************************************************************/
/*!
	If enabled print the header and payload of incoming, outgoing and routed
	frames.
	
        Value       Status
        0x0         Disable (Default)
        0x1         Enable
		
	This enable the Arduino AltSoftSerial library and need an external USB
	to USART converter (like the one used for programming Arduino Ethernet).
	
	In your sketch you should add the following lines
		[ before setup() ] 	AltSoftSerial myUSARTDRIVER(5, 6);	
		
		[in setup()	]		myUSARTDRIVER.begin(9600);

	The TX and RX pins from the external converter shall be reversed, so TX 
	goes on pin 5 (RX from SoftwareSerial) and RX to 6.
*/
/**************************************************************************/
#define ESP8266_DEBUG  			0

enum wifiModes {
  WIFI_MODE_STA = 1,
  WIFI_MODE_AP,
  WIFI_MODE_APSTA
};

enum wifiErrors {
  WIFI_ERR_NONE = 0,
  WIFI_ERR_AT,
  WIFI_ERR_RESET,
  WIFI_ERR_CONNECT,
  WIFI_ERR_LINK  
};

class ESP8266
{
  public:
    // constructor - set link mode and server port
    ESP8266(uint16_t mode = 1, long baudrate = 9600, uint16_t debugLevel = 0);
    
    // init / connect / disconnect access point
    uint16_t initializeWifi(DataCallback dcb, ConnectCallback ccb);
    uint16_t connectWifi(char *ssid, char *password);
    bool disconnectWifi();
    
    // server
    bool startServer(uint16_t port = 8000, long timeout = 300);
    
    // client
    bool startClient(char *ip, uint16_t port, long timeout = 300);
    
	// data and connection feedback
	bool dataAvailable(); 			
	bool connectionAvailable();
	uint16_t  dataRetrieve(char* data_pnt);
	
    // send data across the link
    bool send(char *data);
	uint16_t  send_oFrame(oFrame* frame);
    
    // process wifi messages - MUST be called from main app's loop
    void run();

    // informational
    char *ip();
    uint16_t scan(char *out, uint16_t max);
	
	
  private:
    void clearResults();
    bool sendData(uint16_t chan, char *data);
    bool setLinkMode(uint16_t mode);
    bool startUDPChannel(uint16_t chan, char *address, uint16_t port);
    void processWifiMessage();
    bool getIP();
    bool getBroadcast();
    void debug(char *msg);
    bool searchResults(char *target, long timeout, uint16_t dbg = 0);
};

#endif


