/*
 * Copyright (c) 2019, Frans-Willem Hardijzer <fw@hardijzer.nl>
 */

#include "contiki.h"
#include "dev/io-arch.h"
#include "lib/ringbuf.h"
#include "sys/process.h"
#include <stdbool.h>
#include "net/netstack.h"
#include "serial_protocol.h"
#include "commands.h"

PROCESS(packetbridge_process, "packet bridge process");
AUTOSTART_PROCESSES(&packetbridge_process);

static uint8_t receive_ringbuf_buffer[128];
static struct ringbuf receive_ringbuf;

static int on_serial_byte(uint8_t c) {
  ringbuf_put(&receive_ringbuf, c);
  process_poll(&packetbridge_process);
  return 1;
}

PROCESS_THREAD(packetbridge_process, ev, data) {
  PROCESS_BEGIN();
  (void)data; // Stop whining

  ringbuf_init(&receive_ringbuf, receive_ringbuf_buffer,
               sizeof(receive_ringbuf_buffer));

  io_arch_set_input(on_serial_byte);

  while (true) {
    PROCESS_YIELD();
    if (ev == PROCESS_EVENT_POLL) {
      serial_process_input(&receive_ringbuf);
    }
  }
  PROCESS_END();
}
