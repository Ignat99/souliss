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

#include "tools/SerialPort.cpp"

#define wifi 			USARTDRIVER
#define SVR_CHAN 		1
#define BCN_CHAN 		2
#define CLI_CHAN 		3
#define USART_BUFFER 	192

#if (ESP8266_DEBUG)
	#include <AltSoftSerial.h>
	extern AltSoftSerial myUSARTDRIVER; // RX, TX
	
	#define USART_LOG 	mywifi.print
#endif

SerialPort<0, USART_BUFFER, 0> USARTDRIVER;

enum connectMode {
  CONNECT_MODE_NONE = 0,
  CONNECT_MODE_SERVER,
  CONNECT_MODE_CLIENT
};

int  _mode;
long _baudrate;
char _ipaddress[15];
char _broadcast[15];
int  _port;
//char _device[48];
//char _ssid[48];			Note: Give just a pointer to an external stored information
//char _password[24];		Note: Give just a pointer to an external stored information
//bool _beacon;
//long _beaconInterval;
//long _previousMillis;
int _replyChan;
DataCallback _dcb;
ConnectCallback _ccb;
char *_wb;
int _wctr = 0;
bool _connected;
int _connectMode;
int _debugLevel = 0;

// Constructor
ESP8266::ESP8266(int mode, long baudrate, int debugLevel)
{
  _mode = mode;
  _baudrate = baudrate;
  _debugLevel = debugLevel;
  _port = 8000;
  _replyChan = 0;
  memset(_ipaddress, 0, 15);
  memset(_broadcast, 0, 15);
  //memset(_device, 0, 48);
  //memset(_ssid, 0, 48);
  //memset(_password, 0, 24);
  //_beacon = false;
  //_beaconInterval = (10000L);
  //_previousMillis = 0;
  _connected = false;
}

// Use an external buffer for incoming data
void ESP8266::setBuffer(uint8_t *pointer)
{
	_wb = (char*)pointer;
}

int ESP8266::initializeWifi(DataCallback dcb, ConnectCallback ccb)
{
  
  if (dcb) {
    _dcb = dcb;
  }
  
  if (ccb) {
    _ccb = ccb; 
  }
  
  wifi.begin(_baudrate);
  //wifi.setTimeout(5000); // The SerialPort doesn't support Timeout option
  
  //delay(500);
  clearResults();
  
  // check for presence of wifi module
  wifi.println(F("AT"));
  delay(500);
  if(!searchResults("OK", 1000, _debugLevel)) {
    return WIFI_ERR_AT;
  } 
  
  //delay(500);
  
  // reset WiFi module
  wifi.println(F("AT+RST"));
  delay(500);
  if(!searchResults("Ready", 5000, _debugLevel)) {
    return WIFI_ERR_RESET;
  }
  
  delay(500);
  
  // set the connectivity mode 1=sta, 2=ap, 3=sta+ap
  wifi.print(F("AT+CWMODE="));
  wifi.println(_mode);
  //delay(500);
  
  clearResults();
  
  return WIFI_ERR_NONE;
}

// Connect the module to the WiFi network
int ESP8266::connectWifi()
{
  
  //strcpy(_ssid, ssid);
  //strcpy(_password, password);
  
  // set the access point value and connect
  wifi.print(F("AT+CWJAP=\""));
  wifi.print(WiFi_SSID);
  wifi.print(F("\",\""));
  wifi.print(WiFi_Password);
  wifi.println(F("\""));
  delay(100);
  if(!searchResults("OK", 30000, _debugLevel)) {
    return WIFI_ERR_CONNECT;
  }
  
  // enable multi-connection mode
  if (!setLinkMode(1)) {
    return WIFI_ERR_LINK;
  }
  
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
  int chan;
  if (_connectMode == CONNECT_MODE_SERVER) {
    chan = _replyChan;
  } else {
    chan = CLI_CHAN;
  }
  return sendData(chan, data);
}

// Process the incoming data
void ESP8266::run()
{
  uint8_t v;
   
    // process wifi messages
    while(wifi.available() > 0) {
      v = wifi.read();
      if (v == 10) {
        _wb[_wctr] = 0;
        _wctr = 0;
        processWifiMessage();
      } else if (v == 13) {
        // gndn
      } else {
        _wb[_wctr] = v;
        if() _wctr++;
		else
		{
			// flush
			
		}
      }
    }
  }
}

// Start a TCP server
bool ESP8266::startServer(int port, long timeout)
{
  // cache the port number for the beacon
  _port = port;
  
  wifi.print(F("AT+CIPSERVER="));
  wifi.print(SVR_CHAN);
  wifi.print(F(","));
  wifi.println(_port);
  if(!searchResults("OK", 500, _debugLevel)){
    return false;
  }
  
  // send AT command
  wifi.print(F("AT+CIPSTO="));
  wifi.println(timeout);
  if(!searchResults("OK", 500, _debugLevel)) {
    return false;
  }
  
  _connectMode = CONNECT_MODE_SERVER;
  return true;
}

// Open a TCP/IP connection
bool ESP8266::startClient(char *ip, int port, long timeout)
{
  wifi.print(F("AT+CIPSTART="));
  wifi.print(CLI_CHAN);
  wifi.print(F(",\"TCP\",\""));
  wifi.print(ip);
  wifi.print("\",");
  wifi.println(port);
  delay(100);
  if(!searchResults("OK", timeout, _debugLevel)) {
    return false;
  }
  
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
void ESP8266::processWifiMessage() {
  int packet_len;
  int channel;
  char *pb;  
  
  // if the message is simply "Link", then we have a live connection
  if(strncmp(_wb, "Link", 5) == 0) {
    
    // reduce the beacon frequency by increasing the interval
    //_beaconInterval = 30000; //false;
    
    // flag the connection as active
    _connected = true;
    
    // if a connection callback is set, call it
    if (_ccb) _ccb();
  } else
  
  // message packet received from the server
  if(strncmp(_wb, "+IPD,", 5)==0) {
    
    // get the channel and length of the packet
    sscanf(_wb+5, "%d,%d", &channel, &packet_len);
    
    // cache the channel ID - this is used to reply
    _replyChan = channel;
    
    // if the packet contained data, move the pointer past the header
    if (packet_len > 0) {
      pb = _wb+5;
      while(*pb!=':') pb++;
      pb++;
      
      // execute the callback passing a pointer to the message
      if (_dcb) {
        _dcb(pb);
      }
      
      // DANGER WILL ROBINSON - there is no ring buffer or other safety net here.
      // the application should either use the data immediately or make a copy of it!
      // do NOT block in the callback or bad things may happen
      
    }
  } else {
    // other messages might wind up here - some useful, some not.
    // you might look here for things like 'Unlink' or errors
  }
}

// Send the data
bool ESP8266::sendData(int chan, char *data) {

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

// Send the data using an oFrame structure
bool ESP8266::send_vNet(uint16_t addr, oFrame *frame, uint8_t len) {
	
	uint8_t i, j, data, plen;
	uint16_t crc=0xFFFF;
	
	// Nothing to send
	if (len == 0)
		return USART_FAIL;

	// Start sending the data
	oFrame_Define(frame);
	
	// Define the maximum frame length, total size is  frame_size +
	// the lenght_as_header + the crc
	if(oFrame_GetLenght() > USART_MAXPAYLOAD)
		plen=USART_MAXPAYLOAD;
	else
		plen=oFrame_GetLenght();	
	
  // start send
  wifi.print(F("AT+CIPSEND="));
  wifi.print(BCN_CHAN);
  wifi.print(",");
  wifi.println(USART_PREAMBLE_LEN+USART_HEADERLEN+plen+USART_CRCLEN+USART_POSTAMBLE_LEN);
	
	// Send the preamble
	for(i=0; i<USART_PREAMBLE_LEN; i++)
		wifi.write(USART_PREAMBLE);
		
	// Write frame length
	wifi.write(plen+USART_HEADERLEN+USART_CRCLEN);
																			
	while(oFrame_Available() && plen)					// Send the frame	
	{	
		// Get the next byte to send
		data = oFrame_GetByte();
		
		// Compute the CRC
		crc = crc ^ (data);
		for (j=0; j<8; j++) 
		{
			if (crc & 0x0001)
				crc = (crc >> 1) ^ 0xA001;
			else
				crc = crc >> 1;
		}		
		
		// Send byte
		wifi.write(data);	
		
		// Bytes to be sent
		plen--;	
	}
	
	// This is the CRC
	crc = (crc << 8  | crc >> 8);
	
	// Send the CRC
	uint8_t* u8crc=((uint8_t*)&crc);			// Cast as byte array
	wifi.write(u8crc[0]);
	wifi.write(u8crc[1]);
	
	// Send postamble
	for(i=0; i<USART_POSTAMBLE_LEN; i++)
		wifi.write(USART_POSTAMBLE);
	
	// Remove non processed bytes [if(oFrame_GetLenght() > USART_MAXPAYLOAD)]
	oFrame_Reset();
  
  delay(50);
  
  // to debug only
  searchResults("OK", 500, _debugLevel);
  
  return true;
}

// Set the link mode (single connection, multiple connection)
bool ESP8266::setLinkMode(int mode) {
  wifi.print(F("AT+CIPMUX="));
  wifi.println(mode);
  delay(500);
  if(!searchResults("OK", 1000, _debugLevel)){
    return false;
  }
  return true;
}

// Open an UDP channel
bool ESP8266::startUDPChannel(int chan, char *address, int port) {
  wifi.print(F("AT+CIPSTART="));
  wifi.print(chan);
  wifi.print(F(",\"UDP\",\""));
  wifi.print(address);
  wifi.print("\",");
  wifi.println(port);
  if(!searchResults("OK", 500, _debugLevel)) {
    return false;
  }
  return true;
}

// private convenience functions
bool ESP8266::getIP() {
  
  char c;
  char buf[15];
  int dots, ptr = 0;
  bool ret = false;

  memset(buf, 0, 15);

  wifi.println(F("AT+CIFSR"));
  delay(500);
  while (wifi.available() > 0) {
    c = wifi.read();
    
    // increment the dot counter if we have a "."
    if ((int)c == 46) {
      dots++;
    }
    if ((int)c == 10) {
      // end of a line.
      if ((dots == 3) && (ret == false)) {
        buf[ptr] = 0;
        strcpy(_ipaddress, buf);
        ret = true;
      } else {
        memset(buf, 0, 15);
        dots = 0;
        ptr = 0;
      }
    } else
    if ((int)c == 13) {
      // ignore it
    } else {
      buf[ptr] = c;
      ptr++;
    }
  }

  return ret;
}

// Starting from the module IP address, get the subnet broadcast address
bool ESP8266::getBroadcast() {
  
  int i, c, dots = 0;
  
  if (strlen(_ipaddress) < 7) {
    return false;
  }
  
  memset(_broadcast, 0, 15);
  for (i = 0; i < strlen(_ipaddress); i++) {
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
bool ESP8266::searchResults(char *target, long timeout, int dbg)
{
  int c;
  int index = 0;
  int targetLength = strlen(target);
  int count = 0;
  
#if (ESP8266_DEBUG)  
  char _data[255];	// this is a waste of RAM!!!
  memset(_data, 0, 255);
#endif

  long _startMillis = millis();
  do {
    c = wifi.read();

#if (ESP8266_DEBUG)    
      if (dbg > 0) {
        if (count >= 254) {
          debug(_data);
          memset(_data, 0, 255);
          count = 0;
        }
        _data[count] = c;
        count++;
      }
#endif

      if (c != target[index])
        index = 0;
        
      if (c == target[index]){
        if(++index >= targetLength){
          if (dbg > 1)
            USART_LOG(_data);
          return true;
        }
      }
  } while(wifi.available() && (millis() - _startMillis < timeout));

  if (dbg > 0) {
    if (_data[0] == 0) {
      USART_LOG("Failed: No data");
    } else {
      USART_LOG("Failed");
      USART_LOG(_data);
    }
  }
  return false;
}

void ESP8266::clearResults() {
  char c;
  while(wifi.available() > 0) {
    c = wifi.read();
  }
}
