#include "commands.h"
#include "net/netstack.h"
#include "serial_protocol.h"
#include "serialization.h"

void commands_handle_command(command_t cmd, size_t request_id,
                             const uint8_t *data, size_t data_len) {
  switch (cmd) {
  case Command_Request_Radio_Prepare:
    commands_send_int_response(request_id, 0,
                               NETSTACK_RADIO.prepare(data, data_len));
    break;
  case Command_Request_Radio_Transmit:
    commands_send_int_response(
        request_id, RADIO_TX_OK,
        NETSTACK_RADIO.transmit(deserialize_ushort(data, data_len)));
    break;
  case Command_Request_Radio_Send:
    commands_send_int_response(request_id, RADIO_TX_OK,
                               NETSTACK_RADIO.send(data, data_len));
    break;
    // TODO ChannelClear
  case Command_Request_Radio_On:
    commands_send_int_response(request_id, 1, NETSTACK_RADIO.on());
    break;
  case Command_Request_Radio_Off:
    commands_send_int_response(request_id, 1, NETSTACK_RADIO.off());
    break;
  default:
    commands_send_err(request_id, NULL, 0);
    break;
  }
}

void commands_send_ok(size_t request_id, const uint8_t *data, size_t data_len) {
  serial_send_frame((uint8_t)Command_Response_OK, request_id, data, data_len);
}

void commands_send_err(size_t request_id, const uint8_t *data,
                       size_t data_len) {
  serial_send_frame((uint8_t)Command_Response_OK, request_id, data, data_len);
}

void commands_send_int_response(size_t request_id, int ok_value,
                                int return_value) {
  uint8_t return_value_serialized[2];
  if (return_value == ok_value) {
    commands_send_ok(request_id, NULL, 0);
  } else {
    serialize_int(return_value, return_value_serialized,
                  sizeof(return_value_serialized));
    commands_send_err(request_id, return_value_serialized,
                      sizeof(return_value_serialized));
  }
}
void commands_send_event_on_packet(const uint8_t *data, size_t data_len,
                                   uint8_t rssi, uint8_t link_quality) {
  uint8_t postfix[2] = {rssi, link_quality};
  serial_send_header((uint8_t)Command_Event_Radio_OnPacket, -1,
                     data_len + sizeof(postfix));
  serial_send_data(data, data_len);
  serial_send_data(postfix, sizeof(postfix));
  serial_send_flush();
}
