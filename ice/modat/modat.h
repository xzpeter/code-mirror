/*
 * file: modat.h
 * desc: a simple c lib for AT module
 * mail: xzpeter@gmail.com
 */

#ifndef		__MODAT_H__
#define		__MODAT_H__

#include <pthread.h>
#include <time.h>
#include "modat_config.h"
#include "rbuf.h"

typedef struct _modat MODAT;

/* supported types of modem */
#define SUPPORTED_MODEMS_N  1
typedef enum _modat_type {
	UNKNOWN = 0,
	T35i = 1,
	TYPE_ANY = 255,
} MODAT_TYPE;

typedef enum _modat_status {
	INIT = 1,
	STARTUP = 2,
	READY = 3,
	WORKING = 4,
	SICK = 80,		/* status >= SICK is not right ... */
	NOT_RESPONDING = 96,	/* at cmd timeout too many time! */
	UNTACHED = 97,		/* device file is missing */
	WAIT_USERS = 98,	/* waiting for other working thread to release
				   resource of the device */
	DEAD = 99,		/* all the resources are released and the device is
				   waiting to die */
} MODAT_STATUS;

typedef enum _at_status {
	AT_READY,		/* normal stat, after host picked the results, HOST writable */
	AT_REQUEST,		/* after the host sends a request, DAEMON writable */
	AT_SENT,		/* after the daemon sent the cmd, DAEMON writable */
	AT_RECVED,		/* after the daemon recved the response, HOST writable */
} AT_STATUS;

typedef enum _at_return {
	AT_NOT_RET = -1,	/* not returned */
	AT_OK = 0,		/* return "OK" */
	/* CAUTION! if we got AT_RAWDATA returned, don't forget a 'Ctrl^z' then. */
	AT_RAWDATA = 1,		/* return ">", waiting for SMS input. */
	AT_ERROR = 2,		/* return "ERROR" */
	AT_CME_ERROR = 3,	/* return "AT_CME_ERROR" */
	AT_CMS_ERROR = 4,	/* return "AT_CMS_ERROR" */
	AT_HWERROR = 5,		/* hardware error */
	AT_NO_CARRIER = 6,	/* 'no carrier' error */
} AT_RETURN;

#define		MODAT_SEND_BUFLEN	256
#define		MODAT_RECV_BUFLEN	2048
#define		MODAT_KEYWORD_LEN	32

/*	define of at.mode:
	bit index : meanings
	0 : turn on 'recording' mode if 1, turn off if 0
	1 : block recording if 1, line record if 0
*/
/* use LINE mode, will return only special keywords */
#define		AT_MODE_LINE		0x00
/* use BLOCK mode, will return all the at response after the cmd sent,
   and before OK(or ERROR) is returned */
#define		AT_MODE_BLOCK		0x01

typedef struct _modat_at_buf {
	char send_buf[MODAT_SEND_BUFLEN];
	char recv_buf[MODAT_RECV_BUFLEN];
	char keyword[MODAT_KEYWORD_LEN];
	int mode;
	AT_STATUS status;
	AT_RETURN ret;
	/* adding sent timestamp */
	time_t sent_time;
	int time_out_count;
	volatile int start_copy;	/* put the line into at_recv_buf only if start_copy=1 */
} MODAT_AT_BUF;

#define		MODAT_NAME_LEN		256
#define		DEV_FILE_NAME_LEN	64
#define		GLOBAL_BUF_LEN		1024
#define		MODAT_MAX_MOBILE_ID_LEN		(MAX_MOBILE_ID_LEN)

#define 	MODAT_METHODS \
	int (*open_port)(MODAT *);			/* open comm port, e.g. serial */\
	void (*close_port)(MODAT *);			/* close comm port, e.g. serial */\
	int (*module_startup)(MODAT *);		/* do the start up work, not using at_buf */\
	AT_RETURN (*send)(MODAT *, char *, char *, int);				/* use this to send a at cmd (buffered) */\
	int (*send_sms)(MODAT *, char *, char *);			/* send a SMS using send() */\
	int (*parse_line)(MODAT *, char *);	/* modem specified parsing line function */\
	int (*start_call)(MODAT *p, char *);		/* start a call */\
	int (*stop_call)(MODAT *p);		/* stop a call */\
	int (*network_status)(MODAT *p, char *buf);	/* check recent network status */\
	int (*probe)(MODAT *p);	/* probe the modem */\
	int (*set_engineer_mode)(MODAT *p, int on);  /* open engineer mode */\
	int (*ppp_open)(MODAT *p, int conn_type);  /* ppp dial */\
	int (*ppp_close)(MODAT *p);	/* close ppp connection */\
	int (*recv_sms)(MODAT *p, int pos, char *peer, char *data, int size);	/* recv sms */

/* procedures to add a MODAT function:
 * 1. add function into MODAT struct;
 * 2. add function into DEV_MODEL struct;
 * 3. add function into dev_model[i] in each model, which is in dm-modat.c;
 * 4. add function assignments in modat_register_methods(). */

struct _modat {

	/* modat data structures */
	RBUF *r;		/* a simple round buffer */
	MODAT_AT_BUF at;	/* at cmd buffer */
	char name[MODAT_NAME_LEN];	/* device (file) name */
	char dev_file[DEV_FILE_NAME_LEN];	/* e.g. "ttyUSB0" */
	MODAT_TYPE type;	/* decide which driver to use */
	unsigned int users_count;	/* count of users who are using the mod */
	int enabled;		/* if the device is activated */
	pthread_t thread_id;	/* thread id of the listening thread */
	MODAT_STATUS status;	/* INIT, READY, WORKING, DEAD */
	pthread_mutex_t mutex;	/* mutex of the modat struct */
	int portfd;		/* file handler of the serial port */
	unsigned int speed;	/* speed of the port */
	char sim[MODAT_MAX_MOBILE_ID_LEN];	/* simcard number */
	volatile int engineer_mode;	/* if engineer mode on */
	volatile int start_up_status;	/* if the startup thread is alive */

	/* modat methods definations */
 MODAT_METHODS};

#define		PROD_NAME_LEN		256
/* supports model for modat devices */
typedef struct _modat_support {
	char product_name[PROD_NAME_LEN];
	/* specialized functions */
	int (*check_device_file) (char *file);
	/* common functions to map to MODAT struct */
 MODAT_METHODS} MODAT_SUPPORT;

/* host related functions */
extern int check_at_recved(MODAT * p);
extern void modat_recved_done(MODAT * p);
extern int check_at_ready(MODAT * p);
extern void modat_send_it(MODAT * p);

/* other modat related API functions */

MODAT *modat_init(char *dev_file, int speed);
extern void modat_release(MODAT * p_modat);
extern void modat_set_enable(MODAT * p_modat);
extern int modat_get_enable(MODAT * p_modat);
extern void modat_set_disable(MODAT * p_modat);
extern void modat_set_status(MODAT * p_modat, MODAT_STATUS status);
extern MODAT_STATUS modat_get_status(MODAT * p_modat);
extern int modat_lock(MODAT * p);
extern int modat_unlock(MODAT * p);
extern int modat_ready(MODAT * p);
extern int _modat_is_sick(MODAT * p);
extern int modat_get_engineer_mode(MODAT * p);
extern int modat_set_engineer_mode(MODAT * p, int mode);

#endif
