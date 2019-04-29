#ifndef _COMMANDS_H_
#define _COMMANDS_H_
#include <stdint.h>
#include <string.h>

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
  Command_Request_Radio_InitPendingTable,
  Command_Request_Radio_SetPending,
  /**
   * These are sent from Dongle -> PC
   * Err is usually a protocol error, while OK may still contain a non-OK return
   * value.
   */
  Command_Response_OK = 0x80,
  Command_Response_Err,
  Command_Event_Radio_OnPacket = 0xC0
} command_t;

void commands_handle_command(command_t cmd, size_t request_id,
                             const uint8_t *data, size_t data_len);
void commands_send_ok(size_t request_id, const uint8_t *data, size_t data_len);
void commands_send_ok_int(size_t request_id, int data);
void commands_send_err(size_t request_id, const uint8_t *data, size_t data_len);
void commands_send_err_int(size_t request_id, int data);
void commands_send_event_on_packet(const uint8_t *packet, size_t packet_len,
                                   uint8_t rssi, uint8_t link_quality);
#endif // _COMMANDS_H_

