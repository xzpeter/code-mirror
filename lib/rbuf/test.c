#include <stdio.h>
#include <string.h>
#include "rbuf.h"

int main(void)
{
	RBUF *r;
	char *word;

	r = rbuf_init(1024);

	word = "this is a";
	r->write(r, word, strlen(word));
	word = " test.\rI think, I should ha";
	r->write(r, word, strlen(word));
	word = "ve seen a whole article here.";
	r->write(r, word, strlen(word));
	word = "\rIs it?\r";
	r->write(r, word, strlen(word));

	printf("%s\n", r->readline(r));
	printf("%s\n", r->readline(r));
	printf("%s\n", r->readline(r));

	return 0;
}
