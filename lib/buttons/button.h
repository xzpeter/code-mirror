/*
 * file: button.h
 * desc: a multi-thread button press handler
 * mail: xzpeter@gmail.com
 */
#ifndef __BUTTON_H__
#define __BUTTON_H__

#define  MAX_SUPPORTED_BUTTONS  6

typedef void (*button_handler)(void);

/* we should register handlers before start the button thread */
int register_button_handler(int no, button_handler func);
int button_thread_start(void);
/* we should always close button thread after use. */
int button_thread_stop(void);

#endif
