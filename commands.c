#include "commands.h"
#include "net/netstack.h"
#include "serial_protocol.h"
#include "serialization.h"

static void commands_handle_radio_get_value(size_t request_id,
                                            radio_param_t param) {
  radio_value_t value = 0;
  radio_result_t result;
  uint8_t serialized_response[4];
  result = NETSTACK_RADIO.get_value(param, &value);
  serialize_int(result, &serialized_response[0], 2);
  serialize_int(result, &serialized_response[2], 2);
  commands_send_ok(request_id, serialized_response,
                   sizeof(serialized_response));
}

static void commands_handle_radio_set_value(size_t request_id,
                                            const uint8_t *data,
                                            size_t data_len) {
  radio_param_t param;
  radio_value_t value;
  if (data_len < 4) {
    commands_send_err(request_id, NULL, 0);
    return;
  }
  param = deserialize_uint(&data[0], 2);
  value = deserialize_int(&data[2], 2);
  commands_send_ok_int(request_id, (int)NETSTACK_RADIO.set_value(param, value));
}

static void commands_handle_radio_get_object(size_t request_id,
                                             const uint8_t *data,
                                             size_t data_len) {
  radio_param_t param;
  static uint8_t retval[64];
  size_t expected_len;
  if (data_len < 4) {
    commands_send_err(request_id, NULL, 0);
    return;
  }
  param = deserialize_uint(&data[0], 2);
  expected_len = deserialize_uint(&data[2], 2);
  if (2 + expected_len > sizeof(retval)) {
    commands_send_err(request_id, NULL, 0);
    return;
  }
  serialize_int(
      (int)NETSTACK_RADIO.get_object(param, (void *)&retval[2], expected_len),
      &retval[0], 2);
  commands_send_ok(request_id, retval, 2 + expected_len);
}

static void commands_handle_radio_set_object(size_t request_id,
                                             const uint8_t *data,
                                             size_t data_len) {
  radio_param_t param;
  if (data_len < 2) {
    commands_send_err(request_id, NULL, 0);
    return;
  }
  param = deserialize_uint(data, data_len);
  commands_send_ok_int(request_id, (int)NETSTACK_RADIO.set_object(
                                       param, &data[2], data_len - 2));
}

void commands_handle_command(command_t cmd, size_t request_id,
                             const uint8_t *data, size_t data_len) {
  switch (cmd) {
  case Command_Request_Radio_Prepare:
    commands_send_ok_int(request_id, NETSTACK_RADIO.prepare(data, data_len));
    break;
  case Command_Request_Radio_Transmit:
    commands_send_ok_int(request_id, NETSTACK_RADIO.transmit(
                                         deserialize_ushort(data, data_len)));
    break;
  case Command_Request_Radio_Send:
    commands_send_ok_int(request_id, NETSTACK_RADIO.send(data, data_len));
    break;
  case Command_Request_Radio_ChannelClear:
    commands_send_ok_int(request_id, NETSTACK_RADIO.channel_clear());
    break;
  case Command_Request_Radio_On:
    commands_send_ok_int(request_id, NETSTACK_RADIO.on());
    break;
  case Command_Request_Radio_Off:
    commands_send_ok_int(request_id, NETSTACK_RADIO.off());
    break;
  case Command_Request_Radio_GetValue:
    commands_handle_radio_get_value(
        request_id, (radio_param_t)deserialize_uint(data, data_len));
    break;
  case Command_Request_Radio_SetValue:
    commands_handle_radio_set_value(request_id, data, data_len);
    break;
  case Command_Request_Radio_GetObject:
    commands_handle_radio_get_object(request_id, data, data_len);
    break;
  case Command_Request_Radio_SetObject:
    commands_handle_radio_set_object(request_id, data, data_len);
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

void commands_send_ok_int(size_t request_id, int data) {
  uint8_t data_serialized[2];
  serialize_int(data, data_serialized, sizeof(data_serialized));
  commands_send_ok(request_id, data_serialized, sizeof(data_serialized));
}

void commands_send_err_int(size_t request_id, int data) {
  uint8_t data_serialized[2];
  serialize_int(data, data_serialized, sizeof(data_serialized));
  commands_send_err(request_id, data_serialized, sizeof(data_serialized));
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
