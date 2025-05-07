# The micro-modbus library

The micro-modbus library is a compact and easy-to-use Modbus TCP/IP server designed for use with the FreeRTOS and lwIP libraries. It can be easily modified to support other operating systems or socket-based TCP/IP communication.

## Table of Contents

- [Callback Functions](#callback-functions)
  - [Callback Struct Definition](#callback-struct-definition)
  - [Function Codes](#function-codes)
  - [Valid Return Codes](#valid-return-codes)
  - [Memory Handling](#memory-handling)
- [Example Code](#example-code)
- [Porting the Code](#porting-the-code)
  - [Porting Away from FreeRTOS](#porting-away-from-freertos)
  - [Porting Away from lwIP](#porting-away-from-lwip)
- [Known Issues](#known-issues)
- [Author](#author)
- [Help Improve This Library](#help-improve-this-library)
- [License](#license)

## Callback Functions

This library can handle different function codes by implementing one or more callback functions to process the requests.

Below are the definitions of the five available callback functions:

```c
typedef uint8_t (*ModbusReadBitsCallback)(uint8_t function_code, uint8_t data_table_type,uint16_t starting_address, uint16_t quiantity_of_registers, uint8_t* data);
typedef uint8_t (*ModbusWriteBitsCallback)(uint8_t function_code, uint8_t data_table_type,uint16_t starting_address, uint16_t quiantity_of_registers, uint8_t* data);
typedef uint8_t (*ModbusReadWordsCallback)(uint8_t function_code, uint8_t data_table_type,uint16_t starting_address, uint16_t quiantity_of_registers, uint16_t* data);
typedef uint8_t (*ModbusWriteWordsCallback)(uint8_t function_code, uint8_t data_table_type,uint16_t starting_address, uint16_t quiantity_of_registers, uint16_t* data);
typedef uint8_t (*ModbusOtherFunctionsCallback)(uint8_t function_code, uint8_t* input,uint32_t input_size, uint8_t* output, uint32_t output_size, uint8_t prepend_size, uint8_t append_size);
```

Callback functions are defined in `modbus.h`.

### Callback Struct Definition

Use a ModbusCallback struct to configure custom callback functions that will handle requests.

If an appropriate callback function is not implemented, the server will respond with a `MODBUS_ERROR_ILLEGAL_FUNCTION` to the Modbus client.

```c
typedef struct {
	ModbusReadBitsCallback read_bits;
	ModbusWriteBitsCallback write_bits;
	ModbusReadWordsCallback read_words;
	ModbusWriteWordsCallback write_words;
	ModbusOtherFunctionsCallback other_functions;
} ModbusCallbacks;
```

The struct is defined in `modbus.h`.

### Function Codes

Below is a list over public function code:

| Code | Name                                    | Callback function            |
| ---- | --------------------------------------- | ---------------------------- |
| 0x02 | MODBUS_READ_DISCRETE_INPUTS_FC          | ModbusReadBitsCallback       |
| 0x01 | MODBUS_READ_COILS_FC                    | ModbusReadBitsCallback       |
| 0x05 | MODBUS_WRITE_SINGLE_COIL_FC             | ModbusWriteBitsCallback      |
| 0x0F | MODBUS_WRITE_MULTIPLE_COILS_FC          | ModbusWriteBitsCallback      |
| 0x04 | MODBUS_READ_INPUT_REGISTERS_FC          | ModbusReadWordsCallback      |
| 0x03 | MODBUS_READ_HOLDING_REGISTERS_FC        | ModbusReadWordsCallback      |
| 0x06 | MODBUS_WRITE_SINGLE_REGISTER_FC         | ModbusWriteWordsCallback     |
| 0x10 | MODBUS_WRITE_MULTIPLE_REGISTERS_FC      | ModbusWriteWordsCallback     |
| 0x17 | MODBUS_READ_WRITE_MULTIPLE_REGISTERS_FC | (See note)                   |
| 0x16 | MODBUS_MASK_WRITE_REGISTER_FC           | ModbusOtherFunctionsCallback |
| 0x18 | MODBUS_READ_FIFO_QUEUE_FC               | ModbusOtherFunctionsCallback |
| 0x14 | MODBUS_READ_FILE_RECORD_FC              | ModbusOtherFunctionsCallback |
| 0x15 | MODBUS_WRITE_FILE_RECORD_FC             | ModbusOtherFunctionsCallback |
| 0x07 | MODBUS_READ_EXCEPTION_STATUS_FC         | ModbusOtherFunctionsCallback |
| 0x08 | MODBUS_DIAGNOSTIC_FC                    | ModbusOtherFunctionsCallback |
| 0x0B | MODBUS_GET_COM_EVENT_COUNTER_FC         | ModbusOtherFunctionsCallback |
| 0x0C | MODBUS_GET_COM_EVENT_LOG_FC             | ModbusOtherFunctionsCallback |
| 0x11 | MODBUS_REPORT_SERVER_ID_FC              | ModbusOtherFunctionsCallback |
| 0x43 | MODBUS_READ_DEVICE_INDENTIFICATION_FC   | ModbusOtherFunctionsCallback |

**Note:** `MODBUS_READ_WRITE_MULTIPLE_REGISTERS_FC` is handled by first calling the `ModbusWriteWordsCallback` callback function and then the `ModbusReadWordsCallback` callback function.

All other function codes will map to `ModbusOtherFunctionsCallback`.

### Valid Return Codes

Below is a list of valid return values from a callback function:

| Code | Name                                                 |
| ---- | ---------------------------------------------------- |
| 0x00 | MODBUS_ERROR_OK                                      |
| 0x01 | MODBUS_ERROR_ILLEGAL_FUNCTION                        |
| 0x02 | MODBUS_ERROR_ILLEGAL_DATA_ADDRESS                    |
| 0x03 | MODBUS_ERROR_ILLEGAL_DATA_VALUE                      |
| 0x04 | MODBUS_ERROR_SERVER_DEVICE_FAILURE                   |
| 0x05 | MODBUS_ERROR_ACKNOWLEDGE                             |
| 0x06 | MODBUS_ERROR_SERVER_DEVICE_BUSY                      |
| 0x08 | MODBUS_ERROR_MEMORY_PARITY_ERROR                     |
| 0x0A | MODBUS_ERROR_GATEWAY_PATH_UNAVAILABLE                |
| 0x0B | MODBUS_ERROR_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND |

### Memory Handling

The Micro-Modbus library utilizes FreeRTOS memory functions `pvPortMalloc` and `vPortFree`.

The library will allocate the appropriate number of bytes and manage dynamic memory for the `data` pointer in the callback functions: `ModbusReadBitsCallback`, `ModbusWriteBitsCallback`, `ModbusReadWordsCallback` and `ModbusWriteWordsCallback`.

If the library is unable to allocate memory for the `data` pointer, it will return a `MODBUS_ERROR_SERVER_DEVICE_FAILURE` error message to the client.

For the `ModbusOtherFunctionsCallback`, you must manually allocate the required number of bytes using the `pvPortMalloc` function. In addition to the PDU message, you need to allocate space for the header and footer data necessary for the complete message. This additional memory space is determined by summing the values of the `prepend_size` and `append_size` arguments. You should write the PDU `output` data after the `prepend_size`. The library will automatically add the header and footer data. Furthermore, the library will free the allocated memory automatically once the data is sent back to the client.

## Example Code

Below is a simple example that implements the `ModbusReadWordsCallback` and `ModbusWriteWordsCallback` callback functions:
```c
// modbus_example.c
#include "modbus_tcp.h"

#define MODBUS_SERVER_IP "0.0.0.0"
#define MODBUS_SERVER_PORT 502
#define MODBUS_MAX_CLIENTS 5
#define MODBUS_NUMBER_OF_REGISTERS 10

uint16_t modbus_register[MODBUS_NUMBER_OF_REGISTERS] = {0};

uint8_t modbus_example_read_words_callback(uint8_t function_code, uint8_t data_table_type, uint16_t starting_address, uint16_t quantity_of_registers, uint16_t* data) {

    uint16_t c_addr = starting_address;

    for(int i = 0; i < quantity_of_registers; ++i) {
        
        if(c_addr < MODBUS_NUMBER_OF_REGISTERS)
            data[i] = modbus_register[c_addr];

        ++c_addr;

    }

    return MODBUS_ERROR_OK;

}

uint8_t modbus_example_write_words_callback(uint8_t function_code, uint8_t data_table_type,uint16_t starting_address, uint16_t quantity_of_registers, uint16_t* data) {

    uint16_t c_addr = starting_address;

    for(int i = 0; i < quantity_of_registers; ++i) {
        
        if(c_addr < MODBUS_NUMBER_OF_REGISTERS)
            modbus_register[c_addr] = data[i];

        ++c_addr;

    }

    return MODBUS_ERROR_OK;

}

// Add this function as a FreeRTOS task
void modbus_freertos_task(void const * argument) {

    ModbusCallbacks modbus_callbacks = {
        .read_words = &modbus_example_read_words_callback,
        .write_words = &modbus_example_write_words_callback
    };

    modbus_tcp_start( MODBUS_SERVER_IP, MODBUS_SERVER_PORT, MODBUS_MAX_CLIENTS, &modbus_callbacks);

    for(;;)
    {
        // Note: modbus_tcp_start should never return.
        osDelay(1000);
    }
}

```

**Note:** This code will silently ignore any attempts by the Modbus client to read from or write to register addresses that are not available. If the client executes function codes other than `MODBUS_READ_INPUT_REGISTERS_FC`, `MODBUS_READ_HOLDING_REGISTERS_FC`, `MODBUS_WRITE_SINGLE_REGISTER_FC`, `MODBUS_WRITE_MULTIPLE_REGISTERS_FC` or `MODBUS_READ_WRITE_MULTIPLE_REGISTERS_FC` it will return a `MODBUS_ERROR_ILLEGAL_FUNCTION` error message.

## Porting the Code

Porting the code for use with libraries other than FreeRTOS and lwIP is relatively straightforward.

### Porting Away from FreeRTOS

To port the code away from FreeRTOS, follow these steps:

1. Replace the memory allocation functions `pvPortMalloc` and `vPortFree` in `modbus.c` and `modbus_tcp.c` with appropriate functions for allocating and freeing memory.

2. Substitute the task creation function `xTaskCreate` in `modbus_tcp.c` with a suitable function for creating a new task or thread.

3. Additionally, update the following include files in `modbus.h`:
```c
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"
```

### Porting Away from lwIP

To port the code away from lwIP, replace the following lwIP functions in `modbus_tcp.h` with appropriate socket functions:
- `lwip_socket`
- `lwip_bind`
- `lwip_listen`
- `lwip_accept`
- `lwip_send`
- `lwip_recv`
- `lwip_close`
- `lwip_setsockopt`
- `lwip_htons`
- `lwip_inet_pton`
- `lwip_inet_ntop`

Additionally, update the following include files in `modbus_tcp.h` with the appropriate socket library:
```c
#include "lwip/inet.h"
#include "lwip/sockets.h"
```

## Known Issues

In my implementations, I was unable to get lwIP to support TCP KeepAlive. As a workaround, I have added code to automatically disconnect the client after `MODBUS_TCP_CLIENT_TIMEOUT_SECONDS` seconds, as defined in `modbus_tcp.h`.

If you successfully enable TCP KeepAlive in your socket implementation, you can remove the following lines from `modbus_tcp.c`:
```c
struct timeval timeout;
timeout.tv_sec = MODBUS_TCP_CLIENT_TIMEOUT_SECONDS;
timeout.tv_usec = 0;
lwip_setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
```

## Author

This library was created by Lars Andre Landås <landas@gmail.com>

## Help Improve This Library

I would greatly appreciate it if you could create a pull request for any code changes you make to this library in your project. Your contributions can help enhance the library for everyone!

## License

This project is licensed under the MIT License, a permissive open-source license that allows users to freely use, modify, and distribute the software with minimal restrictions.

The only requirement is that the original license and copyright notice must be included in all copies or substantial portions of the software. This approach not only promotes widespread use but also ensures that the contributions of the original authors are acknowledged.

[MIT License](https://en.wikipedia.org/wiki/MIT_License)
