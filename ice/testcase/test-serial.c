#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "../modat/utils.h"

pthread_t pid;
int fd;

void usage(void)
{
	printf("usage: smini /dev/ttyUSB0 115200.\n");
	exit(0);
}

struct termios old_term, new_term;

void recover(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
}

void sig_int_handle(int flag)
{
	puts("SIGINT detected. quitting.");
	recover();
	exit(0);
}

void sig_term_handle(int flag)
{
	puts("SIGTERM detected. quitting.");
	recover();
	exit(0); 
}

void handle_signal(void)
{
	signal(SIGINT, sig_int_handle);
	signal(SIGTERM, sig_term_handle);
}

void set_input_mode(void)
{
	tcgetattr(STDIN_FILENO, &old_term);
	memcpy(&new_term, &old_term, sizeof(struct termios));
	new_term.c_cc[VMIN] = 1;
	new_term.c_cc[VTIME] = 0;
	new_term.c_lflag &= ~ICANON;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
}

void *handle_recv(void *data)
{
	int ret;
	char recv_buf[512];
	while (1) {
		ret = read(fd, recv_buf, 512);
		if (ret == 0)
			continue;
		/* here, ret=-1 may not an error, errstr is "resource temporarily unavaliable " */
		if (ret == -1) {
// 			printf("ret=-1, errno=%d, errstr=%s\n", errno, strerror(errno));
			continue;
		}
		recv_buf[ret] = 0x00;
		printf("\n--asc-->%s\n", recv_buf);
		printf("--hex-->%s\n", dump_hex(recv_buf, ret));
	}
	/* shouldn't reach here */
	printf("read() returned -1, quitting recving thread.\n");
	exit(0);
	return NULL;
}

int main(int argc, char *argv[])
{
	int len;
	char buf[1];
	char *file = argv[1];
	int speed = atoi(argv[2]);

	if (argc != 3)
		usage();

	fd = serial_open_port(file, speed, CRC_NONE);
	if (fd == -1) {
		printf("open port failed.\n");
		usage();
	}
	puts("open port good.");

	if (pthread_create(&pid, NULL, handle_recv, NULL)) {
		printf("pthread_create() failed.\n");
		usage();
	}
	pthread_detach(pid);
	puts("thread created.");

	handle_signal();
	set_input_mode();

	puts("start to listening...");

	while (1) {
		len = read(STDIN_FILENO, buf, 1);
		if (len == 0 || len == -1)
			continue;
		write(fd, buf, 1);
	}

	puts("read() returned -1, quitting.");
	recover();
	exit(0);
	
	return 0;
}
