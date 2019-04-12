#include "serial_protocol.h"
#include "commands.h"
#include "dev/io-arch.h"
#include <stdbool.h>

#define REQUEST_ID_SIZE 2
#define LENGTH_SIZE 2

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
} serial_state;

static const uint8_t serial_prefix[] = {'Z', 'P', 'B'};

static void serial_reset_state() {
  memset(&serial_state, 0, sizeof(serial_state));
}

void serial_process_input(struct ringbuf *input_buffer) {
  int chr;
  while (true) {
    chr = ringbuf_get(input_buffer);
    if (chr == -1)
      break;
    switch (serial_state.state) {
    case State_WaitingForPrefix: {
      if (serial_prefix[serial_state.offset] != chr) {
        serial_reset_state();
      } else if (++serial_state.offset == sizeof(serial_prefix)) {
        serial_state.state = State_WaitingForCommand;
        serial_state.offset = 0;
      }
      break;
    }
    case State_WaitingForCommand: {
      serial_state.command_id = chr;
      serial_state.request_id = 0;
      serial_state.offset = 0;
      serial_state.state = State_WaitingForRequestId;
      break;
    }
    case State_WaitingForRequestId: {
      serial_state.request_id = (serial_state.request_id << 8) | chr;
      if (++serial_state.offset >= REQUEST_ID_SIZE) {
        serial_state.state = State_WaitingForLength;
        serial_state.offset = 0;
        serial_state.data_length = 0;
      }
      break;
    }
    case State_WaitingForLength: {
      serial_state.data_length = (serial_state.data_length << 8) | chr;
      if (++serial_state.offset >= LENGTH_SIZE) {
        if (serial_state.data_length > sizeof(serial_state.data)) {
          serial_reset_state();
        } else if (serial_state.data_length == 0) {
          commands_handle_command((command_t)serial_state.command_id,
                                  serial_state.request_id, serial_state.data,
                                  serial_state.data_length);
          serial_reset_state();
        } else {
          serial_state.offset = 0;
          serial_state.state = State_WaitingForData;
        }
      }
      break;
    }
    case State_WaitingForData: {
      serial_state.data[serial_state.offset++] = chr;
      if (serial_state.offset >= serial_state.data_length) {
        commands_handle_command((command_t)serial_state.command_id,
                                serial_state.request_id, serial_state.data,
                                serial_state.data_length);
        serial_reset_state();
      }
      break;
    }
    }
  }
}

static size_t serial_size_remaining = 0;

void serial_send_header(uint8_t command, size_t request_id, size_t data_len) {
  size_t i;
  if ((data_len >> (LENGTH_SIZE * 8)) != 0)
    data_len = (1 << (LENGTH_SIZE * 8)) - 1;

  for (i = 0; i < sizeof(serial_prefix); i++) {
    io_arch_writeb(serial_prefix[i]);
  }
  io_arch_writeb((uint8_t)command);
  for (i = 0; i < REQUEST_ID_SIZE; i++) {
    io_arch_writeb((request_id >> ((REQUEST_ID_SIZE - (i + 1)) * 8)) & 0xFF);
  }
  for (i = 0; i < LENGTH_SIZE; i++) {
    io_arch_writeb((data_len >> ((LENGTH_SIZE - (i + 1)) * 8)) & 0xFF);
  }
  serial_size_remaining = data_len;
}

void serial_send_data(const uint8_t *data, size_t data_len) {
  size_t i;
  if (data_len > serial_size_remaining) {
    data_len = serial_size_remaining;
  }
  for (i = 0; i < data_len; i++) {
    io_arch_writeb(data[i]);
  }
  serial_size_remaining -= data_len;
}

void serial_send_flush() {
  while (serial_size_remaining--)
    io_arch_writeb(0);
  io_arch_flush();
}

void serial_send_frame(uint8_t command, size_t request_id, const uint8_t *data,
                       size_t data_len) {
  serial_send_header(command, request_id, data_len);
  serial_send_data(data, data_len);
  serial_send_flush();
}
