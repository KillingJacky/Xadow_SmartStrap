/* This library is for communicating with a Pebble Time via the accessory port for Smart Straps. */

#include "PebbleSerial.h"
#include "hdlc.h"

#define UNLIKELY(x) (__builtin_expect(!!(x), 0))

#define FLAGS_VERSION_MASK     ((uint8_t)0x0F)
#define FLAGS_IS_READ_MASK     ((uint8_t)0x10)
#define FLAGS_IS_MASTER_MASK   ((uint8_t)0x20)
#define FLAGS_RESERVED_MASK    ((uint8_t)0xC0)
#define FLAGS_GET(flags, mask) (((flags) & mask) >> __builtin_ctz(mask))
#define FLAGS_SET(flags, mask, value) \
  (flags) = ((flags) & ~mask) | (((value) << __builtin_ctz(mask)) & mask)

#define FRAME_MIN_LENGTH              3
#define FRAME_FLAGS_OFFSET            0
#define FRAME_PROTOCOL_OFFSET         1
#define FRAME_PAYLOAD_OFFSET          2
#define LINK_CONTROL_VERSION          1

typedef enum {
  LinkControlConnectionRequest = 1
} LinkControlType;

typedef struct {
  uint8_t *payload;
  uint8_t protocol;
  uint8_t parity;
  uint8_t length;
  uint8_t read_length;
  uint8_t parity_buffer;
  bool should_drop;
  bool read_ready;
  bool is_read;
} PebbleFrameInfo;

static HdlcStreamingContext s_hdlc_ctx;
static PebbleFrameInfo s_frame;
static PebbleProtocol s_protocol;
static PebbleCallbacks s_callbacks;

void pebble_init(PebbleProtocol protocol, PebbleCallbacks callbacks) {
  s_protocol = protocol;
  s_callbacks = callbacks;
  s_callbacks.control(PebbleControlDisableTX);
}

bool pebble_handle_byte(uint8_t data, PebbleProtocol *protocol, uint8_t *length, bool *is_read) {
  if (!s_frame.read_ready) {
    // we aren't ready to read from the device yet
    return false;
  }

  if (UNLIKELY(data == HDLC_FLAG)) {
    if (s_frame.length == 0) {
      // this is the starting flag, so just ignore it
      return false;
    } else if (UNLIKELY(s_frame.should_drop)) {
      // we already know it's invalid
    } else if (UNLIKELY(hdlc_streaming_decode_finish(&s_hdlc_ctx) == false)) {
      // decoding error
      s_frame.should_drop = true;
    } else if (UNLIKELY(s_frame.length < FRAME_MIN_LENGTH)) {
      // this frame was too small
      s_frame.should_drop = true;
    } else if (UNLIKELY(s_frame.parity != 0)) {
      // parity error
      s_frame.should_drop = true;
    }

    if (s_frame.should_drop) {
      // drop this frame and prepare for the next one
      pebble_prepare_for_read(s_frame.payload, s_frame.read_length);
    } else {
      // this is a valid frame
      s_frame.read_ready = false;
      *protocol = s_frame.protocol;
      *length = s_frame.length - FRAME_MIN_LENGTH;
      *is_read = s_frame.is_read;
      return true;
    }
  } else if (UNLIKELY(s_frame.should_drop)) {
    // this frame has already been determined to be invalid
  } else if (hdlc_streaming_decode(&s_hdlc_ctx, &data)) {
    // process this byte
    s_frame.parity ^= data;
    if (s_frame.length == FRAME_FLAGS_OFFSET) {
      // this is the flags octet
      if (UNLIKELY(FLAGS_GET(data, FLAGS_VERSION_MASK) != PEBBLE_PROTOCOL_VERSION)) {
        // invalid version
        s_frame.should_drop = true;
      } else if (UNLIKELY(FLAGS_GET(data, FLAGS_IS_MASTER_MASK) != 1)) {
        // we should only be geting frames from the slave
        s_frame.should_drop = true;
      } else if (UNLIKELY(FLAGS_GET(data, FLAGS_RESERVED_MASK) != 0)) {
        // the reserved flags should be set to 0
        s_frame.should_drop = true;
      } else {
        // the header flags are valid
        s_frame.is_read = FLAGS_GET(data, FLAGS_IS_READ_MASK);
      }
    } else if (s_frame.length == FRAME_PROTOCOL_OFFSET) {
      // this is the protocol octet
      switch (data) {
      case PebbleProtocolLinkControl:
      case PebbleProtocolRawData:
        // valid protocol
        s_frame.protocol = data;
        break;
      case PebbleProtocolInvalid:
      default:
        // invalid protocol
        s_frame.should_drop = true;
        break;
      }
    } else {
      const uint8_t payload_length = s_frame.length - FRAME_PAYLOAD_OFFSET;
      if (payload_length - 1 >= s_frame.read_length) {
        // no space in the receive buffer for this byte
        s_frame.should_drop = true;
      } else {
        // keep a 1 byte buffer in memory so we don't write the parity into the payload
        if (payload_length > 0) {
          // put the previous byte into the payload buffer
          s_frame.payload[payload_length-1] = s_frame.parity_buffer;
        }
        s_frame.parity_buffer = data;
      }
    }
    // increment the length
    s_frame.length++;
  }
  return false;
}

void pebble_prepare_for_read(uint8_t *buffer, uint8_t length) {
  hdlc_streaming_decode_start(&s_hdlc_ctx);
  s_frame = (PebbleFrameInfo) { 0 };
  s_frame.payload = buffer;
  s_frame.read_length = length;
  s_frame.read_ready = true;
}

static void prv_send_flag(void) {
  s_callbacks.write_byte(HDLC_FLAG);
}

static void prv_send_byte(uint8_t data, uint8_t *parity) {
  *parity ^= data;
  if (hdlc_encode(&data)) {
    s_callbacks.write_byte(HDLC_ESCAPE);
  }
  s_callbacks.write_byte(data);
}

static void prv_write_internal(PebbleProtocol protocol, const uint8_t *data, uint8_t length) {
  uint8_t parity = 0;
  uint8_t i;

  // enable tx
  s_callbacks.control(PebbleControlEnableTX);

  // send flag
  prv_send_flag();

  // send header flags
  uint8_t flags = 0;
  FLAGS_SET(flags, FLAGS_VERSION_MASK, PEBBLE_PROTOCOL_VERSION);
  prv_send_byte(flags, &parity);

  // send protocol
  prv_send_byte((uint8_t)protocol, &parity);

  // send data
  for (i = 0; i < length; i++) {
    prv_send_byte(data[i], &parity);
  }

  // send parity
  prv_send_byte(parity, &parity);

  // send flag
  prv_send_flag();

  // flush and disable tx
  s_callbacks.control(PebbleControlFlushTX);
  s_callbacks.control(PebbleControlDisableTX);
}

void pebble_write(const uint8_t *data, uint8_t length) {
  prv_write_internal(s_protocol, data, length);
}

void pebble_handle_link_control(uint8_t *buffer) {
  LinkControlType type = buffer[0];
  if (type == LinkControlConnectionRequest) {
    uint8_t version = buffer[1];
    if (version == LINK_CONTROL_VERSION) {
      // re-use the buffer for the response
      buffer[1] = s_protocol;
      prv_write_internal(PebbleProtocolLinkControl, buffer, 2);
    }
  }
}
