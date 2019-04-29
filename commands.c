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

static int radio_check_filtering() {
  if ((FRMFILT0 & 1) != 1)
    return 1;
  if ((FRMFILT1 & 0x78) != 0x78)
    return 2;
  if ((SRCMATCH & 1) != 1)
    return 3;
  if ((SRCMATCH & 2) != 2)
    return 4;
  return 0;
}

static int radio_init_pending_table() {
  int retval;
  // AUTOPEND | SRC_MATCH_EN
  SRCMATCH |= 3;
  retval = radio_check_filtering();
  if (retval != 0)
    return retval;
  SRCSHORTEN0 = 0;
  SRCSHORTEN1 = 0;
  SRCSHORTEN2 = 0;
  SRCSHORTPENDEN0 = 0;
  SRCSHORTPENDEN1 = 0;
  SRCSHORTPENDEN2 = 0;
  SRCEXTEN0 = 0;
  SRCEXTEN1 = 0;
  SRCEXTEN2 = 0;
  SRCEXTPENDEN0 = 0;
  SRCEXTPENDEN1 = 0;
  SRCEXTPENDEN2 = 0;
  return 0;
}


static int radio_pend_set_ext(unsigned int index, const uint8_t *extaddr,
                       size_t extaddr_len) {
  uint8_t set, clear;
  unsigned int table_index;
  if (index >= 8)
    return 1;
  if (extaddr_len != 0 && extaddr_len != 8)
    return 2;
  set = (1 << index);
  clear = ~set;
  // Clear bits, before writing
  SRCEXTEN0 &= clear;
  SRCEXTPENDEN0 &= clear;
  // If we're unsetting, just stop now, no need to
  if (extaddr_len == 0) {
    return 0;
  }
  table_index = (index * 8);
  while (extaddr_len--) {
    (&SRC_ADDR_TABLE)[table_index++] = *(extaddr++);
  }
  // Now set them on the table again.
  SRCEXTPENDEN0 |= set;
  SRCEXTEN0 |= set;
  return 0;
}

static int radio_pend_set_short(unsigned int index, const uint8_t *shortaddr,
                         size_t shortaddr_len) {
  uint8_t set, clear;
  unsigned int table_index;
  if (index >= 8)
    return 1;
  if (shortaddr_len != 0 && shortaddr_len != 4)
    return 2;
  set = (1 << index);
  clear = ~set;
  // Clear bits, before writing
  SRCSHORTEN2 &= clear;
  SRCSHORTPENDEN2 &= clear;
  // If we're unsetting, just stop now, no need to
  if (shortaddr_len == 0) {
    return 0;
  }
  table_index = (8 * 8) + (index * 4);
  while (shortaddr_len--) {
    (&SRC_ADDR_TABLE)[table_index++] = *(shortaddr++);
  }
  // Now set them on the table again.
  SRCSHORTPENDEN2 |= set;
  SRCSHORTEN2 |= set;
  return 0;
}

static void commands_handle_radio_set_pending(size_t request_id, const uint8_t *data,
                                       size_t data_len) {
  uint8_t index;
  if (data_len < 1) {
    commands_send_err(request_id, NULL, 0);
    return;
  }
  index = data[0];
  if (index & 0x80) {
    commands_send_ok_int(
        request_id, radio_pend_set_ext(index & 0x7F, &data[1], data_len - 1));
  } else {
    commands_send_ok_int(
        request_id, radio_pend_set_short(index & 0x7F, &data[1], data_len - 1));
  }
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
  case Command_Request_Radio_InitPendingTable:
    commands_send_ok_int(request_id, radio_init_pending_table());
    break;
  case Command_Request_Radio_SetPending:
    commands_handle_radio_set_pending(request_id, data, data_len);
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
