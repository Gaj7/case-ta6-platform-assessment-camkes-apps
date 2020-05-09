/*
 * Copyright 2020, Collins Aerospace
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <camkes.h>
#include <sel4/sel4.h>
#include <counter.h>
#include <data.h>
#include <queue.h>

#include "hexdump.h"
#include "serial.h"
#include "sentinel_serial_buffer.h"


size_t compute_addr_attr_lmcp_message_size(void *buffer, size_t buffer_length)
{
  static const uint8_t addr_attr_delim = '$';
  static const uint8_t field_delim = '|';

  void *end_of_address_delim = memchr(buffer, addr_attr_delim, buffer_length);
  if (end_of_address_delim == NULL) {
    return 0;
  }
  ssize_t end_of_address_delim_offset = end_of_address_delim - buffer;
  
  void *end_of_attr_delim = memchr(end_of_address_delim, addr_attr_delim, buffer_length - end_of_address_delim_offset);
  if (end_of_attr_delim == NULL) {
    return 0;
  }
  ssize_t end_of_attr_delim_offset = end_of_attr_delim - buffer;
  
  void *end_of_message_delim = memchr(end_of_attr_delim, addr_attr_delim, buffer_length - end_of_attr_delim_offset);
  if (end_of_message_delim == NULL || end_of_message_delim == buffer + buffer_length - 1) {
    return 0;
  }
  
  const size_t lmcp_control_string_size = 4;
  const size_t checksum_size = 4;

  uint8_t *lmcp_message_size_pos = end_of_attr_delim + sizeof(addr_attr_delim) + lmcp_control_string_size;
  size_t lmcp_message_size = ((size_t) lmcp_message_size_pos[0] << 24)
    + ((size_t) lmcp_message_size_pos[1] << 16)
    + ((size_t) lmcp_message_size_pos[2] <<  8)
    + ((size_t) lmcp_message_size_pos[3] <<  0);
    
  if (buffer + buffer_length < (void *) lmcp_message_size_pos + lmcp_message_size + checksum_size) {
    return 0;
  }
    
  return ((size_t) ((void *) lmcp_message_size_pos - buffer)) + lmcp_control_string_size + lmcp_message_size + checksum_size;
}


// User specified input data receive handler for AADL Input Event Data Port (in) named
// "mission_command_in".
void mission_command_in_event_data_receive(counter_t numDropped, data_t *data) {
  printf("%s: received mission command: numDropped: %" PRIcounter "\n", get_instance_name(), numDropped);
}

//------------------------------------------------------------------------------
// Implementation of AADL Input Event Data Port (in) named "mission_command_in"
// Three styles: poll, wait and callback.
//
// Callback would typically be avoid for safety critical systems. It is harder
// to analyze since the callback handler is run on some arbitrary thread.
//
// NOTE: If we only need polling style receivers, we can get rid of SendEvent

recv_queue_t missionCommandInRecvQueue;

// Assumption: only one thread is calling this and/or reading mission_command_in_recv_counter.
bool mission_command_in_event_data_poll(counter_t *numDropped, data_t *data) {
  return queue_dequeue(&missionCommandInRecvQueue, numDropped, data);
}

// void mission_command_in_event_data_wait(counter_t *numDropped, data_t *data) {
//     while (!mission_command_in_event_data_poll(numDropped, data)) {
//         mission_command_in_SendEvent_wait();
//     }
// }

// static void mission_command_in_handler(void *v) {
//     counter_t numDropped;
//     data_t data;
// 
//     // Handle ALL events that have been queued up
//     while (mission_command_in_event_data_poll(&numDropped, &data)) {
//         mission_command_in_event_data_receive(numDropped, &data);
//     }
//     while (! mission_command_in_SendEvent_reg_callback(&mission_command_in_handler, NULL));
// }

//--

void done_emit_underlying(void) WEAK;
static void done_emit(void) {
  /* If the interface is not connected, the 'underlying' function will
   * not exist.
   */
  if (done_emit_underlying) {
    done_emit_underlying();
  }
}


//------------------------------------------------------------------------------
// Implementation of AADL Input Event Data Port (out) named "air_vehicle_state_out_1"
//
// NOTE: If we only need polling style receivers, we can get rid of the SendEvent

void air_vehicle_state_out_1_event_data_send(data_t *data) {
  queue_enqueue(air_vehicle_state_out_1_queue, data);
  air_vehicle_state_out_1_SendEvent_emit();
  done_emit();
}

//------------------------------------------------------------------------------
// Implementation of AADL Input Event Data Port (out) named "air_vehicle_state_out_2"
//
// NOTE: If we only need polling style receivers, we can get rid of the SendEvent

void air_vehicle_state_out_2_event_data_send(data_t *data) {
  queue_enqueue(air_vehicle_state_out_2_queue, data);
  air_vehicle_state_out_2_SendEvent_emit();
  done_emit();
}

//------------------------------------------------------------------------------
// Testing

void pre_init(void) {
  printf("%s: pre init apss\n", get_instance_name());
  serial_pre_init();
}

void post_init(void) {
  printf("%s: post init apss\n", get_instance_name());
  serial_post_init();
  queue_init(air_vehicle_state_out_1_queue);
  queue_init(air_vehicle_state_out_2_queue);
  recv_queue_init(&missionCommandInRecvQueue, mission_command_in_queue);
}

static const char message[] = {
#define AIR_VEHICLE_STATE_MESSAGE
#ifdef AIR_VEHICLE_STATE_MESSAGE
  0x61,0x66,0x72,0x6C,0x2E,0x63,0x6D,0x61,0x73,0x69,0x2E,0x41,0x69,0x72,0x56,0x65,
  0x68,0x69,0x63,0x6C,0x65,0x53,0x74,0x61,0x74,0x65,0x24,0x6C,0x6D,0x63,0x70,0x7C,
  0x61,0x66,0x72,0x6C,0x2E,0x63,0x6D,0x61,0x73,0x69,0x2E,0x41,0x69,0x72,0x56,0x65,
  0x68,0x69,0x63,0x6C,0x65,0x53,0x74,0x61,0x74,0x65,0x7C,0x54,0x63,0x70,0x42,0x72,
  0x69,0x64,0x67,0x65,0x7C,0x34,0x30,0x30,0x7C,0x36,0x38,0x24,0x4C,0x4D,0x43,0x50,
  0x00,0x00,0x01,0xCF,0x01,0x43,0x4D,0x41,0x53,0x49,0x00,0x00,0x00,0x00,0x00,0x00,
  0x0F,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x90,0x41,0xB0,0xD6,0x2D,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x43,0x0A,0xF8,0x22,0x00,0x00,0x00,0x00,0x41,0x9F,0xFF,0x72,0x37,
  0x18,0xF3,0x2C,0x41,0x66,0x0F,0x2B,0x42,0x1E,0x05,0xE2,0x43,0x0A,0xF8,0x22,0x41,
  0xB0,0xDD,0x90,0x01,0x43,0x4D,0x41,0x53,0x49,0x00,0x00,0x00,0x00,0x00,0x00,0x03,
  0x00,0x03,0x40,0x46,0xA8,0x88,0xD3,0xEE,0xDB,0xEA,0xC0,0x5E,0x3F,0x6B,0x3D,0xA6,
  0x0C,0x11,0x44,0x2F,0x00,0x00,0x00,0x00,0x00,0x01,0x42,0xC7,0xFF,0x2F,0x39,0x91,
  0xC0,0x87,0x00,0x02,0x01,0x43,0x4D,0x41,0x53,0x49,0x00,0x00,0x00,0x00,0x00,0x00,
  0x1B,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,
  0x01,0x00,0x00,0x00,0x00,0xC2,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x43,0x4D,
  0x41,0x53,0x49,0x00,0x00,0x00,0x00,0x00,0x00,0x15,0x00,0x03,0x00,0x00,0x00,0x00,
  0x00,0x00,0x27,0x11,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0xC2,0x70,
  0x00,0x00,0x00,0x00,0x00,0x00,0x42,0x34,0x00,0x00,0x42,0x07,0x00,0x00,0x00,0x04,
  0x01,0x43,0x4D,0x41,0x53,0x49,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x03,0x40,
  0x46,0xA8,0x4A,0xCA,0xB8,0x59,0xE5,0xC0,0x5E,0x3E,0x5C,0x82,0xF5,0xCD,0x81,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x43,0x4D,0x41,0x53,0x49,0x00,0x00,0x00,
  0x00,0x00,0x00,0x03,0x00,0x03,0x40,0x46,0xA7,0xDE,0xE2,0x2E,0x07,0x46,0xC0,0x5E,
  0x3F,0x24,0xB4,0xA1,0x86,0xCD,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x43,
  0x4D,0x41,0x53,0x49,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x03,0x40,0x46,0xA8,
  0x62,0x2A,0x06,0xD4,0x4E,0xC0,0x5E,0x3F,0x5A,0x99,0x2C,0xFD,0x21,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x01,0x01,0x43,0x4D,0x41,0x53,0x49,0x00,0x00,0x00,0x00,0x00,
  0x00,0x03,0x00,0x03,0x40,0x46,0xA8,0xD5,0xF9,0x47,0xF4,0xF9,0xC0,0x5E,0x3E,0xEB,
  0x32,0x6B,0x8C,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x43,0x4D,0x41,
  0x53,0x49,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x03,0x40,0x46,0xA8,0x5A,0x8D,
  0xC5,0x49,0xE7,0xC0,0x5E,0x3F,0x07,0xEC,0x63,0x92,0xC8,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x16,
  0x89,0x00,0x00,0x41,0xB0,0xD6,0x2D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x57,0xEC
#else
  0x61,0x66,0x72,0x6C,0x2E,0x63,0x6D,0x61,0x73,0x69,0x2E,0x4F,0x70,0x65,0x72,0x61,  /* afrl:cmasi:Opera */
  0x74,0x69,0x6E,0x67,0x52,0x65,0x67,0x69,0x6F,0x6E,0x24,0x6C,0x6D,0x63,0x70,0x7C,  /* tingRegion$lmcp| */
  0x61,0x66,0x72,0x6C,0x2E,0x63,0x6D,0x61,0x73,0x69,0x2E,0x4F,0x70,0x65,0x72,0x61,  /* afrl:cmasi:Opera */
  0x74,0x69,0x6E,0x67,0x52,0x65,0x67,0x69,0x6F,0x6E,0x7C,0x54,0x63,0x70,0x42,0x72,  /* tingRegion|6?p*r */
  0x69,0x64,0x67,0x65,0x7C,0x34,0x30,0x30,0x7C,0x36,0x38,0x24,0x4C,0x4D,0x43,0x50,  /* idge|400|68$LMCP */
  0x00,0x00,0x00,0x2B,0x01,0x43,0x4D,0x41,0x53,0x49,0x00,0x00,0x00,0x00,0x00,0x00,  /* ...+.CMASI...... */
  0x27,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x50,0x00,0x01,0x00,0x00,0x00,  /* '.........P..... */
  0x00,0x00,0x00,0x01,0x4E,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x4F,0x00,  /* ....N.........O. */
  0x00,0x03,0xE1                                                                    /* ...              */
#endif
};




int run(void) {

  counter_t numDropped;
  data_t data;

  // Busy loop to slow things down
  for (unsigned int j = 0; j < 20000000; ++j) {
    seL4_Yield();
  }


  // Test the sentinel serial buffer
  {
    const size_t max_message_length = 4096;
    bool result;
    ssize_t decode_length;
    int compare_result;
      
    sentinel_serial_buffer_t *ssb = sentinel_serial_buffer_alloc();
    uint8_t *decoded_message = calloc(max_message_length, sizeof(uint8_t));

    result = sentinel_serial_buffer_append_sentinelized_string(ssb, &message[0], sizeof(message));
    fprintf(stdout, "apss: ssb test append returned %d\n", (int) result);  fflush(stdout);

    result = sentinel_serial_buffer_append_sentinelized_string(ssb, &message[0], sizeof(message));
    fprintf(stdout, "apss: ssb test append 2 returned %d\n", (int) result); fflush(stdout);

    decode_length = sentinel_serial_buffer_get_next_payload_string(ssb, &decoded_message[0], max_message_length);
    if (decode_length > 0) {
      compare_result = strncmp((const char *) message, (const char *) decoded_message, sizeof(message));
      fprintf(stdout, "apss: ssb test decode returned %zd octets, compare returned %d\n",
	      decode_length, compare_result); fflush(stdout);
    } else {
      fprintf(stdout, "apss: ssb test decode returned %zd: %s\n",
	      decode_length, strerror(errno));
    }

    decode_length = sentinel_serial_buffer_get_next_payload_string(ssb, &decoded_message[0], max_message_length);
    if (decode_length > 0) {
      compare_result = strncmp((const char *) message, (const char *) decoded_message, sizeof(message));
      fprintf(stdout, "apss: ssb test decode 2 returned %zd octets, compare returned %d\n",
	      decode_length, compare_result); fflush(stdout);
    } else {
      fprintf(stdout, "apss: ssb test decode 2 returned %zd: %s\n",
	      decode_length, strerror(errno));
    }

    free(decoded_message);
    sentinel_serial_buffer_free(ssb);
  }
    
  while (1) {

    // Busy loop to slow things down
    for (unsigned int j = 0; j < 1000; ++j) {

      // Poll and handle mission command input port
      bool dataReceived = mission_command_in_event_data_poll(&numDropped, &data);

      if (dataReceived) {
	
	mission_command_in_event_data_receive(numDropped, &data);

	size_t message_size = compute_addr_attr_lmcp_message_size(&data.payload[0], sizeof(data.payload));

	if (message_size > 0) {
	  fprintf(stdout, "apss: received mission command message of %zu octets\n", message_size);  fflush(stdout);
	  hexdump("    ", 32, &data.payload[0], message_size);    
	  autopilot_serial_server_write_serial(&data.payload[0], message_size);
	}
      }

      // Poll and handle serial receive
      ssize_t received_size = autopilot_serial_server_read_serial((void *) &data.payload[0], sizeof(data.payload));

      if (received_size > 0) {
	fprintf(stdout, "apss: received serial message of %zu octets\n", received_size);  fflush(stdout);
	hexdump("    ", 32, &data.payload[0], received_size);    
	// TODO: Check that received data is an Air Vehicle State message and discard others
	air_vehicle_state_out_1_event_data_send(&data);
	air_vehicle_state_out_2_event_data_send(&data);
      }

      seL4_Yield();

      // Busy loop to slow things down
      for (unsigned int n = 0; n < 1000; ++n) {
	seL4_Yield();
      }

    }

    // Stage data
    memcpy((void *) &data.payload[0], (const void *) &message[0], sizeof(message));
    printf("%s: sending: %d\n", get_instance_name(), sizeof(message));

    // Send the data
    air_vehicle_state_out_1_event_data_send(&data);
    air_vehicle_state_out_2_event_data_send(&data);
    autopilot_serial_server_write_serial(&data.payload[0], sizeof(message));
  }
}

