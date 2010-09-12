/*
 * file: T35i.c
 * desc: T35i specified functions
 * mail: xzpeter@gmail.com
 */

#include "modat_include.h"

int T35i_module_startup(MODAT * p)
{
	/* for T35i, we don't need to do the startup */
	if (p->send(p, "AT", NULL, AT_MODE_LINE))
		return -1;
	return 0;
}
