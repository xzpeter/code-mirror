/*
 * file: main.c
 * desc: test program for modat codes
 * mail: xzpeter@gmail.com
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "modat_include.h"

int main(int argc, char *argv[])
{
	char buf[1024] = { 0 };
	AT_RETURN ret;
	MODAT *pat = NULL;
	if (argc != 3) {
		printf("usage: %s port speed.\n", argv[0]);
		return 0;
	}
	pat = modat_init(argv[1], atoi(argv[2]));
	if (pat == NULL) {
		dbg(1, "failed to call modat_init().");
		return -1;
	}
	while (!modat_ready(pat))
		sleep(1);
	ret = pat->send(pat, "AT", buf, AT_MODE_LINE);
	dbg(1, "ret=%d,buf=%s", ret, buf);
	ret = pat->send_sms(pat, "13811362029", "hello world!");
	dbg(1, "ret=%d,buf=%s", ret, buf);
	modat_release(pat);
	return 0;
}
