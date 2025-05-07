/*******************************************************************************
 *   @file   modbus.h
 *   @brief  Header file of Modbus protocol handling
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

#ifndef INCLUDE_MODBUS_H_
#define INCLUDE_MODBUS_H_

#include <stdint.h>
#include <string.h>

#include "config.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"

#define MODBUS_READ_COILS_FC 0x01
#define MODBUS_READ_DISCRETE_INPUTS_FC 0x02
#define MODBUS_READ_HOLDING_REGISTERS_FC 0x03
#define MODBUS_READ_INPUT_REGISTERS_FC 0x04
#define MODBUS_WRITE_SINGLE_COIL_FC 0x05
#define MODBUS_WRITE_SINGLE_REGISTER_FC 0x06
#define MODBUS_WRITE_MULTIPLE_COILS_FC 0x0F
#define MODBUS_WRITE_MULTIPLE_REGISTERS_FC 0x10
#define MODBUS_READ_WRITE_MULTIPLE_REGISTERS_FC 0x17
#define MODBUS_MASK_WRITE_REGISTER_FC 0x16
#define MODBUS_READ_FIFO_QUEUE_FC 0x18
#define MODBUS_READ_FILE_RECORD_FC 0x14
#define MODBUS_WRITE_FILE_RECORD_FC 0x15
#define MODBUS_READ_EXCEPTION_STATUS_FC 0x07
#define MODBUS_DIAGNOSTIC_FC 0x08
#define MODBUS_GET_COM_EVENT_COUNTER_FC 0x0B
#define MODBUS_GET_COM_EVENT_LOG_FC 0x0C
#define MODBUS_REPORT_SERVER_ID_FC 0x11
#define MODBUS_READ_DEVICE_INDENTIFICATION_FC 0x43

#define MODBUS_DATA_TABLE_DISCRETES_INPUT 0x01
#define MODBUS_DATA_TABLE_COILS 0x02
#define MODBUS_DATA_TABLE_INPUT_REGISTERS 0x03
#define MODBUS_DATA_TABLE_HOLDING_REGISTERS 0x04

#define MODBUS_ERROR_RESPONSE_SIZE 2
#define MODBUS_READ_INPUT_REGISTERS_FIXED_RESPONSE_SIZE 2
#define MODBUS_READ_BIT_FIXED_RESPONSE_SIZE 2
#define MODBUS_WRITE_SINGLE_COIL_RESPONSE_SIZE 5
#define MODBUS_WRITE_SINGLE_REGISTER_RESPONSE_SIZE 5
#define MODBUS_WRITE_MULTIPLE_REGISTER_RESPONSE_SIZE 5
#define MODBUS_WRITE_MULTIPLE_COILS_RESPONSE_SIZE 5
#define MODBUS_READ_WRITE_MULTIPLE_REGISTER_RESPONSE_SIZE 2

#define MODBUS_ERROR_OK 0x00
#define MODBUS_ERROR_ILLEGAL_FUNCTION 0x01
#define MODBUS_ERROR_ILLEGAL_DATA_ADDRESS 0x02
#define MODBUS_ERROR_ILLEGAL_DATA_VALUE 0x03
#define MODBUS_ERROR_SERVER_DEVICE_FAILURE 0x04
#define MODBUS_ERROR_ACKNOWLEDGE 0x05
#define MODBUS_ERROR_SERVER_DEVICE_BUSY 0x06
#define MODBUS_ERROR_MEMORY_PARITY_ERROR 0x08
#define MODBUS_ERROR_GATEWAY_PATH_UNAVAILABLE 0x0A
#define MODBUS_ERROR_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND 0x0B


// data is an array of bytes where byte and bits are aligned like: 00 01 02 03 04 05 06 07 | 08 09 10 11 12 13 ** **
// this differ from the protocol specification
typedef uint8_t (*ModbusReadBitsCallback)(uint8_t function_code,
		uint8_t data_table_type,uint16_t starting_address,
		uint16_t quiantity_of_registers, uint8_t* data);
typedef uint8_t (*ModbusWriteBitsCallback)(uint8_t function_code,
		uint8_t data_table_type,uint16_t starting_address,
		uint16_t quiantity_of_registers, uint8_t* data);

// data is an array of words aligned as little-endian data so ints can be read/written normally
// this differ from the protocol specification
typedef uint8_t (*ModbusReadWordsCallback)(uint8_t function_code,
		uint8_t data_table_type,uint16_t starting_address,
		uint16_t quiantity_of_registers, uint16_t* data);
typedef uint8_t (*ModbusWriteWordsCallback)(uint8_t function_code,
		uint8_t data_table_type,uint16_t starting_address,
		uint16_t quiantity_of_registers, uint16_t* data);

// input is containing the whole request without protocol header and footer (MBAP / CRC / etc ... )
typedef uint8_t (*ModbusOtherFunctionsCallback)(uint8_t function_code,
		uint8_t* input,uint32_t input_size, uint8_t* output, uint16_t *output_size,
		uint8_t prepend_size, uint8_t append_size);

typedef struct {
	ModbusReadBitsCallback read_bits;
	ModbusWriteBitsCallback write_bits;
	ModbusReadWordsCallback read_words;
	ModbusWriteWordsCallback write_words;
	ModbusOtherFunctionsCallback other_functions;
} ModbusCallbacks;

uint8_t modbus_handle_request(uint8_t *input_buffer, int input_size,
			      ModbusCallbacks *callbacks, uint8_t **output_buffer, uint16_t *output_size,
			      uint8_t prepend_size, uint8_t append_size);

#endif /* INCLUDE_MODBUS_H_ */
