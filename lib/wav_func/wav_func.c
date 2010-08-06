/*
 * wav_func.c
 * play, record or check a wav file
 * Peter Xu
 * 2009-10-17
 */
// wav_func v0.4
// obtain_one_channel() corrected
// for NMS, channel0 is empty, so we should take channel1
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/soundcard.h>

#include "lm_wav.h"

#define OPEN_DSP_FAILED			0x00000001    /*打开  dsp 失败!*/
#define SAMPLERATE_STATUS		0x00000002    /*samplerate status failed*/
#define SET_SAMPLERATE_FAILED	0x00000003    /*set samplerate failed*/
#define CHANNELS_STATUS			0x00000004    /*Channels status failed*/
#define SET_CHANNELS_FAILED		0x00000005    /*set channels failed*/
#define FMT_STATUS				0x00000006    /*FMT status failed*/
#define SET_FMT_FAILED			0x00000007    /*set fmt failed*/
#define OPEN_FILE_FAILED		0x00000008    /*opem filed failed*/

#define	LOGHEAD		"[wav_func] "

int audio_play(char *pathname, int nSampleRate, int nChannels, int fmt)
{
	int dsp_fd,status,arg;
	dsp_fd = open("/dev/dsp" , O_WRONLY);   /*open dsp*/
	if(dsp_fd < 0)
	{
		return  OPEN_DSP_FAILED;
	}
	arg = nSampleRate;
	status = ioctl(dsp_fd,SOUND_PCM_WRITE_RATE,&arg); /*set samplerate*/
	if(status < 0)
	{
		close(dsp_fd);
		return SAMPLERATE_STATUS;
	}
	if(arg != nSampleRate)
	{
		close(dsp_fd);
		return SET_SAMPLERATE_FAILED;
	}
	arg = nChannels;  /*set channels*/   
	status = ioctl(dsp_fd, SOUND_PCM_WRITE_CHANNELS, &arg);
	if(status < 0)
	{
		close(dsp_fd);
		return CHANNELS_STATUS;
	}
	if( arg != nChannels)
	{
		close(dsp_fd);
		return SET_CHANNELS_FAILED;
	}
	arg = fmt; /*set bit fmt*/
	status = ioctl(dsp_fd, SOUND_PCM_WRITE_BITS, &arg);
	if(status < 0)
	{
		close(dsp_fd);
		return FMT_STATUS;
	}
	if(arg != fmt)
	{
		close(dsp_fd);
		return SET_FMT_FAILED;
	}

	/*到此设置好了DSP的各个参数*/            
	FILE *file_fd = fopen(pathname,"rb");
	if(file_fd == NULL)
	{
		close(dsp_fd);
		return OPEN_FILE_FAILED;
	}
	// here read 1 sec of data to output
	int num = nChannels*nSampleRate*fmt/8;
	char buf[num];
	int get_num;
	while(feof(file_fd) == 0)
	{
		get_num = fread(buf,1,num,file_fd);
		write(dsp_fd,buf,get_num);
	}
	close(dsp_fd);
	fclose(file_fd);
	return 0;
}

int get_wav_info(int *baud_rate, int *channels, int *bits, char *file_name)
{
	RIFF_HEADER riff_header;
	FMT_BLOCK fmt_block;
	FILE *file = NULL;
	// err should always be zero
	int err = 0;
	int ret = 0;
	short extend_info = 0;

	// clear the structs
	memset(&riff_header, 0, sizeof(riff_header));
	memset(&fmt_block, 0, sizeof(fmt_block));

	// check if the file is valid, and open it
	file = fopen(file_name, "rb");
	if(file == NULL) {
		printf(LOGHEAD "cannot open file %s.\n", file_name);
		return -1;
	}

	// read riff_header
	ret = fread(&riff_header, 1, sizeof(riff_header), file);
	if(ret != sizeof(riff_header))
		err = 1;
	if(strncmp(riff_header.szRiffID, "RIFF", 4))
		err = 1;
	if(strncmp(riff_header.szRiffFormat, "WAVE", 4))
		err = 1;

	// read riff_header error
	if(err) {
		printf(LOGHEAD "read RIFF_HEADER error.\n");
		return -2;
	}

	// read format block
	ret = fread(&fmt_block, 1, sizeof(fmt_block), file);
	if(ret != sizeof(fmt_block))
		err = 1;
	if(strncmp(fmt_block.szFmtID, "fmt ", 4))
		err = 1;
	if(fmt_block.dwFmtSize == 16)
		extend_info = 0;
	else if(fmt_block.dwFmtSize == 18)
		extend_info = 1;
	else
		err = 1;
	
	// read the extend_info if exists
	if(extend_info) {
		ret = fread(&extend_info, 1, 2, file);
		if(ret != 2)
			err = 1;
	}

	// error in reading fmt_block
	if(err) {
		printf(LOGHEAD "read FMT_BLOCK error.\n");
		return -3;
	}

	// show the infos in the struct
// 	printf(LOGHEAD "\n");
// 	printf(LOGHEAD "File name\t:\t%s\n", argv[1]);
// 	printf(LOGHEAD "\n");
	printf( "File sizet:\t%d\n", riff_header.dwRiffSize + 8);
	printf( "Format tag:\t0x%x\n", fmt_block.wavFormat.wFormatTag);
	printf( "Channels:\t%d\n", fmt_block.wavFormat.wChannels);
	printf( "Samples/sec:\t%d\n", fmt_block.wavFormat.dwSamplesPerSec);
	printf( "bytes/sec:\t%d\n", fmt_block.wavFormat.dwAvgBytesPerSec);
	printf( "Block align:\t%d\n", fmt_block.wavFormat.wBlockAlign);
	printf( "Bits/sample:\t%d\n", fmt_block.wavFormat.wBitsPerSample);
	printf( "Extended:\t0x%x\n", extend_info);
// 	printf(LOGHEAD "\n");

	// set the info
	*baud_rate = fmt_block.wavFormat.dwSamplesPerSec;
	*channels = fmt_block.wavFormat.wChannels;
	*bits = fmt_block.wavFormat.wBitsPerSample;

	fclose(file);
	
	return 0;
}

int form_wav_header(char *buf, int sample_rate, int channels, int bits, int sec)
{
	RIFF_HEADER riff_header;
	FMT_BLOCK fmt;
	DATA_BLOCK data_block;

	int raw_data_len, byte_per_sec;

	if(buf == NULL)
		return 0;

	// calc some args
	byte_per_sec = sample_rate * channels * bits / 8;
	raw_data_len = byte_per_sec * sec;

	// clear the header
	memset(&data_block, 0, sizeof(data_block));
	memset(&riff_header, 0, sizeof(riff_header));
	memset(&fmt, 0, sizeof(fmt));

	// set the riff_header structure
	strncpy(riff_header.szRiffID, "RIFF", 4);
	riff_header.dwRiffSize = raw_data_len + WAV_HEADER_SIZE - 8;
	strncpy(riff_header.szRiffFormat, "WAVE", 4);

	// set the fmt struture
	strncpy(fmt.szFmtID, "fmt ", 4);
	fmt.dwFmtSize = 16;
	fmt.wavFormat.wFormatTag = 0x0001;
	fmt.wavFormat.wChannels = channels;
	fmt.wavFormat.dwSamplesPerSec = sample_rate;
	fmt.wavFormat.dwAvgBytesPerSec = byte_per_sec;
// 	fmt.wavFormat.wBlockAlign = 0x0004;
	fmt.wavFormat.wBlockAlign = channels * bits / 8;
	fmt.wavFormat.wBitsPerSample = bits;
	 
	// set the data block
	strncpy(data_block.szDataID, "data", 4);
	data_block.dwDataSize = raw_data_len;

	// copy the data to the buffer pointer
	memcpy(buf, (char *)&riff_header, sizeof(riff_header));
	buf += sizeof(riff_header);
	memcpy(buf, (char *)&fmt, sizeof(fmt));
	buf += sizeof(fmt);
	memcpy(buf, (char *)&data_block, sizeof(data_block));

	return WAV_HEADER_SIZE;
}

// record a period of audio into file named filename, with all the arguments below
// sec is the length of record, in second
int audio_record(char *filename, int sample_rate, int channels, int bits, int sec, int append)
{
	int dev_dsp, fd, i, sec_len, arg, status, /*err, */mal_len;
	char *buf;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	if(!append) { // output a header
		char header[WAV_HEADER_SIZE];
		// open the wav file
		// if the file exists, then overwrite the header
		fd = open(filename, O_WRONLY | O_CREAT, mode);
		if(fd == -1) {
			printf(LOGHEAD "cannot open file %s.\n", filename);
			return -1;
		}
		// write the wav header file
		status = form_wav_header(header, sample_rate, channels, bits, sec);
		if(status != WAV_HEADER_SIZE) {
			printf(LOGHEAD "status != WAV_HEADER_SIZE\n");
			goto append_failed;
		}
		status = write(fd, header, WAV_HEADER_SIZE);
		if(status != WAV_HEADER_SIZE) {
			printf(LOGHEAD "status != WAV_HEADER_SIZE, the 2nd.\n");
			goto append_failed;
		}
// append_ok:
		close(fd);
		return 0;
append_failed:
		close(fd);
		return -1;
	}//if(append)
// 	err = 0;

	// this is the length required for data in 1 sec
	sec_len = sample_rate * channels * bits / 8;

	mal_len = sizeof(char) * sec_len + sizeof(RIFF_HEADER) + sizeof(FMT_BLOCK);
	printf(LOGHEAD "malloc size: %d in %s\n", mal_len, __func__);
	buf = malloc(mal_len);
	if(buf == NULL) {
		printf(LOGHEAD "malloc error in audio_record.\n");
		goto record_fail;
	}

	// open the dsp device
	dev_dsp = open("/dev/dsp" , O_RDONLY);   /*open dsp*/
	if(dev_dsp < 0)
	{
		printf(LOGHEAD "cannot open dsp file.\n");
		goto record_fail;
	}

	// open the wav file
	fd = open(filename, O_APPEND | O_WRONLY);
	if(fd < 0) {
		printf(LOGHEAD "cannot open file %s.\n", filename);
		goto record_fail;
	}

	/* 设置采样时的量化位数 */
	arg = bits;
	status = ioctl(dev_dsp, SOUND_PCM_WRITE_BITS, &arg);
	if (status == -1)
		printf(LOGHEAD "SOUND_PCM_READ_BITS ioctl failed\n"); 
	if (arg != bits)
		printf(LOGHEAD "unable to set sample size\n");

	/* 设置采样时的声道数目 */
	arg = channels; 
	status = ioctl(dev_dsp, SOUND_PCM_WRITE_CHANNELS, &arg);
	if (status == -1)
		printf(LOGHEAD "SOUND_PCM_READ_CHANNELS ioctl failed\n");
	if (arg != channels)
		printf(LOGHEAD "unable to set number of channels\n");

	/* 设置采样时的采样频率 */
	arg = sample_rate;
	status = ioctl(dev_dsp, SOUND_PCM_WRITE_RATE, &arg);
	if (status == -1)
		printf(LOGHEAD "SOUND_PCM_READ_RATE ioctl failed\n");
	if (arg != sample_rate)
		printf(LOGHEAD "unable to set sample rate\n");
	// settings all done

	printf(LOGHEAD "recording ...\n");
	// record for sec seconds
	for(i = 0; i < sec; i++) {
		// debugging
// 		printf(LOGHEAD "recording sec %d...", i);
		int len;
		len = read(dev_dsp, buf, sec_len);
		if(len != sec_len) {
			printf(LOGHEAD "read error\n");
			goto record_fail;
		}
		len = write(fd, buf, sec_len);
		if(len != sec_len) {
			printf(LOGHEAD "write error\n");
			goto record_fail;
		}
	}
	printf(LOGHEAD "record done.\n");

// record_ok:
	close(dev_dsp);
	close(fd);
	free(buf); buf = NULL;
	return 0;

record_fail:
	close(dev_dsp);
	close(fd);
	if (buf) { free(buf); buf = NULL; }
	return -1;
}

/* write a buffer in chop_size */
int split_write(int fd, char *buf, int len, int chop_size)
{
	int wrote_all = 0;
	char *p = buf;
	/* check args */
	if (len <= 0 || chop_size <= 0 || fd <= 0 || buf ==NULL)
		return -1;
	/* if no need to chop */
	if (len <= chop_size) {
		write(fd, buf, len);
		return 0;
	}
	/* do the chop write */
	while (len > 0) {
		/* write len chars starts with p */
		int size, wrote;
		if (len <= chop_size)
			size = len;
		else
			size = chop_size;
		if ((wrote = write(fd, p, size)) != size) {
			printf(LOGHEAD "write() error. size=%d, wrote=%d.\n", size, wrote);
			return (wrote_all+wrote);
		}
		wrote_all += size;
		p += size;
		len -= size;
	}
	return wrote_all;
}

// resave the file_name to single channel
int obtain_one_channel(char *file_name, char *file_name_new, int which)
{
	RIFF_HEADER riff_header;
	FMT_BLOCK fmt_block;
	DATA_BLOCK data_block;
	int file, ret, len, i, bytes_per_sample;
	char *buf, *buf_out, *pin, *pout, *pin_end;

	if((file = open(file_name, O_RDONLY)) == 0) {
		printf(LOGHEAD "error in open file %s\n", file_name);
		return -1;
	}

	// read the header structures
	ret = read(file, (char *)&riff_header, sizeof(RIFF_HEADER));
	if(ret != sizeof(RIFF_HEADER)) {
		printf(LOGHEAD "read riff header error.\n");
		goto read_header_fail;
	}
	ret = read(file, (char *)&fmt_block, sizeof(FMT_BLOCK));
	if(ret != sizeof(FMT_BLOCK)) {
		printf(LOGHEAD "read fmt block error.\n");
		goto read_header_fail;
	}
	ret = read(file, (char *)&data_block, sizeof(DATA_BLOCK));
	if(ret != sizeof(DATA_BLOCK)) {
		printf(LOGHEAD "read data block error.\n");
		goto read_header_fail;
	}

	len = data_block.dwDataSize;
	// since we have to delete one channel from two, len should be even number.
	if(len % 2) {
		printf(LOGHEAD "data len is not even!\n");
		goto read_header_fail;
	}
	if(fmt_block.wavFormat.wChannels != 2) {
		printf(LOGHEAD "channels is %d, which should be 2.\n", fmt_block.wavFormat.wChannels); 
		goto read_header_fail;
	}
	
	// malloc, and read file into buffer
	printf(LOGHEAD "malloc size: %d in %s\n", len, __func__);
	buf = malloc(sizeof(char) * len);
	printf(LOGHEAD "malloc size: %d in %s\n", len/2, __func__);
	buf_out = malloc(sizeof(char) * len / 2);
	if(buf == NULL || buf_out == NULL) {
		printf(LOGHEAD "malloc error.\n");
		goto singlize_fail;
	}

	ret = read(file, buf, len);
	if(ret != len) {
		printf(LOGHEAD "read data len is %d, actual data len in DATA_BLOCK is %d, error occured.\n", 
				ret, len);
		goto singlize_fail;
	}

	// finish reading file
 	close(file);

	// change header info
	len /= 2;
	data_block.dwDataSize -= len;
	riff_header.dwRiffSize -= len;
	fmt_block.wavFormat.wChannels = 1;
	fmt_block.wavFormat.wBlockAlign /= 2;
	fmt_block.wavFormat.dwAvgBytesPerSec /= 2;

	bytes_per_sample = fmt_block.wavFormat.wBitsPerSample / 8;

	// open new file
// 	strcpy(file_name_new, "single_channel_");
// 	strcat(file_name_new, file_name);
	if((file = open(file_name_new, O_WRONLY | O_CREAT, 0644)) == 0) {
		printf(LOGHEAD "error in open output file %s.\n", file_name_new);
		goto singlize_fail;
	}

	if(	!write(file, (char *)&riff_header, sizeof(riff_header)) || 
		!write(file, (char *)&fmt_block, sizeof(fmt_block)) ||
		!write(file, (char *)&data_block, sizeof(data_block)) ) {
		printf(LOGHEAD "write data error.\n");
		goto singlize_fail;
	}
	close(file);

	pin = buf;
	pin_end = buf + 2 * len;

 	pout = buf_out;

	// select which channel to remain, 0 for the first, 1 for the second.
	pin += (which % 2) * bytes_per_sample;

// 	for(i = 0; i < len; i++, pin += pin_interval)
// 		for(j = 0; j < bytes_per_sample; j++)
// 			*(pout++) = *(pin++);
// 	int ctr = 0;
	while(pin < pin_end) {
		for(i = 0; i < bytes_per_sample; i++) {
			*(pout++) = *(pin++);
		}
		pin += bytes_per_sample;
// 		ctr++;
	}

// 	if(ctr != len) {
// 		printf(LOGHEAD "error in copying wav data, copy time is %d, total time is %d.", ctr, len);
// 		goto fail_singlize_2;
// 	}

	if((file = open(file_name_new, O_WRONLY | O_CREAT, 0644)) == 0) {
		printf(LOGHEAD "error in open output file %s.\n", file_name_new);
		goto singlize_fail;
	}
	if(write(file, buf_out, len) != len) {
		printf(LOGHEAD "write wav data error.\n");
		goto singlize_fail;
	}
	close(file);
	//printf(LOGHEAD "close() returned %d.\n", close(file));

// singlize_ok:
	// job done
	free(buf); buf = NULL;
	free(buf_out); buf_out = NULL;
	return 0;

singlize_fail:
	close(file);
	if (buf) { free(buf); buf = NULL; }
	if (buf_out) { free(buf_out); buf_out = NULL; }
	return -2;

read_header_fail:
	close(file);
	return -1;
}

int obtain_one_channel_static(char *file_name, char *file_name_new, 
		int which)
{
	RIFF_HEADER riff_header;
	FMT_BLOCK fmt_block;
	DATA_BLOCK data_block;
	int file_single, file_double, ret, len, i, bytes_per_sample;
	char *pin, *pout, *pin_end;

	if((file_double = open(file_name, O_RDONLY)) == 0) {
		printf(LOGHEAD "error in open file %s\n", file_name);
		return -1;
	}

	// read the header structures
	ret = read(file_double, (char *)&riff_header, sizeof(RIFF_HEADER));
	if(ret != sizeof(RIFF_HEADER)) {
		printf(LOGHEAD "read riff header error.\n");
		goto static_read_header_fail;
	}
	ret = read(file_double, (char *)&fmt_block, sizeof(FMT_BLOCK));
	if(ret != sizeof(FMT_BLOCK)) {
		printf(LOGHEAD "read fmt block error.\n");
		goto static_read_header_fail;
	}
	ret = read(file_double, (char *)&data_block, sizeof(DATA_BLOCK));
	if(ret != sizeof(DATA_BLOCK)) {
		printf(LOGHEAD "read data block error.\n");
		goto static_read_header_fail;
	}

	len = data_block.dwDataSize;
	// since we have to delete one channel from two, len should be even number.
	if(len % 2) {
		printf(LOGHEAD "data len is not even!\n");
		goto static_read_header_fail;
	}
	if(fmt_block.wavFormat.wChannels != 2) {
		printf(LOGHEAD "channels is %d, which should be 2.\n", fmt_block.wavFormat.wChannels); 
		goto static_read_header_fail;
	}
	
	// change header info
	len /= 2;
	data_block.dwDataSize -= len;
	riff_header.dwRiffSize -= len;
	fmt_block.wavFormat.wChannels = 1;
	fmt_block.wavFormat.wBlockAlign /= 2;
	fmt_block.wavFormat.dwAvgBytesPerSec /= 2;

	bytes_per_sample = fmt_block.wavFormat.wBitsPerSample / 8;

	// open new file
// 	strcpy(file_name_new, "single_channel_");
// 	strcat(file_name_new, file_name);
	if((file_single = open(file_name_new, O_WRONLY | O_CREAT, 0644)) == 0) {
		printf(LOGHEAD "error in open output file %s.\n", file_name_new);
		goto static_singlize_fail;
	}

	if(	!write(file_single, (char *)&riff_header, sizeof(riff_header)) || 
		!write(file_single, (char *)&fmt_block, sizeof(fmt_block)) ||
		!write(file_single, (char *)&data_block, sizeof(data_block)) ) {
		printf(LOGHEAD "write data error.\n");
		goto static_singlize_fail;
	}
	close(file_single);

	{
		/* do the static_singlize 
		 * read 2*len from file_double, 
		 * write len to file_single,
		 * with block BLOCK_SIZE. */
#define		BLOCK_SIZE		1024
		char double_buf[BLOCK_SIZE];
		char single_buf[BLOCK_SIZE/2];
		int left_size = 2*len;
		while (left_size) {
			int block_size;
			if (left_size <= BLOCK_SIZE)
				block_size = left_size;
			else
				block_size = BLOCK_SIZE;
			read(file_double, double_buf, block_size);
			// select which channel to remain, 0 for the first, 1 for the second.
			pin = double_buf + (which % 2) * bytes_per_sample;
			pout = single_buf;
			pin_end = double_buf + block_size;
			while (pin < pin_end) {
				for(i = 0; i < bytes_per_sample; i++) {
					*(pout++) = *(pin++);
				}
				pin += bytes_per_sample;
			}

			/* I have to write just after open, so that memory leak problem
			 * disappears ... I don't know why. */
			if((file_single = open(file_name_new, O_WRONLY | O_APPEND)) == 0) {
				printf(LOGHEAD "error in open output file %s.\n", file_name_new);
				goto static_singlize_fail;
			}
 			write(file_single, single_buf, block_size/2);
			close(file_single);

			left_size -= block_size;
		}
	}
	//printf(LOGHEAD "close() returned %d.\n", close(file));

// static_singlize_ok:
	// job done
	close(file_single);
	close(file_double);
	return 0;

static_singlize_fail:
	close(file_single);
	close(file_double);
	return -2;

static_read_header_fail:
	close(file_double);
	return -1;
}
