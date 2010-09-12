/*
 * file: utils.c
 * desc: misc functions used in modat
 * mail: xzpeter@gmail.com
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "modat_include.h"

#define  IO_WAIT_TIME  5

/* function to make date string */
void make_date(char *ct)
{
	struct tm tm_s;
	time_t tt = time(NULL);
	localtime_r(&tt, &tm_s);
	sprintf(ct, "%04d%02d%02d", 
			tm_s.tm_year+1900,
			tm_s.tm_mon+1,
			tm_s.tm_mday
			);
}

/* function to make time string */
void make_time(char *ct)
{
	struct tm tm_s;
	time_t tt = time(NULL);
	localtime_r(&tt, &tm_s);
	sprintf(ct, "%02d%02d%02d", 
			tm_s.tm_hour,
			tm_s.tm_min,
			tm_s.tm_sec
			);
}

/* get_elapsed_seconds */
long get_elapsed_sec(struct timeval old_time)
{
	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);
	if ((cur_time.tv_usec -= old_time.tv_usec) < 0) {
		--cur_time.tv_sec;
		cur_time.tv_usec += 1000000;
	}
	cur_time.tv_sec -= old_time.tv_sec;

	return (long)cur_time.tv_sec;
}

/* get_elapsed_mseconds */
long get_elapsed_msec(struct timeval old_time)
{
	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);
	if ((cur_time.tv_usec -= old_time.tv_usec) < 0) {
		--cur_time.tv_sec;
		cur_time.tv_usec += 1000000;
	}
	cur_time.tv_sec -= old_time.tv_sec;

	return (long)(cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000);
}

/* check_if_threshold_passed */
int check_thres_passed(struct timeval old_time, long threshold)
{
	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);
	if ((cur_time.tv_usec -= old_time.tv_usec) < 0) {
		--cur_time.tv_sec;
		cur_time.tv_usec += 1000000;
	}
	cur_time.tv_sec -= old_time.tv_sec;

	return (cur_time.tv_sec >= threshold) ? 1 : 0;
}

/* reset_one_timer */
void reset_timer(struct timeval *tv)
{
	gettimeofday(tv, NULL);
}

#define  SPD_DEFAULT  (B115200)
static int _trans_speed(int speed)
{
	if (speed == 4800) {return B4800;}
	if (speed == 9600) {return B9600;}
	if (speed == 19200) {return B19200;}
	if (speed == 38400) {return B38400;}
	if (speed == 57600) {return B57600;}
	if (speed == 115200) {return B115200;}
	return SPD_DEFAULT;
}

int serial_open_port(char *portdevice, int speed, crc_mode crc)
{
	int portfd;
	struct termios tty;

	// xzpeter moded
	portfd = open(portdevice, O_RDWR | O_NOCTTY | O_NDELAY);
	if (portfd == -1)
		return -1;

	tty.c_cflag = _trans_speed(speed);
	tty.c_cflag |= CS8;
	if (crc == CRC_ODD) /* odd crc check */
		tty.c_cflag |= (PARENB | PARODD);
	else if (crc == CRC_EVEN) { /* even crc check */
		tty.c_cflag |= PARENB; 
		tty.c_cflag &= ~PARODD; 
	} else /* no crc chec */
		tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
	// xzpeter moded
	tty.c_cflag |= CREAD;
	tty.c_cflag |= CLOCAL;
	tty.c_oflag = 0;
	tty.c_lflag = 0; /* !ICANON */
	tty.c_cc[VTIME] = 5;
	tty.c_cc[VMIN] = 0;
	// xzpeter moded
	tty.c_iflag = IGNPAR | IGNBRK;

	tcsetattr(portfd, TCSANOW, &tty);
	tcflush(portfd, TCIOFLUSH);

#if 0

	mode = O_RDWR;

	portfd = open(portdevice, mode);

	if (portfd == -1) {
		return -1;
	}

	tcgetattr(portfd, &tty);

	//设置输入输出波特率
	cfsetospeed(&tty, _trans_speed(speed));
	cfsetispeed(&tty, _trans_speed(speed));

	//使用非规范方式
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE);

	//采用软件流控制，不使用硬件流控制
	tty.c_iflag |= (IXON | IXOFF | IXANY);
	tty.c_cflag &= ~CRTSCTS;

	/* 8N1,不使用奇偶校验 */

	//传输数据时使用8位数据位
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;

	//一位停止位
	tty.c_cflag &= ~CSTOPB;

	if (crc == CRC_ODD) /* odd crc check */
		tty.c_cflag |= (PARENB | PARODD);
	else if (crc == CRC_EVEN) { /* even crc check */
		tty.c_cflag |= PARENB; 
		tty.c_cflag &= ~PARODD; 
	} else /* no crc chec */
		tty.c_cflag &= ~PARENB;

	//忽略DCD信号，启用接收装置
	tty.c_cflag |= (CLOCAL | CREAD);

	/* Minimum number of characters to read */
	tty.c_cc[VMIN] = 0;
	/* Time to wait for data (tenths of seconds) */
	tty.c_cc[VTIME] = IO_WAIT_TIME;

	/* 使设置立即生效 */
	tcsetattr(portfd, TCSANOW, &tty);
#endif

	return portfd;
}

char *dump_hex(char *buf, int len)
{
	int i;
	static char str[2048];
	if (len >= 2048/3) {
		return NULL;
	}
	for (i = 0; i < len; i++) {
		sprintf(str+i*3, "%02x ", (unsigned char)buf[i]);
	}
	return str;
}

char *check_directory(char *dir)
{
	int len = strlen(dir);
	if (dir[len-1] != '/')
		dir[len] = '/';
	return dir;
}
