/*
 * file: modat_misc.c
 * desc: misc functions used in modat
 * mail: xzpeter@gmail.com
*/
#ifndef __MODAT_MISC_H__
#define __MODAT_MISC_H__

#include <sys/time.h>

extern long get_elapsed_sec(struct timeval old_time);
int serial_open_port(char *portdevice, int speed);

#endif
