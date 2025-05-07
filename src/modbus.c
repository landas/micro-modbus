/*******************************************************************************
 *   @file   modbus.c
 *   @brief  Implementation of general Modbus protocol handling
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


#include "modbus.h"


void modbus_create_error_response(uint8_t **send_buffer, uint16_t *send_size,
				  uint8_t function_code, uint8_t error_code, uint8_t prepend_size,
				  uint8_t append_size)
{
	*send_size = MODBUS_ERROR_RESPONSE_SIZE + prepend_size + append_size;
	*send_buffer = pvPortMalloc(*send_size);

	if (*send_buffer == NULL) {
		*send_size = 0;
	} else {
		uint8_t *send_buffer_cursor = *send_buffer + prepend_size;

		send_buffer_cursor[0] = function_code +
					0x80; // calculate function code for error
		send_buffer_cursor[1] = error_code;
	}
}

void modbus_flip_byte_bits(uint8_t *byte)
{
	uint8_t flipped = 0;
	for (uint8_t i = 0; i < 8; i++) {
		flipped |= ((*byte >> i) & 1) << (7 - i);
	}

	*byte = flipped;
}

void modbus_flip_word(uint16_t *word)
{
	uint16_t register_value_tmp = *word;
	uint8_t *word_bytes = (uint8_t*)word;
	word_bytes[0] = (uint8_t) (register_value_tmp >> 8);
	word_bytes[1] = (uint8_t) (register_value_tmp);
}

uint8_t modbus_write_request_handler(ModbusCallbacks *callbacks,
				     uint8_t *request_data, uint32_t request_data_size, uint8_t **send_buffer,
				     uint16_t *send_size, uint8_t prepend_size, uint8_t append_size)
{

	uint8_t ret = MODBUS_ERROR_OK;
	uint8_t function_code = request_data[0];
	uint16_t output_address = (uint16_t) (request_data[1] << 8)
				  + request_data[2];
	uint16_t output_value = (uint16_t) (request_data[3] << 8) + request_data[4];

	if (function_code == MODBUS_WRITE_SINGLE_COIL_FC
	    && callbacks->write_bits != NULL) {
		uint8_t coil_data = (uint8_t) (output_value == 0 ? 0 : 1);
		ret = callbacks->write_bits(function_code, MODBUS_DATA_TABLE_COILS,
					    output_address, 1, &coil_data);
	} else if (function_code == MODBUS_WRITE_SINGLE_REGISTER_FC
		   && callbacks->write_words != NULL) {
		ret = callbacks->write_words(function_code,
					     MODBUS_DATA_TABLE_HOLDING_REGISTERS, output_address, 1,
					     &output_value);
	} else {
		ret = MODBUS_ERROR_ILLEGAL_FUNCTION;
	}

	if (ret == MODBUS_ERROR_OK) {
		*send_size = MODBUS_WRITE_SINGLE_REGISTER_RESPONSE_SIZE + prepend_size
			     + append_size;
		*send_buffer = pvPortMalloc(*send_size);

		if (*send_buffer == NULL) {
			return MODBUS_ERROR_SERVER_DEVICE_FAILURE;
		}

		(*send_buffer)[0] = function_code;
		(*send_buffer)[1] = (uint8_t) (output_address >> 8);
		(*send_buffer)[2] = (uint8_t) (output_address);
		(*send_buffer)[3] = (uint8_t) (output_value >> 8);
		(*send_buffer)[4] = (uint8_t) (output_value);
	}

	return ret;
}

uint8_t modbus_read_word_request_handler(ModbusCallbacks *callbacks,
		uint8_t *input_buffer, uint32_t input_size, uint8_t **output_buffer,
		uint16_t *output_size, uint8_t prepend_size, uint8_t append_size)
{

	uint8_t ret = MODBUS_ERROR_OK;

	uint8_t function_code = input_buffer[0];
	uint16_t starting_address = (uint16_t) (input_buffer[1] << 8)
				    + input_buffer[2];
	uint16_t quantity_of_registers = (uint16_t) (input_buffer[3] << 8)
					 + input_buffer[4];

	uint8_t byte_count = 2 * quantity_of_registers;

	*output_size = MODBUS_READ_INPUT_REGISTERS_FIXED_RESPONSE_SIZE + byte_count
		       + prepend_size + append_size;
	*output_buffer = pvPortMalloc(*output_size);

	if (*output_buffer == NULL) {
		return MODBUS_ERROR_SERVER_DEVICE_FAILURE;
	}

	uint16_t *register_value =
		(uint16_t*)(*output_buffer + prepend_size +
			    MODBUS_READ_INPUT_REGISTERS_FIXED_RESPONSE_SIZE);

	memset(register_value, 0, byte_count);

	if (callbacks->read_words != NULL) {
		ret = callbacks->read_words(function_code,
					    function_code == MODBUS_READ_HOLDING_REGISTERS_FC ?
					    MODBUS_DATA_TABLE_HOLDING_REGISTERS :
					    MODBUS_DATA_TABLE_INPUT_REGISTERS, starting_address,
					    quantity_of_registers, register_value);
	} else {
		ret = MODBUS_ERROR_ILLEGAL_FUNCTION;
	}

	if (ret != MODBUS_ERROR_OK) {
		return ret;
	}

	uint8_t *output_buffer_cursor = *output_buffer + prepend_size;
	output_buffer_cursor[0] = function_code;
	output_buffer_cursor[1] = byte_count;

	for (int i = 0; i < quantity_of_registers; ++i) {
		modbus_flip_word(&register_value[i]);
	}

	return ret;
}

uint8_t modbus_read_bit_request_handler(ModbusCallbacks *callbacks,
					uint8_t *request_data, uint32_t request_data_size, uint8_t **output_buffer,
					uint16_t *output_size, uint8_t prepend_size, uint8_t append_size)
{

	uint8_t ret = MODBUS_ERROR_OK;

	uint8_t function_code = request_data[0];
	uint16_t starting_address = (uint16_t) (request_data[1] << 8)
				    + request_data[2];
	uint16_t quantity_of_inputs = (uint16_t) (request_data[3] << 8)
				      + request_data[4];

	uint8_t byte_count = (quantity_of_inputs / 8)
			     + (quantity_of_inputs % 8 == 0 ? 0 : 1);

	*output_size = MODBUS_READ_BIT_FIXED_RESPONSE_SIZE + byte_count
		       + prepend_size + append_size;

	*output_buffer = pvPortMalloc(*output_size);

	if (*output_buffer == NULL) {
		return MODBUS_ERROR_SERVER_DEVICE_FAILURE;
	}

	uint8_t *input_status = *output_buffer + prepend_size +
				MODBUS_READ_BIT_FIXED_RESPONSE_SIZE;

	memset(input_status, 0, byte_count);

	if (callbacks->read_bits != NULL) {

		ret = callbacks->read_bits(function_code,
					   function_code == MODBUS_READ_COILS_FC ?
					   MODBUS_DATA_TABLE_COILS :
					   MODBUS_DATA_TABLE_DISCRETES_INPUT,
					   starting_address, quantity_of_inputs, input_status);
	} else {
		ret = MODBUS_ERROR_ILLEGAL_FUNCTION;
	}

	if (ret != MODBUS_ERROR_OK) {
		return ret;
	}

	uint8_t *send_buffer_cursor = *output_buffer + prepend_size;
	send_buffer_cursor[0] = function_code;
	send_buffer_cursor[1] = byte_count;

	// Flip bits in byte
	for(uint8_t i = 0; i < byte_count; ++i) {
		modbus_flip_byte_bits(&send_buffer_cursor[2+i]);
	}

	return ret;
}

uint8_t modbus_write_multiple_registers_request_handler(
	ModbusCallbacks *callbacks, uint8_t *input_buffer,
	uint32_t input_size, uint8_t **output_buffer, uint16_t *output_size,
	uint8_t prepend_size, uint8_t append_size)
{

	uint8_t ret = MODBUS_ERROR_OK;

	if (callbacks->write_words == NULL) {
		return MODBUS_ERROR_ILLEGAL_FUNCTION;
	}

	uint8_t function_code = input_buffer[0];
	uint16_t starting_address = (uint16_t) (input_buffer[1] << 8)
				    + input_buffer[2];
	uint16_t quantity_of_registers = (uint16_t) (input_buffer[3] << 8)
					 + input_buffer[4];
	uint8_t byte_count = input_buffer[5];

	*output_size = MODBUS_WRITE_MULTIPLE_REGISTER_RESPONSE_SIZE + prepend_size +
		       append_size;
	*output_buffer = pvPortMalloc(*output_size);

	if (*output_buffer == NULL) {
		return MODBUS_ERROR_SERVER_DEVICE_FAILURE;
	}

	uint16_t *registers_value = (uint16_t*)(input_buffer + 6);

	for (int i = 0; i < byte_count; ++i) {
		modbus_flip_word(&registers_value[i]);
	}

	ret = callbacks->write_words(function_code,
				     MODBUS_DATA_TABLE_HOLDING_REGISTERS, starting_address,
				     quantity_of_registers, registers_value);

	uint8_t* output_buffer_cursor = (*output_buffer + prepend_size);

	memcpy(output_buffer_cursor, input_buffer,
	       MODBUS_WRITE_MULTIPLE_REGISTER_RESPONSE_SIZE);

	return ret;
}

uint8_t modbus_write_multiple_coils_request_handler(
	ModbusCallbacks *callbacks, uint8_t *input_buffer,
	uint32_t input_size, uint8_t **output_buffer, uint16_t *output_size,
	uint8_t prepend_size, uint8_t append_size)
{

	if (callbacks->write_bits == NULL) {
		return MODBUS_ERROR_ILLEGAL_FUNCTION;
	}

	uint8_t ret = MODBUS_ERROR_OK;
	uint8_t function_code = input_buffer[0];
	uint16_t starting_address = (uint16_t) (input_buffer[1] << 8)
				    + input_buffer[2];
	uint16_t quantity_of_outputs = (uint16_t) (input_buffer[3] << 8)
				       + input_buffer[4];
	uint8_t byte_count = input_buffer[5];

	*output_size = MODBUS_WRITE_MULTIPLE_COILS_RESPONSE_SIZE + prepend_size +
		       append_size;
	*output_buffer = pvPortMalloc(*output_size);

	if (*output_buffer == NULL) {
		return MODBUS_ERROR_SERVER_DEVICE_FAILURE;
	}

	uint8_t *outputs_value = input_buffer + 6;

	for (int i = 0; i < byte_count; ++i) {
		modbus_flip_byte_bits(&outputs_value[i]);
	}

	ret = callbacks->write_bits(function_code,
				    MODBUS_DATA_TABLE_COILS, starting_address,
				    quantity_of_outputs, outputs_value);

	*output_buffer += prepend_size;
	memcpy(*output_buffer, input_buffer,
	       MODBUS_WRITE_MULTIPLE_REGISTER_RESPONSE_SIZE);

	return ret;
}

uint8_t modbus_read_write_multiple_registers_request_handler(
	ModbusCallbacks *callbacks, uint8_t *input_buffer,
	uint32_t input_size, uint8_t **output_buffer, uint16_t *output_size,
	uint8_t prepend_size, uint8_t append_size)
{

	if (callbacks->read_words == NULL || callbacks->write_words == NULL) {
		return MODBUS_ERROR_ILLEGAL_FUNCTION;
	}

	uint8_t ret = MODBUS_ERROR_OK;

	ret = modbus_write_multiple_coils_request_handler(
		      callbacks,
		      input_buffer,
		      input_size,
		      output_buffer,
		      output_size,
		      prepend_size,
		      append_size);

	if(*output_buffer != NULL) {
		vPortFree(*output_buffer);
	}

	if (ret != MODBUS_ERROR_OK) {
		return ret;
	}

	ret = modbus_read_word_request_handler(
		      callbacks,
		      input_buffer,
		      input_size,
		      output_buffer,
		      output_size,
		      prepend_size,
		      append_size);

	return ret;
}

uint8_t modbus_other_functions_request_handler(
	ModbusCallbacks *callbacks, uint8_t function_code, uint8_t *input_buffer,
	uint32_t input_size, uint8_t **output_buffer, uint16_t *output_size,
	uint8_t prepend_size, uint8_t append_size)
{

	if (callbacks->other_functions == NULL) {
		return MODBUS_ERROR_ILLEGAL_FUNCTION;
	}

	return callbacks->other_functions(
		       function_code,
		       input_buffer,
		       input_size,
		       *output_buffer,
		       output_size,
		       prepend_size,
		       append_size);
}

uint8_t modbus_handle_request(uint8_t *input_buffer, int input_size,
			      ModbusCallbacks *callbacks, uint8_t **output_buffer, uint16_t *output_size,
			      uint8_t prepend_size, uint8_t append_size)
{

	uint8_t ret = MODBUS_ERROR_SERVER_DEVICE_FAILURE;
	uint8_t function_code = input_buffer[0];

	switch (function_code) {
	case MODBUS_READ_COILS_FC:
	case MODBUS_READ_DISCRETE_INPUTS_FC:
		ret = modbus_read_bit_request_handler(callbacks, input_buffer,
						      input_size, output_buffer, output_size, prepend_size,
						      append_size);
		break;
	case MODBUS_READ_HOLDING_REGISTERS_FC:
	case MODBUS_READ_INPUT_REGISTERS_FC:
		ret = modbus_read_word_request_handler(callbacks, input_buffer, input_size,
						       output_buffer, output_size, prepend_size, append_size);
		break;
	case MODBUS_WRITE_SINGLE_COIL_FC:
	case MODBUS_WRITE_SINGLE_REGISTER_FC:
		ret = modbus_write_request_handler(callbacks, input_buffer, input_size,
						   output_buffer, output_size, prepend_size, append_size);
		break;
	case MODBUS_WRITE_MULTIPLE_REGISTERS_FC:
		ret = modbus_write_multiple_registers_request_handler(callbacks,
			input_buffer, input_size, output_buffer, output_size,
			prepend_size, append_size);
		break;
	case MODBUS_WRITE_MULTIPLE_COILS_FC:
		ret = modbus_write_multiple_coils_request_handler(callbacks,
			input_buffer, input_size, output_buffer, output_size,
			prepend_size, append_size);
		break;
	case MODBUS_READ_WRITE_MULTIPLE_REGISTERS_FC:
		ret = modbus_read_write_multiple_registers_request_handler(callbacks,
			input_buffer, input_size, output_buffer, output_size,
			prepend_size, append_size);
		break;
	default:
		ret = MODBUS_ERROR_ILLEGAL_FUNCTION;
		break;
	}

	if (ret != MODBUS_ERROR_OK) {
		if (*output_buffer != NULL) {
			vPortFree(*output_buffer);
			*output_buffer = NULL;
			output_buffer = NULL;
		}

		modbus_create_error_response(output_buffer, output_size,
					     function_code, ret, prepend_size, append_size);
	}

	return MODBUS_ERROR_OK;
}
