#ifndef _APPLICATION_H
#define _APPLICATION_H

#ifndef VERSION
#define VERSION "vdev"
#endif

#include <bcl.h>

void lcdBufferString(char *str, int x, int y);
void lcdBufferNumber(int number, int x, int y);

#endif // _APPLICATION_H
