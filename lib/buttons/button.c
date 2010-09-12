/* 
 * file: button.c
 * desc: a multi-thread button press handler
 * mail: xzpeter@gmail.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include "button.h"

static pthread_t button_pid = 0;
static int _button_thread_running = 0;
static int _button_thread_stop = 0;
static char buttons[MAX_SUPPORTED_BUTTONS] = {'0', '0', '0', '0', '0', '0'};
static button_handler btn_handler[MAX_SUPPORTED_BUTTONS];
static int buttons_fd;

static int _button_init(void)
{
	buttons_fd = open("/dev/buttons", 0);
	if (buttons_fd < 0) {
		pthread_exit(NULL);
	}
}

static void *_button_thread(void *data)
{
	puts("starting button detection thread.");
	_button_init();
	for (;;) {
		char current_buttons[MAX_SUPPORTED_BUTTONS];
		int count_of_changed_key;
		int i;
		if (read(buttons_fd, current_buttons, sizeof current_buttons) != sizeof current_buttons) {
			pthread_exit(NULL);
		}
		for (i = 0, count_of_changed_key = 0; i < sizeof buttons / sizeof buttons[0]; i++) {
			if (buttons[i] != current_buttons[i]) {
				printf("%d button before: %c, now: %c\n", i, buttons[i], current_buttons[i]);
				if (current_buttons[i] == '1') { /* just pressed */
					if (btn_handler[i])
						(btn_handler[i])();
// 					else
// 						printf("button %d pressed, but no handler.\n", i);
				}
				/* update button data */
				buttons[i] = current_buttons[i];
			}
		}
		if (_button_thread_stop) { /* main thread call for stop */
			break;
		}
		usleep(100000);
	}

	close(buttons_fd);
	_button_thread_running = 0;
	puts("button detection thread stopped.");
	return 0;
}

/* these are interface functions */

int button_thread_start(void)
{
	/* only accept one button thread working. */
	if (button_pid)
		return 0;
	if (pthread_create(&button_pid, NULL, _button_thread, NULL))
		return -1;
	pthread_detach(button_pid);
	_button_thread_running = 1;
	return 0;
}

int button_thread_stop(void)
{
	_button_thread_stop = 1;
	/* wait for stop */
	while (_button_thread_running) usleep(100000);
	return 0;
}

int register_button_handler(int no, button_handler func)
{
	if (no < 0 || no > MAX_SUPPORTED_BUTTONS)
		return -1;
	btn_handler[no] = func;
	return 0;
}

// void button_0_handler(void)
// {
// 	puts("button 0 pressed.");
// }
// 
// int main(void)
// {
// 	register_button_handler(0, button_0_handler);
// 	if (button_thread_start()) {
// 		puts("button thread start failed.");
// 		exit(0);
// 	}
// 	sleep(30);
// 	button_thread_stop();
// 	return 0;
// }
