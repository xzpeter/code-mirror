/*
 * file: ads321.c
 * desc: ADS321 module related codes
 * mail: xzpeter@gmail.com
 */

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "modat/utils.h"
#include "ads321.h"
#include "config.h"
#include "dbg.h"

static int _fd = 0;

extern int start_sensor_index, end_sensor_index, raw_data_mode;

#define  AD321_RECV_BUF_LEN  1024
#define  AD321_CMD_LEN_MAX  128
/* send ads321 commands 
 * cmd: the cmd to send
 * rest: least chars to recv
 * return: the result. */
static char *_cmd(char *cmd, int rest)
{
	static char _recv_buf[AD321_RECV_BUF_LEN];
	int len, timeout_count;

	if (!_fd || !cmd || rest < 0) {
		dbg(1, "check args failed.");
		return NULL;
	}
	len = strlen(cmd);
	if (len <= 0) {
		dbg(1, "ADS321 cmd too long(len=%d).", len);
		return NULL;
	}
	/* send the command */
	if (write(_fd, cmd, len) != len) {
		dbg(1, "ADS321 write error.");
		return NULL;
	}
	dbg(2, "-->len: %d, buf: %s", len, cmd);
	usleep(100000);

	/* do the recv */
	len = 0; /* count the received char */
	timeout_count = 0; /* clear time out counter */
	do {
		int ret;
		ret = read(_fd, _recv_buf+len, sizeof(_recv_buf)-len);
		if (ret <= 0) { /* here, -1 may not mean error */
			dbg(1, "read() timeout.");
			if (++timeout_count >= AD321_TIMEOUT_MAX_N) {
				dbg(1, "timeout %d times, return false.", timeout_count);
				return NULL;
			}
		}
		len += ret;
	} while (rest > len);

	_recv_buf[len] = 0x00;
	dbg(2, "<--len: %d, buf: %s", len, dump_hex(_recv_buf, len));
	return _recv_buf;
}

int _ads321_init_port(char *port)
{
	if (_fd)
		return _fd;
	return serial_open_port(port, ADS321_BAUD_RATE, ADS321_CRC_MODE);
}

/* check if the sensor with address=dev_no is alive */
int _check_alive(int dev_no)
{
	char *recv_buf, buf[256];
	int n;

	snprintf(buf, 256, "s%02d;adr?;", dev_no);
	recv_buf = _cmd(buf, 4);
	if (recv_buf == NULL)
		return -1;
	if (!sscanf(recv_buf, "%d", &n))
		return -2;
	if (n != dev_no)
		return -3;

	return 0;
}

int _ads321_init_params(void)
{
	int i, ret;
	/* no init param, instead, check alive for all the sensors */
	for (i = start_sensor_index; i < end_sensor_index; i++) {
		if ((ret = _check_alive(i))) {
			dbg(1, "sensor address=%02d is not alive, ret=%d.", i, ret);
			return -1;
		}
	}
	return 0;
}

int ads321_init(char *port)
{
	int ret;
	/* init port */
	if ((_fd = _ads321_init_port(port)) <= 0) {
		dbg(1, "_ads321_init_port() at %s failed. ret=%d.", port, _fd);
		_fd = 0;
		return -1;
	}
	dbg(4, "init port done.");
	if ((ret = _ads321_init_params())) {
		dbg(1, "_ads321_init_params() failed. ret=%d.", ret);
		return -2;
	}
	return 0;
}

// static void _switch_char(char *a, char *b)
// {
// 	char c;
// 	c = *a;
// 	*a = *b;
// 	*b = c;
// }
// 
// static void _switch_int(char *buf)
// {
// 	if (!buf)
// 		return;
// 	_switch_char(buf, buf+3);
// 	_switch_char(buf+1, buf+2);
// }

double _weight_compensate(int w)
{
	return (double)(w/10.0);
}

/* compensate for temperature error */
double _temp_compensate(double tmp)
{
	return (tmp - 2.5);
}

/* read weight from device */
int ads321_get_weight(int dev_no, double *weight)
{
	char cmd[128];
	char *buf;
	if (!_fd || dev_no < start_sensor_index || dev_no >= end_sensor_index || !weight) {
		dbg(1, "check args failed.");
		return -1;
	}
	snprintf(cmd, 128, "s%02d;msv?;", dev_no);
	if (!(buf = _cmd(cmd,6))) {
		dbg(1, "weight cmd failed.");
		return -2;
	}
	ads_int data;
	/* read data into bits: 31~8 */
	data.c[3] = buf[0];
	data.c[2] = buf[1];
	data.c[1] = buf[2];
	data.c[0] = 0x00;
	/* do the right shift */
	data.i = data.i >> 8;
	/* data translation */
	if (raw_data_mode)
		*weight = data.i;
	else
		*weight = _weight_compensate(data.i);
	return 0;
}

int ads321_get_temp(int dev_no, double *temp)
{
	char cmd[128];
	char *buf;
	if (!_fd || dev_no < start_sensor_index || dev_no >= end_sensor_index || !temp) {
		dbg(1, "check args failed.");
		return -1;
	}
	snprintf(cmd, 128, "s%02d;tep?;", dev_no);
	if (!(buf = _cmd(cmd,10))) {
		dbg(1, "weight cmd failed.");
		return -2;
	}
	if (!sscanf(buf, "%lf", temp)) {
		dbg(1, "cannot convert to double.");
		return -3;
	}
	if (!raw_data_mode)
		*temp = _temp_compensate(*temp);
	return 0;
}
