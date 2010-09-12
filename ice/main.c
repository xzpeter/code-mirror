/*
 * file: main.c
 * desc: test program for modat codes
 * mail: xzpeter@gmail.com
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "modat/modat.h"
#include "modat/utils.h"
#include "config.h"
#include "ads321.h"
#include "dbg.h"

#define  SEND_BUF_LEN  10000

static struct timeval sample_timer;
static time_t start_time;
static char send_buf[SEND_BUF_LEN] = {0};

int debug_level = DEF_DEBUG_LEVEL, 
		raw_data_mode = DEF_RAW_DATA_MODE;

/* these 2 ports must be set in arguments */
static char gsm_port[128] = GSM_MODULE_PORT,
						ads321_port[128] = ADS321_PORT;

static unsigned int sample_interval = SAMPLE_INTERVAL, 
										send_interval = SEND_INTERVAL;

/* save default report saving directory */
static char log_dir[256] = DEF_LOG_DIR;

int start_sensor_index = START_SENSOR_INDEX, 
		end_sensor_index = END_SENSOR_INDEX;
static int sensor_number = DEFAULT_SENSOR_N;

/* temporary save sample data */
ADS_DATA ads_data[MAX_SENSOR_N];

/* if sms_report_on, send the report throw SMS too. */
static int sms_report_on = 0;

static char report_data_file[100];

void clear_buffer(void)
{
	bzero(&ads_data, sizeof(ads_data));
}

int sample_one_sensor(int dev_no, double *weight, double *temp)
{
	if (dev_no < start_sensor_index || dev_no >= end_sensor_index) {
		dbg(1, "sensor index not right(%d<..<%d, dev_no=%d).", 
				start_sensor_index, end_sensor_index, dev_no);
		return -1;
	}

	/* sample the weight */
	if (ads321_get_weight(dev_no, weight)) {
		dbg(1, "read weight failed(address=%d).", dev_no);
		*weight = 0.0;
		return -2;
	}
	dbg(3, "address=%d, read weight=%lf.\n", dev_no, *weight);

	/* sample the temperature */
	if (ads321_get_temp(dev_no, temp)) {
		dbg(1, "read temperature failed(address=%d).", dev_no);
		*temp = 0.0;
		return -3;
	}
	dbg(3, "address=%d, read temp=%lf.", dev_no, *temp);

	return 0;
}

void format_one_report(int dev_no, double weight, double temp)
{
	char buf[256], ts[10], ds[10];
	time_t the_time;
	struct tm *tt;

	/* buffer the result */
	the_time = time(NULL);
	tt = localtime(&the_time);
	make_date(ds);
	make_time(ts);
	snprintf(buf, 256, "%s,%s,%02d,%04.2lf,%02.2lf\n", 
			ds, 
			ts,
			dev_no,
			weight,
			temp);
	strcat(send_buf, buf);
}

void do_sample(void)
{
	int dev_no, ok_channel = 0;

	/* first, clear buffer. if the result is 0, it's possibly failed in sampling. */
	clear_buffer();

	dbg(1, "start sampling.");
	
	/* sample the sensors one by one */
	for (dev_no = start_sensor_index; dev_no < end_sensor_index; dev_no++) {
		int i = dev_no - start_sensor_index;
		double weight, temp;
		dbg(1, "sampling channel %d...", dev_no);
		if (sample_one_sensor(dev_no, &weight, &temp)) {
			dbg(1, "sample sensor(address=%d) failed.", dev_no);
		} else {
			dbg(1, "sample sensor(address=%d) successfully. (%lf, %lf)", 
					dev_no, weight, temp);
			ads_data[i].weight = weight;
			ads_data[i].temp = temp;
			ok_channel++;
		}
		/* buffer the single report */
		format_one_report(dev_no, ads_data[i].weight, ads_data[i].temp);
	}
	dbg(1, "%d of %d channels sampled OK.", ok_channel, sensor_number);
}

void log_sample_data(void)
{
	int fd = open(report_data_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
	if (fd == -1) {
		dbg(1, "WARNING! failed to log sample data.");
		return;
	}
	write(fd, send_buf, strlen(send_buf));
	close(fd);
	dbg(1, "sample data saved OK.");
}

void init_report_file(void)
{
	/* init report name */
	char t1[10], t2[10], cmd[256];
	make_date(t1);
	make_time(t2);
	snprintf(report_data_file, sizeof(report_data_file), 
			"%s/report.%s-%s.txt", 
			log_dir, 
			t1,
			t2);
	unlink("report.latest");
	snprintf(cmd, sizeof(cmd), 
			"ln -s %s report.latest",
			report_data_file);
	system(cmd);
	dbg(1, "report logging to file: %s (report.latest)", report_data_file);
}

void usage(void)
{
	puts("");
	puts("usage: ice [-s] [-a <ADS321_PORT>] [-d <DEBUG_LEVEL>] [-g <GSM_PORT>] [-l <SAMPLE_INTERVAL_LEN>] [-t <SAMPLE_TIMES>]"
			"[-S <START_SENSOR_INDEX>] [-E <END_SENSOR_INDEX>] [-r] [-L <LOG_DIR>] [-h] [-v]");
	puts("");
	puts("*** some params can be set for program:");
	puts("-a to set ADS321 port.");
	puts("-d to set debug level (only for verbose debug)");
	puts("-g to set GSM port.");
	puts("-l to set sample periods. (in second).");
	puts("-t to set how many times to buffer before each report sent.");
	puts("-s to enable SMS report to server.");
	puts("-S to set start sensor index to sample.");
	puts("-E to set end sensor index to sample(CAUTION: itself is not included, start<=n<end will be sampled");
	puts("-r to enable raw data mode in data sampling.");
	puts("-L to set default log directory on the disk.");
	puts("e.g. : ice -s -a /dev/ttySAC1 -g /dev/ttySAC2");
	puts("");
	puts("*** some other params:");
	puts("-h to show this help information.");
	puts("-v to show version.");
	puts("");
	exit(0);
}

void parse_options(int argc, char *argv[])
{
	int opt;
	dbg(3, "argc=%d. start parsing options...", argc);
	while ((opt = getopt(argc, argv, "a:d:g:hl:rst:vE:R:S:")) != -1) {
		dbg(3, "arg=%d", (unsigned int)opt);
		switch (opt) {
			case 'a':
				dbg(3, "setting ads321 port to %s", optarg);
				strcpy(ads321_port, optarg);
				break;
			case 'g':
				dbg(3, "setting gsm port to %s", optarg);
				strcpy(gsm_port, optarg);
				break;
			case 's':
				dbg(3, "setting sms report on.");
				sms_report_on = 1;
				break;
			case 'l':
				dbg(3, "setting sample interval to %s.", optarg);
				sample_interval = atoi(optarg);
				break;
			case 't':
				dbg(3, "setting send interval to %s.", optarg);
				send_interval = atoi(optarg);
				break;
			case 'd':
				dbg(3, "setting debug level to %s", optarg);
				debug_level = atoi(optarg);
				break;
			case 'S':
				dbg(3, "setting start sensor index to %s", optarg);
				start_sensor_index = atoi(optarg);
				break;
			case 'E':
				dbg(3, "setting end sensor index to %s", optarg);
				end_sensor_index = atoi(optarg);
				break;
			case 'r':
				dbg(3, "setting raw data mode on.");
				raw_data_mode = 1;
				break;
			case 'R':
				dbg(3, "setting default log directory to %s", optarg);
				strcpy(log_dir, optarg);
				break;
			case 'v':
				printf("ICE version: %d.%d\n", PRIMARY_VERSION, SECONDARY_VERSION);
				exit(0);
				break;
			case 'h':
			default:
				usage();
				break;
		}
	}

	dbg(3, "start checking params...");
	sensor_number = end_sensor_index - start_sensor_index;
	if (sensor_number > MAX_SENSOR_N) {
		dbg(1, "sensor number max is %d(now is %d).", MAX_SENSOR_N, sensor_number);
		usage();
	}
	if (!strlen(ads321_port)) {
		dbg(1, "it is a must to set ADS321 port for the program.");
		usage();
	}
	if (sms_report_on && !strlen(gsm_port)) {
		dbg(1, "GSM modem port must be set with sms report turned on.");
		usage();
	}
	if (send_interval > 20) {
		dbg(1, "send interval cannot > 20.(buffer may overflow)");
		usage();
	}

	/* show info */
	dbg(1, "Start sensor index is %d", start_sensor_index);
	dbg(1, "End sensor index is %d", end_sensor_index);
	dbg(1, "Ice now working in mode %s.", sms_report_on?"SMS":"NO SMS");
	dbg(1, "GSM modem should be connected to %s.", gsm_port);
	dbg(1, "ADS321 should be connected to %s.", ads321_port);
	dbg(1, "Sample interval is %u.", sample_interval);
	dbg(1, "Send interval is %u.", send_interval);
	dbg(1, "Debug level is %d.", debug_level);
	dbg(1, "Raw data mode is %s.", raw_data_mode?"ON":"OFF");
	dbg(1, "Log directory is \"%s/\".", log_dir);
}

int main(int argc, char *argv[])
{
	int sample_count, ret;
	MODAT *at = NULL;

	/* init value */
	time(&start_time);
	bzero(&ads_data, sizeof(ads_data));

	dbg(1, "ICE program started at %s", ctime(&start_time));
	dbg(1, "ICE program version is %d.%d", PRIMARY_VERSION, SECONDARY_VERSION);

	parse_options(argc, argv);

	if (sms_report_on) {
		/* init GSM module */
		while (NULL == (at = modat_init(gsm_port, GSM_MODULE_BAUD_RATE))) {
			dbg(1, "GSM module init failed. try again in 5 sec...");
			sleep(5);
		}
		dbg(1, "init module T35i successfully. waiting startup...");

		/* wait for startup of the modem */
		while (!modat_ready(at))
			sleep(1);

		dbg(1, "Modem start up ok.");
	} else {
		dbg(1, "in NO SMS mode, not init GSM module.");
	}

	/* init ADS321 */
	while (ads321_init(ads321_port)) {
		dbg(1, "ADS321 init failed. Try again in 5 sec ...");
		sleep(5);
	}
	dbg(1, "init ADS321 successfully.");

	init_report_file();

	/* reset timer of sampling, and counter */
	reset_timer(&sample_timer);
	sample_count = 0;

	dbg(1, "Now starting the main loop.");
	/* main loop */
	while (1) {
		/* sample process */
		if (get_elapsed_sec(sample_timer) >= sample_interval) {
			do_sample();
			sample_count++;
			reset_timer(&sample_timer); 
		}
		/* send report process */
		if (sample_count >= send_interval) {
			dbg(1, "report content: \n---------\n%s---------", send_buf);
			/* record report in data log */
			log_sample_data();
			/* send sms if necessary */
			if (sms_report_on) {
				/* GSM MODE */
				if ((ret = at->send_sms(at, SERVER_PEER_NUM, send_buf))) {
					dbg(1, "send sms failed, ret=%d", ret);
				} else {
					dbg(1, "data report sent successfully.\n");
				}
			}
			sample_count = 0;
			send_buf[0] = 0x00;
		}
		sleep(1);
	}

	return 0;
}
