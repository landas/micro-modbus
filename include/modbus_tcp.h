/*******************************************************************************
 *   @file   modbus_tcp.h
 *   @brief  Header file of TCP-specific Modbus protocol handling
 *   @author Lars Andre Landås (landas@gmail.com)
********************************************************************************
 * MIT License
 *
 * Copyright (c) 2025 Lars Andre Landås
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
*******************************************************************************/

#ifndef INCLUDE_MODBUS_TCP_H_
#define INCLUDE_MODBUS_TCP_H_

#include "modbus.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

#define MODBUS_TCP_KEEPALIVE_IDLE_TIME_SECONDS 60
#define MODBUS_TCP_KEEPALIVE_ITERVAL_TIME_SECONDS 10
#define MODBUS_TCP_KEEPALIVE_RETRY_COUNT 5

#define MODBUS_TCP_CLOSE_CONNECTION 0xC0

#define MODBUS_MBAP_HEADER_LENGTH 7
#define MODBUS_MBAP_UNIT_ID_LENGTH 1

typedef struct {
	uint16_t transaction_id;  // Transaction Identifier
	uint16_t protocol_id;     // Protocol Identifier
	uint16_t length;          // Length
	uint8_t unit_id;          // Unit Identifier
} MBAPHeader;

typedef struct {
	uint8_t connected;
	int socket;
	int time_start;
	ModbusCallbacks* modbus_callbacks;
} ModbusTCPClient;

void modbus_tcp_start(char* server_ip, uint16_t server_port,
		      uint8_t clients_limit, ModbusCallbacks* modbus_callbacks);

#endif /* INCLUDE_MODBUS_TCP_H_ */
