#ifndef		__MOD_COMMON_H__
#define		__MOD_COMMON_H__

int common_send_sms(MODAT * p, char *who, char *data);

int common_recv_sms(MODAT * p, int pos, char *peer, char *data, int size);

AT_RETURN common_send(MODAT * p, char *data, char *result, int mode);

int common_device_file_adoptation(char *dev_file);

/* startup codes for modems */
int common_module_startup(MODAT * p);

/* check if the device file 'file' is the specific type */
int common_check_device_file(char *file);

/* close a port */
void common_close_port(MODAT * p);

/* common open a port */
int common_open_port(MODAT * pat);

/* parsing a line received from modem. */
int common_parse_line(MODAT * p, char *line);

/* start a call */
int common_start_call(MODAT * p, char *who);

/* stop a call */
int common_stop_call(MODAT * p);

/* checkout network status */
int common_network_status(MODAT * p, char *buf);

/* probe the modem */
int common_probe(MODAT * p);

/* read self SIM number and put the number into 'buf' */
int common_get_self_sim_number(MODAT * p, char *num);

/* set engineer mode */
int common_set_engineer_mode(MODAT * p, int on);

#endif
