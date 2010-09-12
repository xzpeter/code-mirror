/*
 * file: ads321.h
 * desc: ADS321 module related codes
 * mail: xzpeter@gmail.com
 */

#ifndef __ADS321_H__
#define __ADS321_H__

/* ADS321 structures */
typedef struct _ads_data {
	double weight;
	double temp;
} ADS_DATA;

typedef union _ads_int {
	int i;
	unsigned char c[4];
} ads_int;

extern int ads321_init(char *port);
extern int ads321_get_weight(int dev_no, double *weight);
extern int ads321_get_temp(int dev_no, double *temp);

#endif
