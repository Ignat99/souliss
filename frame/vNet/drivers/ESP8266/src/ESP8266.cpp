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

#include "Arduino.h"
#include "ESP8266.h"

// The name of the class that refers to the USART, change it accordingly to the used device
#ifndef ESP8266DRIVER_INSKETCH
#	define 	wifi 		Serial

#	if (ESP8266_DEBUG)
#		include <AltSoftSerial.h>
		extern AltSoftSerial myUSARTDRIVER; // RX, TX
	
#		define USART_LOG 	myUSARTDRIVER.print
#	endif
#endif

// Define ESP8266 useful constants
#define SVR_CHAN 			1
#define BCN_CHAN 			2
#define CLI_CHAN 			3
#define	ESP8266_HEADER		9

#define	WIFI_ERR_AT 		1
#define	WIFI_ERR_RESET		2
#define	WIFI_ERR_NONE		3
#define	WIFI_ERR_CONNECT	4
#define	WIFI_ERR_LINK		5

enum connectMode {
  CONNECT_MODE_NONE = 0,
  CONNECT_MODE_SERVER,
  CONNECT_MODE_CLIENT
};

// Internal parameters/variables
uint8_t		_mode;
long		_baudrate;
char		_ipaddress[15];
char		_broadcast[15];
uint16_t	_port;
uint8_t		_replyChan;

// There are used to flag upper layers for available incoming data
bool		_dcb;
uint8_t		_plen = 0;
char		*_wb;
uint8_t		_wblen = 0;
uint8_t		_wctr = 0;
bool		_connected;
uint8_t		_connectMode;
uint8_t		_debugLevel = 0;

// Constructor
ESP8266::ESP8266(uint16_t mode, long baudrate, uint16_t debugLevel)
{
	_mode = mode;
	_baudrate = baudrate;
	_debugLevel = debugLevel;
	_port = 8000;
	_replyChan = 0;
	memset(_ipaddress, 0, 15);
	memset(_broadcast, 0, 15);
	_connected = false;
}

// Use an external buffer for incoming data
void ESP8266::setBuffer(uint8_t *pointer, uint8_t len)
{
	_wb = (char*)pointer;
	_wblen = len;
}

// Handle data with upper layers
bool ESP8266::dataAvailable() 		{return _dcb}
bool ESP8266::connectionAvailable() {return _connected}

// Get the pointer of the payload and length
uint16_t ESP8266::dataRetrieve(char* data_pnt) 		
{
	data_pnt = _wb;		// Transfer the actual pointer where data are stored
	_dcb=false;			// Remove the data available flag
	return _plen;		// Return the length on the available data
}

// Once data has been retrieved, shall be released
uint16_t ESP8266::releaseData()
{
	// The buffer capacity shall not be smaller than the incoming frame
	if(_wblen>_plen) memmove(_wb, _wb+_plen, _wblen-_plen);
	else			 memset (_wb, 0x0, _wblen);
}

uint16_t ESP8266::initializeWifi()
{
	wifi.begin(_baudrate);
  
	// Some USART drivers doesn't support the Timeout option
	#if(ESP8266_ENTIMEOUT)
	wifi.setTimeout(ESP8266_TIMEOUT); 
	#endif
  
	delay(500);
	clearResults();
  
	// check for presence of wifi module
	wifi.println(F("AT"));
	delay(500);
	if(!searchResults("OK", 1000, _debugLevel)) 
		return WIFI_ERR_AT;
  
	delay(500);
  
	// reset WiFi module
	wifi.println(F("AT+RST"));
	delay(500);
	if(!searchResults("Ready", 5000, _debugLevel)) 
		return WIFI_ERR_RESET;
  
	delay(500);
  
	// set the connectivity mode 1=sta, 2=ap, 3=sta+ap
	wifi.print(F("AT+CWMODE="));
	wifi.println(_mode);
	delay(500);
  
	clearResults();
  
	return WIFI_ERR_NONE;
}

// Connect the module to the WiFi network
uint16_t ESP8266::connectWifi()
{
	// set the access point value and connect
	wifi.print(F("AT+CWJAP=\""));
	wifi.print(WiFi_SSID);
	wifi.print(F("\",\""));
	wifi.print(WiFi_Password);
	wifi.println(F("\""));
	delay(100);
	if(!searchResults("OK", 30000, _debugLevel)) 
		return WIFI_ERR_CONNECT;

  
	// enable multi-connection mode
	if (!setLinkMode(1))
		return WIFI_ERR_LINK;
  
	// get the IP assigned by DHCP
	getIP();
  
	// get the broadcast address (assumes class c w/ .255)
	getBroadcast();
  
	return WIFI_ERR_NONE;
}

// Start a broadcast channel, all data transmitted on this channel will be UDP/IP
// broadcast on hard-coded port
bool ESP8266::startBroadcast()
{
	return startUDPChannel(BCN_CHAN, _broadcast, ETH_PORT);
}

// Send data as TCP/IP on a unicast address
bool ESP8266::send(char *data)
{
	uint16_t chan;
	if (_connectMode == CONNECT_MODE_SERVER)
		chan = _replyChan;
	else
		chan = CLI_CHAN;

	return sendData(chan, data);
}

// Send the data using an oFrame structure
uint16_t ESP8266::send_oFrame(oFrame* frame) {

	oFrame_Define(frame);						// Set the frame
	uint8_t slen = oFrame_GetLenght();			// Get the total length	
		
	// start send
	wifi.print(F("AT+CIPSEND="));
	wifi.print(BCN_CHAN);
	wifi.print(",");
	wifi.println(slen);
		
	// Send byte
	wifi.write(data);	

	// Write up the end of the buffer
	for(uint8_t i=0;i<slen;i++)
		if(oFrame_Available())
			write(dstAddr++, oFrame_GetByte());
		else
			break;		
		
	// Remove non processed bytes
	oFrame_Reset();
	delay(50);
  
	// to debug only
	searchResults("OK", 500, _debugLevel);
	return true;
}


// Process the incoming data
void ESP8266::run()
{
	uint8_t v;
   
    // process wifi messages
    while(wifi.available() > 0) 
	{
		v = wifi.read();
		_wb[_wctr] = v;
		if(_wctr < _wblen) _wctr++;
		else
		{
			// if the max length has been exceed, a cut will result in
			// the frame once processed		  
		}
	  
		if(_wblen > ESP8266_HEADER) processWifiMessage(_wblen); 

    }
}

// Start a TCP server
bool ESP8266::startServer(uint16_t port, long timeout)
{
	// cache the port number for the beacon
	_port = port;
  
	wifi.print(F("AT+CIPSERVER="));
	wifi.print(SVR_CHAN);
	wifi.print(F(","));
	wifi.println(_port);
	if(!searchResults("OK", 500, _debugLevel))
		return false;
  
	// send AT command
	wifi.print(F("AT+CIPSTO="));
	wifi.println(timeout);
	if(!searchResults("OK", 500, _debugLevel))
		return false;
  
	_connectMode = CONNECT_MODE_SERVER;
	return true;
}

// Open a TCP/IP connection
bool ESP8266::startClient(char *ip, uint16_t port, long timeout)
{
	wifi.print(F("AT+CIPSTART="));
	wifi.print(CLI_CHAN);
	wifi.print(F(",\"TCP\",\""));
	wifi.print(ip);
	wifi.print("\",");
	wifi.println(port);
	delay(100);
	if(!searchResults("OK", timeout, _debugLevel))
		return false;

	_connectMode = CONNECT_MODE_CLIENT;
	return true;
}

// Return the module IP address
char *ESP8266::ip()
{
	return _ipaddress;
}

// *****************************************************************************
// PRIVATE FUNCTIONS BELOW THIS POINT
// *****************************************************************************

// process a complete message from the WiFi side
// and send the actual data payload to the serial port
void ESP8266::processWifiMessage(uint8_t available_bytes) {
	uint16_t packet_len;
	uint16_t channel;
	uint8_t pb;  
  
	// if the message is simply "Link", then we have a live connection
	if(strncmp(_wb, "Link", 5) == 0)  // flag the connection as active 
		_connected = true;
  
	// message packet received from the server
	if(strncmp(_wb, "+IPD,", 5)==0) 
	{
		// get the channel and length of the packet
		sscanf(_wb+5, "%d,%d", &channel, &packet_len);
		
		if(packet_len+5 <= available_bytes)
		{
			// cache the channel ID - this is used to reply
			_replyChan = channel;
			
			// if the packet contained data, move the pointer past the header
			if ((packet_len > 0) && (packet_len < _wblen)) {
			  while((*(_wb+5+pb)!=':') && (pb < _wblen)) pb++;
			  pb++;
			  
			  // Remove the header
			  memmove(_wb, _wb+pb, _wblen-pb);
			  
			  // a packet is available, store length and pointer
			  _dcb=true;
			  _plen=packet_len;
			  
			  // DANGER WILL ROBINSON - there is no ring buffer or other safety net here.
			  // the application should either use the data immediately or make a copy of it!
			  // do NOT block in the callback or bad things may happen
			}	
		}
	} 
	else {
    // other messages might wind up here - some useful, some not.
    // you might look here for things like 'Unlink' or errors
  }
}

// Send the data
bool ESP8266::sendData(uint16_t chan, char *data) {

	// start send
	wifi.print(F("AT+CIPSEND="));
	wifi.print(chan);
	wifi.print(",");
	wifi.println(strlen(data));
  
	// send the data
	wifi.println(data);
  
	delay(50);
  
	// to debug only
	searchResults("OK", 500, _debugLevel);
  
	return true;
}

// Set the link mode (single connection, multiple connection)
bool ESP8266::setLinkMode(uint16_t mode) {
	wifi.print(F("AT+CIPMUX="));
	wifi.println(mode);
	delay(500);
	if(!searchResults("OK", 1000, _debugLevel))
		return false;
	
	return true;
}

// Open an UDP channel
bool ESP8266::startUDPChannel(uint16_t chan, char *address, uint16_t port) {
	wifi.print(F("AT+CIPSTART="));
	wifi.print(chan);
	wifi.print(F(",\"UDP\",\""));
	wifi.print(address);
	wifi.print("\",");
	wifi.println(port);
	if(!searchResults("OK", 500, _debugLevel))
		return false;

	return true;
}

// private convenience functions
bool ESP8266::getIP() {
  
	char c;
	char buf[15];
	uint16_t dots, ptr = 0;
	bool ret = false;

	memset(buf, 0, 15);

	wifi.println(F("AT+CIFSR"));
	delay(500);
	while (wifi.available() > 0) 
	{
		c = wifi.read();
		
		// increment the dot counter if we have a "."
		if ((uint16_t)c == 46)
			dots++;

		if ((uint16_t)c == 10) 
		{
			// end of a line.
			if ((dots == 3) && (ret == false)) 
			{
				buf[ptr] = 0;
				strcpy(_ipaddress, buf);
				ret = true;
			} 
			else 
			{
				memset(buf, 0, 15);
				dots = 0;
				ptr = 0;
			}
		} 
		else if ((uint16_t)c == 13) 
		{
			// ignore it
		} 
		else 
		{
			buf[ptr] = c;
			ptr++;
		}
	}

	return ret;
}

// Starting from the module IP address, get the subnet broadcast address
bool ESP8266::getBroadcast() {
  
	uint16_t i, c, dots = 0;
  
	if (strlen(_ipaddress) < 7)
		return false;

  
	memset(_broadcast, 0, 15);
	for (i = 0; i < strlen(_ipaddress); i++) 
	{
		c = _ipaddress[i];
		_broadcast[i] = c;
		if (c == 46) dots++;
		if (dots == 3) break;
	}
	i++;
	_broadcast[i++] = 50;
	_broadcast[i++] = 53;
	_broadcast[i++] = 53;
  
	return true;
}

// Parse the incoming frame and looks for a target string
bool ESP8266::searchResults(char *target, long timeout, uint16_t dbg)
{
	uint16_t c;
	uint16_t index = 0;
	uint16_t targetLength = strlen(target);
	uint16_t count = 0;
  
	#if (ESP8266_DEBUG)  
	char _data[255];	// this is a waste of RAM!!!
	memset(_data, 0, 255);
	#endif

	long _startMillis = millis();
	do 
	{
		c = wifi.read();

		#if (ESP8266_DEBUG)    
		if (count >= 254) 
		{
			debug(_data);
			memset(_data, 0, 255);
			count = 0;
		}
		_data[count] = c;
		count++;
		#endif

		if (c != target[index])
			index = 0;
				
		if (c == target[index])
		{
			if(++index >= targetLength)
			{
				#if (ESP8266_DEBUG)
				USART_LOG(_data);
				#endif
				
				return true;
			}
		}
	} 
	while(wifi.available() && (millis() - _startMillis < timeout));

	#if (ESP8266_DEBUG)    
	if (_data[0] == 0) 
		USART_LOG("Failed: No data");
	else 
	{
		USART_LOG("Failed");
		USART_LOG(_data);
	}
	#endif
	
	return false;
}

void ESP8266::clearResults() {
	char c;
	while(wifi.available() > 0) {
		c = wifi.read();
	}
}
