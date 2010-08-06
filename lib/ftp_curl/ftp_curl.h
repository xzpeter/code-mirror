#ifndef		__FTP_CURL_H__
#define		__FTP_CURL_H__

/*
 *	file	:	ftp_curl.h
 *	desc	:	ftp functions based on libcurl
 *	author	:	Peter Xu
 *	time	:	2009.10.11
 */

typedef struct _ftp_curl_options {
	int timeout;
	int debug;
	double speed;
	double total_time;
	double bytes_done;
	long last_response;
} FTP_CURL_OPTIONS;

// both download & upload are able to auto-resume if there is a not-finished file
// debug = 1 for verbose, otherwise 0
// set timeout = 0 for no timeout

// init curl options
int init_curl_options(FTP_CURL_OPTIONS *options_p, int timeout, int debug);
// download a remote file to local file
int ftp_download_resumable(char *local_file, char *remote_file, 
		FTP_CURL_OPTIONS *option_p);
// upload a local file to remote file
int ftp_upload_resumable(char *remote_file, char *local_file, 
		FTP_CURL_OPTIONS *option_p);
// unlink a file on the remote server
int ftp_unlink(char *remote_url, FTP_CURL_OPTIONS *options_p);
// return curl error string
const char *ftp_curl_error_str(int err_no);

#endif
