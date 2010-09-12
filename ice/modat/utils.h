/*
 * file: utils.c
 * desc: misc functions used in modat
 * mail: xzpeter@gmail.com
*/
#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/time.h>

typedef enum _crc_mode {
	CRC_NONE, 
	CRC_ODD,
	CRC_EVEN,
} crc_mode;

extern void make_date(char *ct);
extern void make_time(char *ct);
extern long get_elapsed_sec(struct timeval old_time);
extern long get_elapsed_msec(struct timeval old_time);
extern int check_thres_passed(struct timeval old_time, long threshold);
extern void reset_timer(struct timeval *tv);
extern int serial_open_port(char *portdevice, int speed, crc_mode crc);
extern char *dump_hex(char *buf, int len);

#endif
