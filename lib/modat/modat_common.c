/*
 * file: modat_common.c
 * desc: basic support of modem device
 * mail: xzpeter@gmail.com
 */

#include <stdio.h>
#include <termio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "modat_include.h"

#define			MAX_CELLID_LEN		20

/****************************
 * LOCALLY USED FUNCTIONS	*
 ****************************/

int common_set_sms_mode(MODAT * p, int n)
{
	char cmd[32];

	if (p == NULL || (n != 0 && n != 1))
		return -1;

	snprintf(cmd, 32, "AT+CMGF=%d", n);
	return p->send(p, cmd, NULL, AT_MODE_LINE);
}

/****************************
 * VIRTUAL DEVICE FUNCTIONS	*
 ****************************/

/* CAUTION : before call all these functions in the host thread, 
	 we should always get the mutex of the modat.
AND : we should never touch the mutex in all these functions below,
or deadlocks may occur. */

/* send a SMS */
int common_send_sms(MODAT * p, char *who, char *data)
{
	/* p->send must be implemented first */
	char cmd_buf[512];
	char res_buf[512];
	int ret;

	dbg(3, "[dm] common_send_sms entry for sending sms");

	if (!p || !who || !data)
		return -1;

	if ((ret = strlen(who)) > MAX_CELLID_LEN) {
		dbg(1, "CELLID TOO LONG in common_send_sms(), [len=%d] : %s",
				ret, who);
		goto send_sms_error;
	}
	if (strlen(data) >= 500) {
		dbg(1, "DATA TOO LONG in common_send_sms().");
		goto send_sms_error;
	}

	dbg(3, "Ready to send: %s", data);

	ret = snprintf(cmd_buf, 510, "AT+CMGS=%s", who);
	/* send CMGS cmd */
	ret = (int)p->send(p, cmd_buf, res_buf, AT_MODE_BLOCK);
	if (ret != AT_RAWDATA) {
		dbg(1, "SEND SMS cmd error, it's %d, should %d.", ret,
				AT_RAWDATA);
		goto send_sms_error;
	}
	sleep(1);

	snprintf(cmd_buf, 510, "%s\x1A", data);
	ret = (int)p->send(p, cmd_buf, res_buf, AT_MODE_BLOCK);
	if (ret != AT_OK) {
		dbg(1, "SEND RAWDATA got: %s", res_buf);
		dbg(1, "SEND RAW DATA error, return is %d, should %d.", ret,
				AT_OK);
		goto send_sms_error;
	}
	/* send ok */
	dbg(2, "SMS sent: %s ---> %s", data, who);
	return 0;

send_sms_error:
	return -1;
}

/* recv a sms */
int common_recv_sms(MODAT *p, int pos, char *peer, char *data, int size)
{
	/* p->send must be implemented first */
	char cmd_buf[512];
	char res_buf[2048];
	int ret;

	char *pch1, *pch2;

	dbg(3, "[dm] common_recv_sms entry for incoming sms %d", pos);

	if (!p || !data)
		return -1;

	/* first, get the ownership of the device */
	//      lock_AT_interface(p);
	if (pos < 0 || pos > 40) {
		dbg(1, "Invalid position in common_recv_sms(), [pos=%d]", pos);
		goto recv_sms_error;
	}

	ret = snprintf(cmd_buf, 510, "AT+CMGR=%d", pos);
	/* send CMGR cmd */
	dbg(3, "[dm] send %s in common_recv_sms #%d sms", cmd_buf, pos);
	ret = (int)p->send(p, cmd_buf, res_buf, AT_MODE_BLOCK);
	if (ret != AT_OK) {
		dbg(1, "RECV SMS cmd error, it's %d, should %d.", ret, AT_OK);
		goto recv_sms_error;
	}
	if ((pch2 = strstr(res_buf, "+CMGR:")) == NULL) {	// 不是一条需要处理的短信息
		*data = 0x00;
		goto recv_sms_error;
	}
	// get the peer number
	char *pch3, *pch4;
	int id_len;

	if ((pch3 = strstr(pch2, ",\"+86")) == NULL) {
		dbg(1, "%s: peer not found.", __func__);
	} else {
		dbg(1, "peer found. pch3=%s", pch3);
		pch3 += 5;	// start of the phone number
		pch4 = strchr(pch3, '"');	// end of the phone
		id_len = pch4 - pch3;
		dbg(1, "pch4=%s", pch3);
		if (pch4 != NULL && id_len < 12) {
			memcpy(peer, pch3, id_len);
			peer[id_len] = 0x00;
			dbg(2, "sms from peer : %s", peer);;
		} else {
			dbg(1, "%s: peer not found.", __func__);
		}
	}

	// 检查短消息的内容是否为空
	if (NULL == (pch1 = strstr(pch2, "\"\r\n")))	
		// 查找第一行末尾的\"\r\n"字符串，不能忽略最前面的"，否则会查找到第一行的开头
		if (NULL == (pch1 = strstr(pch2, "\"\n")))
			pch1 = strstr(pch2, "\"\r");

	if (pch1 == NULL || *(pch1 + 2) == 0x00 || *(pch1 + 3) == 0x00) {	//短消息内容为空字符串
		*data = 0x00;
		goto recv_sms_error;
	}
	pch1 += 3;

	// 将短消息的内容拷贝到psms_buf[]缓冲区中
	pch2 = strpbrk(pch1, "\r\n");
	*pch2 = 0x00;
	if ((pch2 - pch1) > (size - 1)) {
		memcpy(data, pch1, size - 1);
		*(data + size - 1) = 0x00;
		goto recv_sms_error;
	} else
		strncpy(data, pch1, (size - 1));

	/* recv ok */
	dbg(2, "SMS recv : %s <--- %11s", data, p->name);

	ret = snprintf(cmd_buf, 510, "AT+CMGD=%d", pos);
	/* send CMGD cmd */
	ret = (int)p->send(p, cmd_buf, res_buf, AT_MODE_BLOCK);
	if (ret != AT_OK) {
		dbg(1, "DELETE SMS cmd error, it's %d, should %d.", ret, AT_OK);
		goto recv_sms_error;
	}
	return 0;

recv_sms_error:
	return -1;
}

/****************************
 *  BASIC MODATICE FUNCTIONS	*
 ****************************/

/* CAUTION!! this function is really common used, but really dangerous, 
	 you must be sure that *data is OBSOLUTELY large enough for any possible
	 data. the recommended length is 1024 AT LEAST */
AT_RETURN common_send(MODAT * p, char *data, char *result, int mode)
{
	char *p1, *p2;
#define KEY_LEN 128
	char key[KEY_LEN] = { 0 };
	int len;

	if (!p || !data)
		return AT_NOT_RET;

	if ((len = strlen(data)) >= 500) {
		dbg(1, "AT CMD TOO LONG, in common_send().");
		return AT_NOT_RET;
	}

	/* wait until device is ready */
	while (!check_at_ready(p))
		usleep(100000);
	if (_modat_is_sick(p))
		return AT_NOT_RET;

	p->at.mode = mode;
	p->at.recv_buf[0] = 0x00;	/* clear recv buffer */

	if (1)			// (mode & AT_MODE_LINE)
	{
		/* find the keyword */
		p1 = strpbrk(data, "+^");
		if (p1) {
			p2 = strpbrk(p1++, "=?");
			if (p2) {
				memcpy(key, p1, p2 - p1);
				key[p2 - p1] = 0x00;
			} else {
				strncpy(key, p1, KEY_LEN);
			}
		} else {
			/* no key */
			key[0] = 0x00;
		}
		strncpy(p->at.keyword, key, MODAT_KEYWORD_LEN);
	} else {		/* AT_MODE_BLOCK */
		/* save all the block, not a keyword */
		p->at.keyword[0] = 0x00;
	}

	if (data[len - 1] == 0x1a) {
		/* do special treatment while sending sms */
		strncpy(p->at.send_buf, data, MODAT_SEND_BUFLEN);
		dbg(1, "[common_send] send_buf: %s", data);
	} else
		snprintf(p->at.send_buf, sizeof(p->at.send_buf), "%s\r", data);

	/* order to send! */
	modat_send_it(p);
	/* wait until job is done */
	while (!check_at_recved(p))
		usleep(100000);
	if (_modat_is_sick(p))
		return AT_NOT_RET;

	/* if the caller supplied a buffer to recv results, let's do it. 
	 * buffer MUST be large enough, or mem overflow may occur.
	 * so it's dangerous to use the result pointer */
	if (result)
		strcpy(result, p->at.recv_buf);

	/* data recved in p->at.recv_buf */
	modat_recved_done(p);
	return p->at.ret;
}

/* do the startup work for your module, 
	 returns 0 if startup ok, else if error. */
int common_module_startup(MODAT * p)
{
	return -1;
}

/* do anything you want to check whether the device file
	 belongs to you. return 1 if so, return 0 to discard. */
int common_check_device_file(char *file)
{
	/* by default, adopt nothing */
	return 0;
}

void common_close_port(MODAT * p)
{
	modat_lock(p);
	if (p->portfd) {
		close(p->portfd);
		p->portfd = 0;
	}
	modat_unlock(p);
}

int common_open_port(MODAT * pmodat)
{
	char portdevice[256];
	int portfd;

	/* if the port is already opened */
	if (pmodat->portfd > 0) {
		dbg(1, "port already opened.");
		return 0;
	}

	strcpy(portdevice, pmodat->dev_file);

	/* try to open port */
	if ((portfd = serial_open_port(portdevice, pmodat->speed)) < 0) {
		dbg(1, "serial_open_port() failed.");
		return -1;
	}

	/* save fd */
	modat_lock(pmodat);
	pmodat->portfd = portfd;
	modat_unlock(pmodat);

	return 0;
}

int common_parse_line(MODAT * p, char *line)
{
	return 0;
}

int common_start_call(MODAT * p, char *who)
{
	char cmd[64];
	int n = strlen(who);
	if (n < 0 || n > MAX_MOBILE_ID_LEN)
		return AT_NOT_RET;
	snprintf(cmd, 64, "ATD%s;", who);
	return p->send(p, cmd, NULL, AT_MODE_LINE);
}

int common_stop_call(MODAT * p)
{
	return p->send(p, "ATH", NULL, AT_MODE_LINE);
}

int common_network_status(MODAT * p, char *buf)
{
	return p->send(p, "AT+CREG?", buf, AT_MODE_LINE);
}

int common_probe(MODAT * p)
{
	return p->send(p, "AT", NULL, AT_MODE_LINE);
}

/* read the 1st entry in the address book to get self SIMcard number */
int common_get_self_sim_number(MODAT * p, char *num)
{
	char buf[1024];
	memset(buf, 0x00, sizeof(buf));
	if (p->send(p, "AT+CPBR=01", buf, AT_MODE_LINE))
		return -1;
	/* get self number */
	dbg(1, "response of AT+CPBR=1:%s", buf);
	sscanf(buf, "+CPBR: 1,\"%[0-9+]s\"", num);
	dbg(1, "get telephone number is %s", num);
	return 0;
}

/* set engineer mode */
int common_set_engineer_mode(MODAT * p, int on)
{
	char *cmd;
	if (on) {
		cmd = "AT^DCTS=1,64\r";
		/* turn engineer mode on */
		p->send(p, "AT^DCTS=1,64", NULL, AT_MODE_BLOCK);
		modat_set_engineer_mode(p, 1);
	} else {
		cmd = "AT^DCTS=0\r";
		/* turn off */
		p->send(p, "AT^DCTS=0", NULL, AT_MODE_BLOCK);
		modat_set_engineer_mode(p, 0);
	}
	return 0;
}
