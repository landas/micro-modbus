/*******************************************************************************
 *   @file   modbus_tcp.c
 *   @brief  Implementation of TCP/IP-specific Modbus protocol handling
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

#include "modbus_tcp.h"

void modbus_mbap_fill_struct(MBAPHeader *header, const uint8_t* data)
{
	header->transaction_id = (uint16_t)(data[0] << 8)  + data[1];
	header->protocol_id = (uint16_t)(data[2] << 8) + data[3];
	header->length = (uint16_t)(data[4] << 8) + data[5];
	header->unit_id = data[6];
}

void modbus_mbap_fill_from_struct(uint8_t* data, MBAPHeader *header)
{
	data[0] = (uint8_t)(header->transaction_id >> 8);
	data[1] = (uint8_t)(header->transaction_id);
	data[2] = (uint8_t)(header->protocol_id >> 8);
	data[3] = (uint8_t)(header->protocol_id);
	data[4] = (uint8_t)(header->length >> 8);
	data[5] = (uint8_t)(header->length);
	data[6] = (uint8_t)(header->unit_id);
}

uint8_t modbus_tcp_handle_request(uint8_t *data, int data_size,
				  ModbusCallbacks* callbacks, int client_socket)
{

	if(data_size < 8)
		return 3;

	MBAPHeader mbap_header = {};
	uint8_t *send_buffer = NULL;
	uint16_t send_size = 0;
	uint8_t ret = 0;

	modbus_mbap_fill_struct(&mbap_header, data);

	ret = modbus_handle_request(data + MODBUS_MBAP_HEADER_LENGTH,
				    data_size - MODBUS_MBAP_HEADER_LENGTH, callbacks, &send_buffer, &send_size,
				    MODBUS_MBAP_HEADER_LENGTH, 0);

	if(send_buffer != NULL) {
		mbap_header.length = send_size - MODBUS_MBAP_HEADER_LENGTH +
				     MODBUS_MBAP_UNIT_ID_LENGTH;
		modbus_mbap_fill_from_struct(send_buffer, &mbap_header);
		lwip_send(client_socket, send_buffer, send_size, 0);
		vPortFree(send_buffer);
		send_buffer = NULL;
	}

	return ret;
}

int16_t modbus_tcp_find_free_connection_slot(ModbusTCPClient
		*modbus_tcp_clients, uint8_t max_clients)
{

	for(uint8_t i=0; i<max_clients; ++i) {
		if(modbus_tcp_clients[i].connected == 0) {
			return i;
		}
	}
	return -1;
}

void modbus_tcp_connection_handle(void *parameters)
{
	char buffer[MODBUS_RECV_BUFFER_SIZE];
	int bytes_received;
	int ret = 0;
	ModbusTCPClient *client = (ModbusTCPClient *)parameters;

	while (ret != MODBUS_TCP_CLOSE_CONNECTION
	       && (bytes_received = lwip_recv(client->socket, buffer, MODBUS_RECV_BUFFER_SIZE, 0)) > 0) {
		ret = modbus_tcp_handle_request((uint8_t*)buffer, bytes_received,
						client->modbus_callbacks, client->socket);
		osDelay(MODBUS_CONNECTION_INTERVAL_MS);
	}

	lwip_close(client->socket);
	client->connected = 0;
	client->socket = 0;
	client->time_start = 0;
	vTaskDelete(NULL);
}

void modbus_tcp_start(char* server_ip, uint16_t server_port,
		      uint8_t max_clients, ModbusCallbacks* modbus_callbacks)
{
	int sockfd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len = sizeof(client_addr);
	char client_ip[INET_ADDRSTRLEN];

	ModbusTCPClient *modbus_tcp_clients = pvPortMalloc(sizeof(
			ModbusTCPClient) * max_clients);

	if(modbus_tcp_clients == NULL)
		return;

	if ((sockfd = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_len = sizeof(server_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = lwip_htons(server_port);

	if (lwip_inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
		return;
	}

	if (lwip_bind(sockfd, (struct sockaddr *)&server_addr,
		      sizeof(server_addr)) < 0) {
		lwip_close(sockfd);
		return;
	}

	if (lwip_listen(sockfd, max_clients) < 0) {
		lwip_close(sockfd);
		return;
	}

	for(;;) {
		int client_sock = lwip_accept(sockfd, (struct sockaddr *)&client_addr,
					      &addr_len);
		if(client_sock < 0) {
			lwip_close(client_sock);
			continue;
		}

		lwip_inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

		uint16_t connection_slot = modbus_tcp_find_free_connection_slot(
						   modbus_tcp_clients, max_clients);
		if(connection_slot != -1) {

			// Setup TCP/IP keepalive for client connection
			int tcp_keepalive_idle_time = MODBUS_TCP_KEEPALIVE_IDLE_TIME_SECONDS;
			int tcp_keepalive_interval_time = MODBUS_TCP_KEEPALIVE_ITERVAL_TIME_SECONDS;
			int tcp_keepalive_retry_count = MODBUS_TCP_KEEPALIVE_RETRY_COUNT;
			int tcp_keepalive_enable = 1;

			lwip_setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &tcp_keepalive_enable,
					sizeof(tcp_keepalive_enable));
			lwip_setsockopt(client_sock, IPPROTO_TCP, TCP_KEEPIDLE,
					&tcp_keepalive_idle_time, sizeof(tcp_keepalive_idle_time));
			lwip_setsockopt(client_sock, IPPROTO_TCP, TCP_KEEPINTVL,
					&tcp_keepalive_interval_time, sizeof(tcp_keepalive_interval_time));
			lwip_setsockopt(client_sock, IPPROTO_TCP, TCP_KEEPCNT,
					&tcp_keepalive_retry_count, sizeof(tcp_keepalive_retry_count));

			modbus_tcp_clients[connection_slot].connected = 1;
			modbus_tcp_clients[connection_slot].socket = client_sock;
			modbus_tcp_clients[connection_slot].time_start = xTaskGetTickCount();
			modbus_tcp_clients[connection_slot].modbus_callbacks = modbus_callbacks;
			xTaskCreate(modbus_tcp_connection_handle,
				    "modbcon",
				    configMINIMAL_STACK_SIZE,
				    (void *)&modbus_tcp_clients[connection_slot],
					osPriorityNormal,
				    NULL);
		} else {
			lwip_close(client_sock);
			continue;
		}

		osDelay(MODBUS_CONNECTION_INTERVAL_MS);
	}

	lwip_close(sockfd);
}
