#ifndef __serial_h__
#define __serial_h__

#include <termios.h>

int serialOpen(const char *portname, speed_t speed);
int serialClose(const char *portname);
void serialWrite(const char *string);

#endif
