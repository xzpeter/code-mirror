#include <stdio.h>
#include <stdlib.h>
#include "wav_func.h"

int main(int argc, char *argv[])
{
	int count, call_len, interval;
	char *file;
	count = 0;

	if (argc != 4) {
		printf("usage : %s file call_len interval\n", argv[0]);
		return -1;
	}

	file = argv[1];
	call_len = atoi(argv[2]);
	interval = atoi(argv[3]);
	audio_record(file, 8000, 2, 16, call_len, AUDIO_CREATE_HEADER);
	while(count < call_len) {
		int rec_time;
		rec_time = call_len - count;
		rec_time = rec_time > interval ? interval : rec_time;
		// record a short interval
		printf("recording %d sec ...\n", rec_time);
		audio_record(file, 8000, 2, 16, rec_time, AUDIO_RECORD_DATA);
		count += interval;
		// check if the call is still on
	}
	printf("done.\n");
	return 0;
}

