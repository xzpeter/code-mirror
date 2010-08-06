/*
	desc: this file is used to dump .wav file infomation in the wav header
	mailto: xzpeter@gmail.com
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../wav_func.h"

void usage(char *name)
{
	printf("usage: %s WAV1 WAV2 ...\n", name);
	exit(0);
}

int main(int argc, char *argv[])
{
	int res, i;

	if (argc < 2)
		usage(argv[0]);

	for (i=1; i<argc; i++) {
		char *wavfile = argv[i];
		int baud, channel, bits;
		printf("========================\n");
		printf("file name:\t%s\n", wavfile);
		res = get_wav_info(&baud, &channel, &bits, wavfile);
		if (res) {
			printf("failed in get_wav_info(), ret=%d.\n", res);
			exit(-1);
		}
// 		printf("baud rate:\t%d\n", baud);
// 		printf("channels: \t%d\n", channel);
// 		printf("bits:     \t%d\n", bits);
	}
	printf("========================\n");
	return 0;
}
