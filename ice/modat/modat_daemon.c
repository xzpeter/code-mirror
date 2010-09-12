/*
 * file: modat_daemon.c
 * desc: implementations of daemon thread of MODAT device
 * mail: xzpeter@gmail.com
*/

#define	_GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include "modat_include.h"

#define DAEMON_RECV_BUFFER_SIZE 2048

/* some startup status */
#define  NOT_STARTUP     (0)
#define  INIT_STARTUP    (1)
#define  DURING_STARTUP  (2)
#define  OK_STARTUP      (3)
#define  FAILED_STARTUP  (4)

#define		IO_WAIT_TIME		5	/* units of 100ms */

void at_buffer_init(MODAT_AT_BUF * at)
{
	bzero(at, sizeof(MODAT_AT_BUF));
}

/* use the do_startup_work thread to help startup a modem */
void *do_startup_work(void *data)
{
	int ret;
	MODAT *p = (MODAT *) data;
	int startup_ok = 0;
	int retry = 3;

	dbg(1, "starting up device %s.", p->name);

	/* set start up status */
	p->start_up_status = DURING_STARTUP;

	/* open engineer mode, default is ON */
#ifdef ENGINEER_MODE_ON
	dbg(1, "opening engineer mode ...");
	if (p->set_engineer_mode(p, 1)) {
		dbg(1, "error in open engineer mode. setting DEAD.");
		modat_set_status(p, DEAD);
		goto do_startup_failed;
	}
	dbg(1, "now in engineer mode.");
#else
	p->set_engineer_mode(p, 0);
	dbg(1, "not opening engineer mode.");
#endif

#ifndef DM_SKIP_MODULE_STARTUP
	/* try startup */
	while (retry--) {
		dbg(1, "last %d attempt to start device %s.", retry, p->name);
		ret = p->module_startup(p);
		if (!ret) {
			dbg(1, "startup device %s successfully", p->name);
			startup_ok = 1;
			break;
		} else {
			dbg(1, "failed to startup device %s : %d", p->name,
			    ret);
		}
	}

	if (!startup_ok) {
		dbg(1, "device startup failed.");
		goto do_startup_failed;
	}
	dbg(1, "start up ok.");
#else
	dbg(1, "skipped startup.");
#endif

	/* successful */
	p->start_up_status = OK_STARTUP;
	return NULL;

      do_startup_failed:
	/* failed */
	p->start_up_status = FAILED_STARTUP;
	return (void *)-1;
}

void do_init_work(MODAT * p)
{
	/* the module is enabled by main process */
	dbg(1, "device enabled ... prepare to startup ...");

	/* try to open port */
	if (p->open_port(p)) {
		dbg(1, "open_port() failed.");
		modat_set_status(p, DEAD);
		return;
	}

	/* init AT buffer */
	at_buffer_init(&p->at);
	/* start up ok */
	modat_set_status(p, STARTUP);

	return;
}

/* all the infomation handles in here */
int handle_line(MODAT * p, char *line)
{
//      dbg(1, "HANDLE LINE: %s.", line);
	/* modem specified function to parse the line */
	p->parse_line(p, line);
	return 0;
}

/* this function is repeated called in daemon normal work.
   it's job is to help the host do the send & recv work of at cmd. 
   there should be 5 status, any of the status can only be changed 
   by 'host' OR 'daemon', never both. */
void do_send_and_recv(MODAT * p)
{
	MODAT_AT_BUF *at = &p->at;
	RBUF *r = p->r;

	/* do the responses */
	switch (at->status) {
	case AT_READY:
		break;
	case AT_REQUEST:
		{
			/* host has announced a query cmd */
			int datalen, sent;
			datalen = strlen(at->send_buf);
			dbg(3, "send : %s", at->send_buf);
			sent = write(p->portfd, at->send_buf, datalen);
			if (sent != datalen) {
				dbg(1,
				    "ERROR WRITE PORT, sent is %d, datalen %d.",
				    sent, datalen);
				at->ret = AT_NOT_RET;
				at->status = AT_RECVED;
				break;
			}
			/* clear recv buffer */
			bzero(at->recv_buf, sizeof(at->recv_buf));
			at->start_copy = 0;	/* initially not copy recved data into recv_buf */
			at->status = AT_SENT;
			/* remember sent timestamp */
			at->sent_time = time(NULL);
			sleep(1);
			break;
		}
	case AT_SENT:
		{
			time_t time_gone = time(NULL) - at->sent_time;
			if (time_gone >= AT_TIMEOUT_SEC) {
				/* wowow, it's totally timeout! */
				dbg(1,
				    "TIMEOUT AT cmd for %d time, force return.",
				    ++at->time_out_count);
				at->ret = AT_NOT_RET;
				at->status = AT_RECVED;
				if (at->time_out_count >= AT_TIMEOUT_COUNT_MAX) {
					dbg(1,
					    "this device has %d time(s) of at cmd timeout now, reseting.",
					    at->time_out_count);
					modat_set_status(p, NOT_RESPONDING);
					return;
				}
			}
		}
		break;
	case AT_RECVED:
		break;
	}

	/* do the daily recv */
	{
		unsigned int len, done;
		char buf[DAEMON_RECV_BUFFER_SIZE];
		char *pch, *pch2;

		/* added for Q2687, if portfd==0, do not read */
		/* TOFIX */
		if (p->portfd == 0) {
			/* do nothing */
			len = 0;
		} else {
			len =
			    read(p->portfd, buf, DAEMON_RECV_BUFFER_SIZE - 10);
		}

		if (len == DAEMON_RECV_BUFFER_SIZE - 10)
			dbg(1, "WARNING: RECV BUFFER MAY OVERFLOWED.");

		if (len > 0) {
			/* do some special treatment to string "\n>" when using SMS 
			   at cmd. solution is : add another "\n" after '\n>' */
			buf[len] = 0x00;
			//dbg(1, "recv : %s", buf);
			if (!strncmp(buf, "> ", 2))
				pch = buf;	/* "> " is found at the beginning (normally) */
			else
				pch = strstr(buf, "\n> ");	/* found in the middle */
			if (pch) {	/* no matter where, we found "> " */
				pch += 2;
				for (pch2 = buf + len - 1; pch2 > pch; pch2--)
					*(pch2 + 1) = *pch2;
				*(pch2 + 1) = '\n';
				len++;
				buf[len] = 0x00;
			}
			/* after this, "\n>" are converted to "\n>\n", 
			   now we can recog it.  so tricky ... */

			done = r->write(r, buf, len);
			if (done != len) {
				dbg(1,
				    "RBUF wrote length %d, while %d needed to write, "
				    "possibly the buffer is not big enough.",
				    done, len);
			}
		} else if (len < 0) {
			dbg(1, "error in read(): %s", strerror(errno));
			modat_set_status(p, DEAD);
			return;
		}
	}

	/* handle related infomation */
	{
		char *line;

		while ((line = r->readline(r))) {
//                      dbg(1, "recv : %s", line);
			/* recved a whole line */
			if (at->status == AT_SENT) {

				/* record required response */
//                              if (at->mode & AT_MODE_LINE) {
//                                      if (strcasestr(line, at->keyword))
//                                              /* always the newest! */
//                                              strncpy(at->recv_buf, line, MODAT_RECV_BUFLEN);
//                              } else { /* mode BLOCK */
//                                      /* xzpeter fixed 2010.6.9, mem overflow! when using strcat */
//                                      strncat(at->recv_buf, line, MODAT_RECV_BUFLEN - strlen(at->recv_buf));
//                              }

				if (at->start_copy) {
					if (at->mode == AT_MODE_BLOCK)
						strncat(at->recv_buf, line,
							MODAT_RECV_BUFLEN -
							strlen(at->recv_buf));
				} else {
					/* not start copy yet */
					if (at->keyword[0]
					    && strcasestr(line, at->keyword)) {
						/* if there is a keyword and matched */
						at->start_copy = 1;	/* trigger copy */
						strncpy(at->recv_buf, line,
							MODAT_RECV_BUFLEN);
					}
				}

				/* host is waiting for "OK" and "keyword" */
				if (strstr(line, "OK")) {
					at->ret = AT_OK;
					at->status = AT_RECVED;
					if (!at->start_copy)
						strcpy(at->recv_buf, line);
				} else if (strstr(line, "CME ERROR")) {
					at->ret = AT_CME_ERROR;
					at->status = AT_RECVED;
					if (!at->start_copy)
						strcpy(at->recv_buf, line);
				} else if (strstr(line, "CMS ERROR")) {
					at->ret = AT_CMS_ERROR;
					at->status = AT_RECVED;
					if (!at->start_copy)
						strcpy(at->recv_buf, line);
				} else if (strstr(line, "ERROR")) {
					at->ret = AT_ERROR;
					at->status = AT_RECVED;
					if (!at->start_copy)
						strcpy(at->recv_buf, line);
				} else if (!strncmp(line, "> ", 2)) {
					at->ret = AT_RAWDATA;
					at->status = AT_RECVED;
				} else if (strstr(line, "NO CARRIER")) {
					at->ret = AT_NO_CARRIER;
					at->status = AT_RECVED;
					if (!at->start_copy)
						strcpy(at->recv_buf, line);
				}
			}
			handle_line(p, line);
		}
	}
}

/* daemon thread, to take full control of a device. */
void *device_daemon(void *pdata)
{
	pthread_t startup_thread;
	MODAT *p = (MODAT *) pdata;
	dbg(1, "daemon for %s started.", p->name);
	int retval;

	/* daemon main process */
	while (1) {
		switch (modat_get_status(p)) {
		case INIT:
			if (modat_get_enable(p))
				do_init_work(p);
			break;
		case STARTUP:
			/* some specified startup steps */
			switch (p->start_up_status) {
			case NOT_STARTUP:
				/* create the start_up thread */
				dbg(1,
				    "starting startup_thread with DETACHED.");
				retval =
				    pthread_create(&startup_thread, NULL,
						   &do_startup_work, (void *)p);
				retval |= pthread_detach(startup_thread);
				if (!retval) {
					dbg(1,
					    "started startup_thread successfully");
					p->start_up_status = INIT_STARTUP;
				} else {
					dbg(1, "retval=%d, errno=%d, errstr=%s",
					    retval, errno, strerror(errno));
					dbg(1,
					    "failed to create startup_thread: %s",
					    strerror(retval));
					p->start_up_status = FAILED_STARTUP;
				}
				break;
			case INIT_STARTUP:
				/* do nothing */
				break;
			case DURING_STARTUP:
				/* the thread is start! waiting result. */
				break;
			case OK_STARTUP:
				/* startup done successfully */
				dbg(1,
				    "startup successfully, setting status to READY.");
				modat_set_status(p, READY);
				break;
			case FAILED_STARTUP:
				/* startup failed */
				dbg(1,
				    "startup failed, setting status to DEAD.");
				modat_set_status(p, DEAD);
				break;
			default:
				dbg(1, "start_up_status=%d, out of control.",
				    p->start_up_status);
				break;
			}
			do_send_and_recv(p);
			break;
		case READY:
		case WORKING:
			do_send_and_recv(p);
			/* do send and recv */
			break;
		case NOT_RESPONDING:
			modat_set_status(p, WAIT_USERS);
			break;
		case UNTACHED:
			dbg(1, "UNTACHED ?! Roger ! now there are %d users"
			    " accessing the modem.", p->users_count);
			modat_set_status(p, WAIT_USERS);
			break;
		case WAIT_USERS:
			if (p->users_count == 0) {
				dbg(1, "all users acknowledged. marking DEAD.");
				modat_set_status(p, DEAD);
			}
			break;
		case DEAD:
			dbg(1, "Hi, I am DEAD ... quitting thread now.");
			pthread_exit(NULL);
			break;
		default:
			dbg(1, "VITAL ERROR! p->type[%d] not known.", p->type);
			pthread_exit(NULL);
			break;
		}
		usleep(200000);
	}

	return (void *)0;
}
