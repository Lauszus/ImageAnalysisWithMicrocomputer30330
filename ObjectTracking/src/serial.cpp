// Based on: http://stackoverflow.com/a/6947758

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "serial.h"

static int set_interface_attribs(int fd, int speed, int parity);
static int set_blocking(int fd, int should_block);

int fd;

int serialOpen(const char *portname, speed_t speed) {
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY); // Added "O_NDELAY"
    if (fd < 0) {
        printf("error %d opening %s: %s", errno, portname, strerror (errno));
        return 1;
    }
    if (set_interface_attribs(fd, speed, 0) < 0) // Set speed and 8N1 (no parity)
        return 1;
    if (set_blocking(fd, 0) < 0) // Set no blocking
        return 1;
    return 0;
}

int serialClose(const char *portname) {
    if (close(fd) < 0) {
        printf("error %d closing %s: %s", errno, portname, strerror (errno));
        return 1;
    }
    return 0;
}

void serialWrite(const char *string) {
    write(fd, string, strlen(string));
}

static int set_interface_attribs(int fd, int speed, int parity) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        printf("error %d from tcgetattr", errno);
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN] = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD); // ignore modem controls,
                                     // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("error %d from tcsetattr", errno);
        return -1;
    }
    return 0;
}

static int set_blocking(int fd, int should_block) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        printf("error %d from tggetattr", errno);
        return -1;
    }

    tty.c_cc[VMIN] = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("error %d setting term attributes", errno);
        return -1;
    }
    return 0;
}
