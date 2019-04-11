/*
 * Copyright (c) 2019, Frans-Willem Hardijzer <fw@hardijzer.nl>
 */

#include "contiki.h"
#include "dev/io-arch.h"
#include "lib/ringbuf.h"
#include "sys/process.h"
#include <stdbool.h>
#include "net/netstack.h"

PROCESS(packetbridge_process, "packet bridge process");
AUTOSTART_PROCESSES(&packetbridge_process);

#define REQUEST_ID_SIZE 2
#define LENGTH_SIZE 2

static uint8_t receive_ringbuf_buffer[128];
static struct ringbuf receive_ringbuf;
static struct {
  // Which state are we in ?
  enum {
    State_WaitingForPrefix = 0,
    State_WaitingForCommand,
    State_WaitingForRequestId,
    State_WaitingForLength,
    State_WaitingForData
  } state;
  /* In multi-byte states, this is the offset. e.g.
   * state=WaitingForLength, state_offset=1 means we're
   * waiting for the second byte of length data.
   */
  size_t offset;
  // Command identifier
  uint8_t command_id;
  // Request identifier
  size_t request_id;
  // Length of data
  size_t data_length;
  uint8_t data[255];
} state;

typedef enum {
  // These are sent from PC -> Dongle, and answered with
  Command_Request_Radio_Prepare = 0,
  Command_Request_Radio_Transmit,
  Command_Request_Radio_Send,
  Command_Request_Radio_ChannelClear,
  Command_Request_Radio_On,
  Command_Request_Radio_Off,
  Command_Request_Radio_GetValue,
  Command_Request_Radio_SetValue,
  Command_Request_Radio_GetObject,
  Command_Request_Radio_SetObject,
  Command_Response_OK = 0x80,
  Command_Response_Err,
  Command_Event_Radio_OnPacket = 0xC0
} command_t;

static const uint8_t expectedPrefix[] = {'Z', 'P', 'B'};

static void reset_state() { memset(&state, 0, sizeof(state)); }

static void send_response(command_t command, size_t request_id,
                          const uint8_t *data, size_t data_len) {
  size_t i;
  if ((data_len >> (LENGTH_SIZE * 8)) != 0)
    data_len = (1 << (LENGTH_SIZE * 8)) - 1;

  for (i = 0; i < sizeof(expectedPrefix); i++) {
    io_arch_writeb(expectedPrefix[i]);
  }
  io_arch_writeb((uint8_t)command);
  for (i = 0; i < REQUEST_ID_SIZE; i++) {
    io_arch_writeb((request_id >> ((REQUEST_ID_SIZE - (i + 1)) * 8)) & 0xFF);
  }
  for (i = 0; i < LENGTH_SIZE; i++) {
    io_arch_writeb((data_len >> ((LENGTH_SIZE - (i + 1)) * 8)) & 0xFF);
  }
  for (i = 0; i < data_len; i++) {
    io_arch_writeb(data[i]);
  }
  io_arch_flush();
}

static void send_ok_nodata(size_t request_id) {
  send_response(Command_Response_OK, request_id, NULL, 0);
}

static void send_error_int(size_t request_id, int ret) {
  static uint8_t data[4];
  data[0] = (ret >> 24) & 0xFF;
  data[1] = (ret >> 16) & 0xFF;
  data[2] = (ret >> 8) & 0xFF;
  data[3] = (ret >> 0) & 0xFF;
  send_response(Command_Response_Err, request_id, data, sizeof(data));
}

static void send_expect_zero_response(size_t request_id, int api_retval) {
  if (api_retval == 0) {
    send_ok_nodata(request_id);
  } else {
    send_error_int(request_id, api_retval);
  }
}

static unsigned int uint_from_bytes(const uint8_t *data, size_t data_len) {
  unsigned int retval = 0;
  size_t i = 0;
  for (i = 0; i < data_len; i++) {
    retval = (retval << 8) | data[i];
  }
  return retval;
}

static void handle_command(command_t cmd, size_t request_id,
                           const uint8_t *data, size_t data_len) {
  switch (cmd) {
  case Command_Request_Radio_Prepare:
    send_expect_zero_response(request_id,
                              NETSTACK_RADIO.prepare(data, data_len));
    break;
  case Command_Request_Radio_Transmit:
    send_expect_zero_response(
        request_id, NETSTACK_RADIO.transmit(
                        (unsigned short)uint_from_bytes(data, data_len)));
    break;
  case Command_Request_Radio_Send:
    send_expect_zero_response(request_id, NETSTACK_RADIO.send(data, data_len));
    break;
    // TODO ChannelClear
  case Command_Request_Radio_On:
    send_expect_zero_response(request_id, NETSTACK_RADIO.on());
    break;
  case Command_Request_Radio_Off:
    send_expect_zero_response(request_id, NETSTACK_RADIO.off());
    break;
  default:
    send_response(Command_Response_Err, request_id, NULL, 0);
    break;
  }
}

static void process_receive_buffer(void) {
  int chr;
  while (true) {
    chr = ringbuf_get(&receive_ringbuf);
    if (chr == -1)
      break;
    switch (state.state) {
    case State_WaitingForPrefix: {
      if (expectedPrefix[state.offset] != chr) {
        reset_state();
      } else if (++state.offset == sizeof(expectedPrefix)) {
        state.state = State_WaitingForCommand;
        state.offset = 0;
      }
      break;
    }
    case State_WaitingForCommand: {
      state.command_id = chr;
      state.request_id = 0;
      state.offset = 0;
      state.state = State_WaitingForRequestId;
      break;
    }
    case State_WaitingForRequestId: {
      state.request_id = (state.request_id << 8) | chr;
      if (++state.offset >= REQUEST_ID_SIZE) {
        state.state = State_WaitingForLength;
        state.offset = 0;
        state.data_length = 0;
      }
      break;
    }
    case State_WaitingForLength: {
      state.data_length = (state.data_length << 8) | chr;
      if (++state.offset >= LENGTH_SIZE) {
        if (state.data_length > sizeof(state.data)) {
          reset_state();
        } else if (state.data_length == 0) {
          handle_command(state.command_id, state.request_id, state.data,
                         state.data_length);
          reset_state();
        } else {
          state.offset = 0;
          state.state = State_WaitingForData;
        }
      }
      break;
    }
    case State_WaitingForData: {
      state.data[state.offset++] = chr;
      if (state.offset >= state.data_length) {
        handle_command((command_t)state.command_id, state.request_id,
                       state.data, state.data_length);
        reset_state();
      }
      break;
    }
    }
  }
}

static int on_serial_byte(uint8_t c) {
  ringbuf_put(&receive_ringbuf, c);
  process_poll(&packetbridge_process);
  return 1;
}

PROCESS_THREAD(packetbridge_process, ev, data) {
  PROCESS_BEGIN();

  ringbuf_init(&receive_ringbuf, receive_ringbuf_buffer,
               sizeof(receive_ringbuf_buffer));

  io_arch_set_input(on_serial_byte);

  while (true) {
    PROCESS_YIELD();
    if (ev == PROCESS_EVENT_POLL) {
      process_receive_buffer();
    }
  }
  PROCESS_END();
}
