/*
 * Copyright 2020, Collins Aerospace
 */

#include <camkes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <counter.h>
#include <data.h>
#include <queue.h>

#include <stdint.h>
#include <sys/types.h>

#include "hexdump.h"

// Forward declarations
void operating_region_out_event_data_send(data_t *data);


//------------------------------------------------------------------------------
// User specified input data receive handler for AADL Input Event Data Port (in) named
// "p1_in".
void operating_region_in_event_data_receive(counter_t numDropped, data_t *data) {
    printf("%s: received operating region: numDropped: %" PRIcounter "\n", get_instance_name(), numDropped); fflush(stdout);
    // hexdump("    ", 32, data->payload, sizeof(data->payload));
    operating_region_out_event_data_send(data);
}


recv_queue_t operatingRegionInRecvQueue;

// Assumption: only one thread is calling this and/or reading p1_in_recv_counter.
bool operating_region_in_event_data_poll(counter_t *numDropped, data_t *data) {
    return queue_dequeue(&operatingRegionInRecvQueue, numDropped, data);
}



void done_emit_underlying(void) WEAK;
static void done_emit(void) {
  /* If the interface is not connected, the 'underlying' function will
   * not exist.
   */
  if (done_emit_underlying) {
    done_emit_underlying();
  }
}


void operating_region_out_event_data_send(data_t *data) {
    queue_enqueue(operating_region_out_queue, data);
    operating_region_out_SendEvent_emit();
    done_emit();
}



void run_poll(void) {
    counter_t numDropped;
    data_t data;

    while (true) {

        bool dataReceived = operating_region_in_event_data_poll(&numDropped, &data);
        if (dataReceived) {
            operating_region_in_event_data_receive(numDropped, &data);
        }

        seL4_Yield();
    }

}



void post_init(void) {
    recv_queue_init(&operatingRegionInRecvQueue, operating_region_in_queue);
    queue_init(operating_region_out_queue);
}

int run(void) {

    run_poll();

}

