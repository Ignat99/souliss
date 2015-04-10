/**************************************************************************
	Souliss - vNet Virtualized Network
    Copyright (C) 2013  Veseo

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
	
	Originally developed by Dario Di Maio
	
***************************************************************************/
/*!
    \file 
    \ingroup

*/
/**************************************************************************/
#include <stdio.h>
#include <string.h>

#include "vNetDriver_esp8266.h"							
#include "src/ESP8266.cpp"

// Global variables used in the driver
uint8_t	usartframe[USARTBUFFER];
uint16_t myaddress=0;

uint16_t vNetM3_address=0;							// Node address for the media
uint16_t vNetM3_srcaddr=0;							// Node address from incoming frame
uint8_t  vNetM3_isdata =0;							// Flag if Media3 has incoming data
oFrame vNetM3_oFrame;								// Data structure for output frame

// The ESP8266 cannot work with two media at same time as is done
// for other Ethernet drivers, the refers to M1 frames is just to
// keep the same naming used in other drivers, but here just M3
// is implemented
oFrame vNetM1_oFrame;	
uint8_t vNetM1_header;

// Define the wifi module constructor
ESP8266 esp8266;

/**************************************************************************/
/*!
    Init the ESP8266 Module
*/
/**************************************************************************/
void vNet_Init_M3()
{	
	esp8266.setBuffer(usartframe, USARTBUFFER);
	esp8266.initializeWifi();
	esp8266.connectWifi();
	esp8266.startBroadcast();
	esp8266.startServer(ETH_PORT);
}

/**************************************************************************/
/*!
	Set the vNet address and all the network parameters
*/
/**************************************************************************/
void vNet_SetAddress_M3(uint16_t addr)
{
	// Locally store the address
	vNetM3_address=addr;	
	oFrame_Define(&vNetM3_oFrame);
	oFrame_Set((uint8_t*)(&vNetM3_address), 0, VNET_M3_APPEND, 0, 0);
}

/**************************************************************************/
/*!
    Get the source address of the most recently received frame
*/
/**************************************************************************/
uint16_t vNet_GetSourceAddress_M3(){return vNetM3_srcaddr;}

/**************************************************************************/
/*!
    The upper layer needs to identify if data are on M1 or M3, and this
	flags are used for that scope
*/
/**************************************************************************/
uint8_t  vNet_setIncomingData_M3() {vNetM3_isdata = 1;}
uint8_t  vNet_hasIncomingData_M3() 
{
	if(vNetM3_isdata)
	{
		vNetM3_isdata = 0; 
		return 1;
	}
	return 0;
}	

/**************************************************************************/
/*!
	If data are available store in the temporary buffer
*/
/**************************************************************************/
uint8_t vNet_DataAvailable_M3()
{
	// Process the communication and return if there are available data
	esp8266.run();
	return esp8266.dataAvailable();
}

/**************************************************************************/
/*!
	Retrieve the complete vNet frame from the incoming buffer
*/
/**************************************************************************/
uint8_t vNet_RetrieveData_M3(uint8_t *data)
{
	uint8_t *indata;
	
	// Get the incoming data
	uint8_t	inlen = esp8266.dataRetrieve((char*)indata);
	if(inlen)	memmove(data, indata, inlen);
	
	// Remove data from the driver buffer
	esp8266.releaseData();
}

/**************************************************************************/
/*!
	Send a message via UDP/IP
*/
/**************************************************************************/
uint8_t vNet_Send_M3(uint16_t addr, oFrame *frame, uint8_t len)
{	
	/***
		Add the whole length as first byte and the node address
		at the end of the frame
	***/

	// Add the node address
	oFrame_Define(&vNetM3_oFrame);
	oFrame_Set((uint8_t*)(&vNetM3_address), 0, VNET_M3_APPEND, 0, 0);
	
	// Add the length as first byte
	vNetM1_header = len+VNET_M3_HEADER+VNET_M3_APPEND;
	oFrame_Define(&vNetM1_oFrame);
	oFrame_Set(&vNetM1_header, 0, 1, 0, frame);

	// Append the address as last, this is contained into a dedicated oFrame
	oFrame_AppendLast(&vNetM3_oFrame);
	
	// Send data in IP broadcast	
	if(esp8266.send_oFrame(&vNetM1_oFrame))
	{
		oFrame_Reset();		// Free the frame
		
		// Restart the socket
		vNet_Init_M3();
		
		return ETH_FAIL;	// If data sent fail, return
	}
	
	// At this stage data are processed or socket is failed, so we can
	// securely reset the oFrame
	oFrame_Reset();		
		
	return ETH_SUCCESS;
}

/**************************************************************************/
/*!
    Set the base IP address
*/
/**************************************************************************/
void eth_SetBaseIP(uint8_t *ip_addr)
{
}

/**************************************************************************/
/*!
    Set the IP address
*/
/**************************************************************************/
void eth_SetIPAddress(uint8_t *ip_addr)
{
}

/**************************************************************************/
/*!
    Set the Subnet mask
*/
/**************************************************************************/
void eth_SetSubnetMask(uint8_t *submask)
{
}

/**************************************************************************/
/*!
    Set the Gateway
*/
/**************************************************************************/
void eth_SetGateway(uint8_t *gateway)
{
}

/**************************************************************************/
/*!
    Get the IP address
*/
/**************************************************************************/
void eth_GetIP(uint8_t *ip_addr)
{
}

/**************************************************************************/
/*!
    Get the base IP address
*/
/**************************************************************************/
void eth_GetBaseIP(uint8_t *ip_addr)
{
}

/**************************************************************************/
/*!
    Get the Subnet mask
*/
/**************************************************************************/
void eth_GetSubnetMask(uint8_t *submask)
{
}

/**************************************************************************/
/*!
    Get the Gateway
*/
/**************************************************************************/
void eth_GetGateway(uint8_t *gateway)
{	
}