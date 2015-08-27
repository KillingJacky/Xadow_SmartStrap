#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PEBBLE_BAUDRATE         115200
#define PEBBLE_PROTOCOL_VERSION 1
#define PEBBLE_MAX_PAYLOAD      80

typedef enum {
  PebbleProtocolInvalid = 0x00,
  PebbleProtocolLinkControl = 0x01,
  PebbleProtocolRawData = 0x02,
  PebbleProtocolNum
} PebbleProtocol;

typedef enum {
  PebbleControlEnableTX,
  PebbleControlDisableTX,
  PebbleControlFlushTX
} PebbleControl;

typedef void (*PebbleWriteByteCallback)(uint8_t data);
typedef void (*PebbleControlCallback)(PebbleControl cmd);

typedef struct {
  PebbleWriteByteCallback write_byte;
  PebbleControlCallback control;
} PebbleCallbacks;

void pebble_init(PebbleProtocol protocol, PebbleCallbacks callbacks);
void pebble_prepare_for_read(uint8_t *buffer, uint8_t length);
bool pebble_handle_byte(uint8_t data, PebbleProtocol *protocol, uint8_t *length, bool *is_read);
void pebble_write(const uint8_t *payload, uint8_t length);
void pebble_handle_link_control(uint8_t *buffer);
