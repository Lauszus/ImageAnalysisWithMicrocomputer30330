/* Copyright (C) 2015 Kristian Sloth Lauszus. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Sloth Lauszus
 Web      :  http://www.lauszus.com
 e-mail   :  lauszus@gmail.com
*/

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
