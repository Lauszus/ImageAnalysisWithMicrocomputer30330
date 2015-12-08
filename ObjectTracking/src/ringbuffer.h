#ifndef __ringbuffer_h__
#define __ringbuffer_h__

#define BUFFER_SIZE 128

class RingBuffer {
public:
    RingBuffer(void) ;
    void write(int c);
    void clear(void);
    int available(void);
    int read(void);

private:
    int buffer[BUFFER_SIZE];
    int head;
    int tail;
};

#endif
