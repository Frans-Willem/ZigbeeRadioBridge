#include "commands.h"
#include "net/mac/mac.h"
#include "net/netstack.h"
#include "net/packetbuf.h"

static void packet_bridge_mac_send(mac_callback_t sent, void *ptr) {
  (void)sent;
  (void)ptr;
}

static void packet_bridge_mac_input(void) {
  commands_send_event_on_packet((const uint8_t *)packetbuf_dataptr(),
                                packetbuf_datalen(),
                                packetbuf_attr(PACKETBUF_ATTR_RSSI),
                                packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY));
}
static int packet_bridge_mac_on(void) { return 0; }

static int packet_bridge_mac_off(int keep_radio_on) { return 0; }

static void packet_bridge_mac_init(void) { return; }

const struct mac_driver packet_bridge_mac_driver = {
    "packet-bridge-mac",     packet_bridge_mac_init, packet_bridge_mac_send,
    packet_bridge_mac_input, packet_bridge_mac_on,   packet_bridge_mac_off};
