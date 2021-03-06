/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * Copyright 2019 Adventium Labs
 * Modifications made to original
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_Adventium_BSD)
 */

#ifdef __cplusplus
#extern "C" {
#endif

#include <am_queue.h>
#include <stdint.h>
#include <stddef.h>

//------------------------------------------------------------------------------
// Sender API
//
// See queue.h for API documentation. Only implementation details are documented here.

void am_queue_init(am_queue_t *queue) {
  // NOOP for now. C's struct initialization is sufficient.  If we ever do need
  // initialization logic, we may also need to synchronize with receiver
  // startup.
}

void am_queue_enqueue(am_queue_t *queue, am_data_t *data) {
  // Simple ring with one dirty element that will be written next. Only one
  // writer, so no need for any synchronization. elt[queue->numSent %
  // QUEUE_SIZE] is always considered dirty. So do not advance queue-NumSent
  // till AFTER data is copied.

  size_t i = queue->numSent % AM_QUEUE_SIZE;
  queue->elt[i] = *data; // Copy data into queue
  // Release memory fence - ensure that data write above completes BEFORE we advance queue->numSent
  __atomic_thread_fence(__ATOMIC_RELEASE);
  ++(queue->numSent);
}

//------------------------------------------------------------------------------
// Receiver API
//
// See queue.h for API documentation. Only implementation details are documented here.

void am_recv_queue_init(am_recv_queue_t *recvQueue, am_queue_t *queue) {
  recvQueue->numRecv = 0;
  recvQueue->queue = queue;
}

bool am_queue_dequeue(am_recv_queue_t *recvQueue, am_counter_t *numDropped, am_data_t *data) {
  am_counter_t *numRecv = &recvQueue->numRecv;
  am_queue_t *queue = recvQueue->queue;
  // Get a copy of numSent so we can see if it changes durring read
  am_counter_t numSent = queue->numSent;
  // Acquire memory fence - ensure read of queue->numSent BEFORE reading data
  __atomic_thread_fence(__ATOMIC_ACQUIRE);
  // How many new elements have been sent? Since we are using unsigned
  // integers, this correctly computes the value as counters wrap.
  am_counter_t numNew = numSent - *numRecv;
  if (0 == numNew) {
    // Queue is empty
    return false;
  }
  // One element in the ring buffer is always considered dirty. Its the next
  // element we will write.  It's not safe to read it until numSent has been
  // incremented. Thus there are really only (QUEUE_SIZE - 1) elements in the
  // queue.
  *numDropped = (numNew <= AM_QUEUE_SIZE - 1) ? 0 : numNew - AM_QUEUE_SIZE + 1;
  // Increment numRecv by *numDropped plus one for the element we are about to
  // read.
  *numRecv += *numDropped + 1;
  am_counter_t numRemaining = numSent - *numRecv;
  size_t i = (*numRecv - 1) % AM_QUEUE_SIZE;
  *data = queue->elt[i]; // Copy data
  // Acquire memory fence - ensure read of data BEFORE reading queue->numSent again
  __atomic_thread_fence(__ATOMIC_ACQUIRE);
  if (queue->numSent - *numRecv + 1 < AM_QUEUE_SIZE) {
    // Sender did not write element we were reading. Copied data is coherent.
    return true;
  } else {
    // Sender may have written element we were reading. Copied data may be incoherent.
    // We dropped the element we were trying to read, so increment *numDropped.
    ++(*numDropped);
    return false;
  }
}

bool am_queue_is_empty(am_recv_queue_t *recvQueue) {
  return (recvQueue->queue->numSent == recvQueue->numRecv);
}

#ifdef __cplusplus
}
#endif
