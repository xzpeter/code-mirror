/*
 * file: config.h
 * desc: configurations for ice project
 * file: xzpeter@gmail.com
 */

#ifndef __ICE_CONFIG_H__
#define __ICE_CONFIG_H__

/* configs */
/* program version */
#define  PRIMARY_VERSION        0
#define  SECONDARY_VERSION      2

/* how many sensors are supported */
#define  MAX_SENSOR_N           6

#define  DEFAULT_SENSOR_N       4

/* the start index of sensors */
#define  START_SENSOR_INDEX     1
#define  END_SENSOR_INDEX       (START_SENSOR_INDEX + DEFAULT_SENSOR_N)

#define  DEF_DEBUG_LEVEL        1
#define  DEF_RAW_DATA_MODE      0
#define  DEF_LOG_DIR            "/sdcard"

#define  SAMPLE_INTERVAL        10 /* sec */
#define  SEND_INTERVAL          2  /* times to sample during one report period */
#define  SERVER_PEER_NUM        "13811362029"
#define  AD321_TIMEOUT_MAX_N    5

/* definations for ADS321 */
#ifndef PLATFORM_ARM
	#define  ADS321_PORT  "/dev/ttyUSB0"
	#define  GSM_MODULE_PORT  "/dev/ttyUSB1"
#else
	#define  ADS321_PORT  "/dev/ttyUSB0"
	#define  GSM_MODULE_PORT  "/dev/ttySAC1"
#endif

#define  ADS321_BAUD_RATE  19200
#define  GSM_MODULE_BAUD_RATE  9600
#define  ADS321_CRC_MODE  CRC_NONE

#endif
