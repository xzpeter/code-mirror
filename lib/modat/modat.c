/*
 * file: modat.c
 * mail: xzpeter@gmail.com
 * desc: a simple c lib for MODAT module
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <termio.h>
#include "modat_include.h"

/**********************************
 * MODAT functions 
 *********************************/

#define MAX_USERS_PER_MODEM  10

const MODAT_SUPPORT modat_supports[SUPPORTED_MODEMS_N + 1] = {
	{
	 .product_name = "UNKNOWN",
	 .open_port = common_open_port,
	 .close_port = common_close_port,
	 .check_device_file = common_check_device_file,
	 .module_startup = common_module_startup,
	 .send = common_send,
	 .send_sms = common_send_sms,
	 .parse_line = common_parse_line,
	 .start_call = common_start_call,
	 .stop_call = common_stop_call,
	 .network_status = common_network_status,
	 .probe = common_probe,
	 .set_engineer_mode = common_set_engineer_mode,
	 .recv_sms = common_recv_sms,
	 },
	{
	 .product_name = "TC35i",
	 .open_port = common_open_port,
	 .close_port = common_close_port,
	 .check_device_file = common_check_device_file,
	 .module_startup = T35i_module_startup,
	 .send = common_send,
	 .send_sms = common_send_sms,
	 .parse_line = common_parse_line,
	 .start_call = common_start_call,
	 .stop_call = common_stop_call,
	 .network_status = common_network_status,
	 .probe = common_probe,
	 .set_engineer_mode = common_set_engineer_mode,
	 .recv_sms = common_recv_sms,
	 },
};

/* check if the device is available or not */
int modat_ready(MODAT * p)
{
	AT_STATUS status;
	if (p == NULL)
		return -1;
	status = modat_get_status(p);
	return (status < SICK && status >= READY);
}

/* only used for AT send&recv, applications should use modat_ready(). */
int _modat_is_sick(MODAT * p)
{
	AT_STATUS status;
	if (p == NULL)
		return -1;
	status = modat_get_status(p);
	return (status >= SICK);
}

/* call this to check if your modat cmd has recved */
int check_at_recved(MODAT * p)
{
	if (_modat_is_sick(p))
		return 1;	/* force return true */
	return (p->at.status == AT_RECVED);
}

/* if you have collected all the data you need in AT_BUF, call it to 
   tell the daemon. */
void modat_recved_done(MODAT * p)
{
	modat_lock(p);
	p->at.status = AT_READY;
	modat_unlock(p);
}

/* check if modat buf is ready for your command */
int check_at_ready(MODAT * p)
{
	if (_modat_is_sick(p))
		return 1;	/* force return true */
	return (p->at.status == AT_READY);
}

/* when you have fulfilled modat_sendbuf and keyword as you want, call it 
   to let daemon know. */
void modat_send_it(MODAT * p)
{
	modat_lock(p);
	p->at.status = AT_REQUEST;
	modat_unlock(p);
}

int modat_lock(MODAT * p)
{
	return pthread_mutex_lock(&p->mutex);
}

int modat_unlock(MODAT * p)
{
	return pthread_mutex_unlock(&p->mutex);
}

/* ONLY IF we do the assignment of the functions here, 
   can we use these functions in the host thread later on. */
void modat_register_methods(MODAT * p)
{
	MODAT_TYPE type = p->type;
	p->open_port = modat_supports[type].open_port;
	p->close_port = modat_supports[type].close_port;
	p->module_startup = modat_supports[type].module_startup;
	p->send = modat_supports[type].send;
	p->send_sms = modat_supports[type].send_sms;
	p->parse_line = modat_supports[type].parse_line;
	p->start_call = modat_supports[type].start_call;
	p->stop_call = modat_supports[type].stop_call;
	p->network_status = modat_supports[type].network_status;
	p->probe = modat_supports[type].probe;
	p->set_engineer_mode = modat_supports[type].set_engineer_mode;
	p->recv_sms = modat_supports[type].recv_sms;
}

MODAT_TYPE modat_detect(char *dev_file)
{
	/* TOFIX: auto detect of device */
	return T35i;
}

int _set_speed(MODAT * p, int speed)
{
	if (speed == 9600) {
		p->speed = B9600;
		return 0;
	}
	if (speed == 115200) {
		p->speed = B115200;
		return 0;
	}
	return -1;
}

#define		RBUF_SIZE			4096
/* init a device by its device file, e.g. "ttyUSB0" */
MODAT *modat_init(char *dev_file, unsigned int speed)
{
	static int device_count = 0;
	MODAT *pat = NULL;

	if (strlen(dev_file) >= sizeof(pat->dev_file))
		return NULL;

	/* do the malloc */
	pat = malloc(sizeof(MODAT));
	if (pat == NULL)
		return NULL;
	bzero(pat, sizeof(MODAT));

	/* set the speed of port */
	if (_set_speed(pat, speed)) {
		dbg(1, "speed of port %d not supported.", speed);
		goto free_modat;
	}

	/* malloc for rbuf */
	pat->r = rbuf_init(RBUF_SIZE);
	if (pat->r == NULL)
		goto free_modat;

	if (pat) {
		/* malloc ok, do the initialize. 
		 * default enabled */
		pat->enabled = 1;
		/* set device file */
		strcpy(pat->dev_file, dev_file);
		/* type cannot be changed after init of the modat */
		pat->type = modat_detect(dev_file);
		/* format name from type */
		snprintf(pat->name, sizeof(pat->name), "%s/%d",
			 modat_supports[pat->type].product_name,
			 device_count++);
		/* default status is INIT */
		pat->status = INIT;
		/* init mutex */
		pthread_mutex_init(&(pat->mutex), NULL);
		/* register related model functions to this device */
		modat_register_methods(pat);
		/* init simcard number by 0000 */
		strncpy(pat->sim, "0000", sizeof(pat->sim));
		/* defaultly set engineer mode off */
		pat->engineer_mode = 0;
	}

	/* create daemon thread for the device. */
	if (pthread_create(&pat->thread_id, NULL, device_daemon, pat)) {
		/* thread creation error */
		dbg(1, "create thread for device [%d][%s] error.", pat->type,
		    pat->name);
		goto free_rbuf;
	}
	/* all ok */
	dbg(2, "%s initialized successfully.", pat->name);
	return pat;

      free_rbuf:
	pthread_mutex_destroy(&(pat->mutex));
	free(pat->r);
      free_modat:
	free(pat);
	return NULL;
}

/* free a modat structure */
void modat_release(MODAT * pat)
{
	void *ret_thread;
	/* close the thread first */
	modat_set_status(pat, DEAD);
	pthread_join(pat->thread_id, &ret_thread);
	dbg(1, "daemon thread returned with %d.", (int)ret_thread);
	pat->close_port(pat);
	pat->r = rbuf_release(pat->r);
	if (pthread_mutex_destroy(&pat->mutex)) {
		dbg(1, "VITAL!! mutex is destroyed while some thread "
		    "is using it!");
	}
	free(pat);
}

/* we should not call this directly, since we don't know if
 * the pat is available now. To apply for a device resource, 
 * use functions given in dm-uid.h. */
int modat_user_malloc(MODAT * pat)
{
	if (pat == NULL)
		return -1;

	/* first lock the structure, make sure that we do add 1 to
	 * the user_count, if the device is available */
	modat_lock(pat);

	if (!modat_ready(pat)) {
		dbg(1, "at is sick. failed malloc.");
		modat_unlock(pat);
		return -2;
	}

	if (pat->users_count >= MAX_USERS_PER_MODEM) {
		dbg(1, "MAX_USER(%d) reached, while user(%d).",
		    MAX_USERS_PER_MODEM, pat->users_count);
		modat_unlock(pat);
		return -3;
	}

	/* we can use the device */
	pat->users_count++;

	dbg(1, "increase to %d USERS.", pat->users_count);

	modat_unlock(pat);
	return 0;
}

void modat_user_free(MODAT * pat)
{
	modat_lock(pat);
	pat->users_count--;
	dbg(1, "decrease to %d USERS.", pat->users_count);
	modat_unlock(pat);
}

void modat_set_enable(MODAT * pat)
{
	modat_lock(pat);
	pat->enabled = 1;
	modat_unlock(pat);
}

int modat_get_enable(MODAT * pat)
{
	if (pat == NULL)
		return -1;
	return pat->enabled;
}

void modat_set_status(MODAT * pat, MODAT_STATUS status)
{
	modat_lock(pat);
	pat->status = status;
	modat_unlock(pat);
	dbg(1, "change status to %d.", status);
}

MODAT_STATUS modat_get_status(MODAT * pat)
{
	return pat->status;
}

void modat_set_disable(MODAT * pat)
{
	modat_lock(pat);
	pat->enabled = 0;
	modat_unlock(pat);
}

char *at_get_sim(MODAT * p)
{
	if (p == NULL)
		return NULL;
	return p->sim;
}

int modat_get_engineer_mode(MODAT * p)
{
	return (p->engineer_mode);
}

int modat_set_engineer_mode(MODAT * p, int mode)
{
	modat_lock(p);
	if (mode)
		p->engineer_mode = 1;
	else
		p->engineer_mode = 0;
	modat_unlock(p);
	return 0;
}
