#include "commands.h"
#include "net/mac/mac.h"
#include "net/netstack.h"
#include "net/packetbuf.h"

static void packet_bridge_rdc_send(mac_callback_t sent, void *ptr) {
  (void)sent;
  (void)ptr;
}

static void packet_bridge_rdc_send_list(mac_callback_t sent, void *ptr,
                                        struct rdc_buf_list *list) {
  (void)sent;
  (void)ptr;
  (void)list;
}

static void packet_bridge_rdc_input(void) {
  commands_send_event_on_packet((const uint8_t *)packetbuf_dataptr(),
                                packetbuf_datalen(),
                                packetbuf_attr(PACKETBUF_ATTR_RSSI),
                                packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY));
}
static int packet_bridge_rdc_on(void) { return 1; }

static int packet_bridge_rdc_off(int keep_radio_on) { return 0; }

static unsigned short packet_bridge_rdc_cca(void) { return 0; }

static void packet_bridge_rdc_init(void) { return; }

const struct rdc_driver packet_bridge_rdc_driver = {
    "packet-bridge-rdc",     packet_bridge_rdc_init,
    packet_bridge_rdc_send,  packet_bridge_rdc_send_list,
    packet_bridge_rdc_input, packet_bridge_rdc_on,
    packet_bridge_rdc_off,   packet_bridge_rdc_cca};
