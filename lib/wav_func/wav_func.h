/*
 * wav_func.h
 * play, record or check a wav file
 * Peter Xu
 * 2009-9-10
 */
#ifndef		__WAV_FUNC_H__
#define		__WAV_FUNC_H__

#define		AUDIO_CREATE_HEADER		0
#define		AUDIO_RECORD_DATA		1

// play a wav file, with the specialized arguments below
int audio_play(char *pathname, int nSampleRate, int nChannels, int fmt);

// get the options of a wav file
int get_wav_info(int *baud_rate, int *channels, int *bits, char *file_name);

// record a period of audio into file named filename, with all the arguments below. sec is the length of record, in second
// added function appending record, @2009.11.6
// use append = 0 to create a file with corresponding wav header,
// use append = 1 to record audio date into wav file, which already inited.
int audio_record(char *filename, int sample_rate, int channels, int bits, int sec, int append);

// pick a single channel from a wav file
int obtain_one_channel(char *file_name, char *file_name_new, int which);
/* static version */
int obtain_one_channel_static(char *file_name, char *file_name_new, int which);

#endif
