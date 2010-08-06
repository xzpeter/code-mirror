/*
 * file: modat_misc.c
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

int serial_open_port(char *portdevice, int speed)
{
	int portfd, mode;
	struct termios tty;

	mode = O_RDWR;

	portfd = open(portdevice, mode);

	if (portfd == -1) {
		dbg(1, "Can't Open Serial Port:%s\n", portdevice);
		return -1;
	}

	tcgetattr(portfd, &tty);

	//设置输入输出波特率
	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	//使用非规范方式
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE);

	//采用软件流控制，不使用硬件流控制
	tty.c_iflag |= (IXON | IXOFF | IXANY);
	tty.c_cflag &= ~CRTSCTS;

	/* 8N1,不使用奇偶校验 */

	//传输数据时使用8位数据位
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;

	//一位停止位,不使用奇偶校验
	tty.c_cflag &= ~(PARENB | CSTOPB);

	//忽略DCD信号，启用接收装置
	tty.c_cflag |= (CLOCAL | CREAD);

	/* Minimum number of characters to read */
	tty.c_cc[VMIN] = 0;
	/* Time to wait for data (tenths of seconds) */
	tty.c_cc[VTIME] = IO_WAIT_TIME;

	/* 使设置立即生效 */
	tcsetattr(portfd, TCSANOW, &tty);

	return portfd;
}
