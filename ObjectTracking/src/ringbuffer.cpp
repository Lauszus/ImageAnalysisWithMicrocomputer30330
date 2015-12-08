// Inspired by: https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/cores/arduino/HardwareSerial.cpp and https://github.com/arduino/Arduino/blob/master/hardware/arduino/sam/cores/arduino/RingBuffer.cpp

#include <stdint.h>
#include <string.h>

#include "ringbuffer.h"

RingBuffer::RingBuffer(void) {
    memset(buffer, 0, BUFFER_SIZE);
    head = 0;
    tail = 0;
}

void RingBuffer::write(int c) {
    int i = (uint32_t)(head + 1) % BUFFER_SIZE ;

    // If we should be storing the received character into the location
    // just before the tail (meaning that the head would advance to the
    // current location of the tail), we're about to overflow the buffer
    // and so we don't write the character or advance the head.
    if (i != tail) {
        buffer[head] = c;
        head = i;
    }
}

void RingBuffer::clear(void) {
    head = tail;
}

int RingBuffer::available(void) {
    return ((uint32_t)(BUFFER_SIZE + head - tail)) % BUFFER_SIZE;
}

int RingBuffer::read(void) {
    if (head == tail) // If the head isn't ahead of the tail, we don't have any characters
        return -1;
    else {
        int c = buffer[tail];
        tail = (uint32_t)(tail + 1) % BUFFER_SIZE;
        return c;
    }
}
