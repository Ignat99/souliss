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
#ifndef VNET_ESP8266_H
#define VNET_ESP8266_H

#include "Arduino.h"
#include "src/types.h"
#include "GetConfig.h"				// need : usartUsrCfg.h

#define	USARTBUFFER	192

void vNet_Init_M3();
void vNet_SetAddress_M3(uint16_t addr);
uint8_t vNet_Send_M3(uint16_t addr, oFrame *frame, uint8_t len);
uint8_t vNet_DataSize_M3();
uint8_t vNet_DataAvailable_M3();
uint8_t vNet_RetrieveData_M3(uint8_t *data);
uint16_t vNet_GetSourceAddress_M3();

#endif
