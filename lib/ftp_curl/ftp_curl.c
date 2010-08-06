/*
 *	file	:	ftp_curl.c
 *	desc	:	ftp functions based on libcurl, modified from libcurl examples
 *	author	:	Peter Xu
 *	time	:	2009.10.11
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include "ftp_curl.h"

#define	LOGHEAD		"[ftp-curl] "
#define	BUF_LEN		256

struct FtpFile {
	const char *filename;
	FILE *stream;
};

int init_curl_options(FTP_CURL_OPTIONS *options_p, int timeout, int debug)
{
	if (options_p == NULL)
		return -1;

	options_p->timeout = timeout;
	options_p->debug = debug;
	options_p->total_time = 0;
	options_p->speed = 0;
	options_p->bytes_done = 0;
	options_p->last_response = 0L;

	return 0;
}

const char *ftp_curl_error_str(int err_no)
{
	return curl_easy_strerror((CURLcode) err_no);
}

struct curl_info {
	double speed_download;//CURLINFO_SPEED_DOWNLOAD
	double speed_upload;//CURLINFO_SPEED_UPLOAD
	double total_time;//CURLINFO_TOTAL_TIME
	long last_response;//CURLINFO_RESPONSE_CODE
} s_curl_info ;

int progress_callback(void *clientp, double dltotal, double dlnow, 
		double ultotal, double ulnow)
{
	double *bytes = (double *)clientp;
	if (dltotal) // in download progress
		*bytes = dlnow;
	else // in upload progress
		*bytes = ulnow;

	return 0;
}

int get_curl_info(CURL *curl)
{
	int ret;

	ret = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, 
			&s_curl_info.speed_download);
	if (ret != CURLE_OK)
		return -1;
	ret = curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, 
			&s_curl_info.speed_upload);
	if (ret != CURLE_OK)
		return -1;
	ret = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, 
			&s_curl_info.total_time);
	if (ret != CURLE_OK)
		return -1;
	ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, 
			&s_curl_info.last_response);
	if (ret != CURLE_OK)
		return -1;

	printf(LOGHEAD "download speed \t: %f.\n", s_curl_info.speed_download);
	printf(LOGHEAD "upload speed \t: %f.\n", s_curl_info.speed_upload);
	printf(LOGHEAD "total time \t\t: %f.\n", s_curl_info.total_time);
	printf(LOGHEAD "last respose \t: %ld.\n:", s_curl_info.last_response);

	return 0;
}

static size_t ftp_curl_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct FtpFile *out=(struct FtpFile *)stream;
	if(out && !out->stream) {
		/* open file for writing */
		out->stream=fopen(out->filename, "wb");
		if(!out->stream)
			return -1; /* failure, can't open file to write */
	}
	return fwrite(buffer, size, nmemb, out->stream);
}

int ftp_download_resumable(char *local_file, char *remote_file, 
		FTP_CURL_OPTIONS *option_p)
{
	CURL *curl;
	CURLcode res;
	long file_len = 0;
	double downloaded = 0;
	struct stat mystat;
	int ret;

	if(local_file == NULL || remote_file == NULL) {
		printf(LOGHEAD "download file cannot be empty.\n");
		return -1;
	}

	struct FtpFile ftpfile={
		local_file,
		NULL
	};

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if(curl) {
		// get the file size, for resuming the download
		bzero((void *)&mystat, sizeof(mystat));
		if(access(local_file, W_OK) == 0 && stat(local_file, &mystat) == 0 && mystat.st_size > 0) {
			file_len = mystat.st_size;
			printf(LOGHEAD "file %s exists, size %ld, using appending mode.\n", 
					local_file, file_len);
			ftpfile.stream = fopen(local_file, "ab+");
			fseek(ftpfile.stream, file_len, SEEK_SET);
		}

		curl_easy_setopt(curl, CURLOPT_RESUME_FROM, file_len);

		curl_easy_setopt(curl, CURLOPT_URL, remote_file);
		/* Define our callback to get called when there's data to be written */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ftp_curl_fwrite);
		/* Set a pointer to our struct to pass to the callback */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

		/* Switch on full protocol/debug output */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, (long)option_p->debug);

		// trace transfer progress
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &downloaded);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);

		// set timeout
		if (option_p->timeout)
			curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, option_p->timeout);

		res = curl_easy_perform(curl);

		// how many bytes downloaded ?
		option_p->bytes_done = downloaded;

		/* always cleanup */
		curl_easy_cleanup(curl);

		// get curl info
		if ( ret = get_curl_info(curl) ) {
			printf(LOGHEAD "error in getting curl info, ret is %d.\n", ret);
		} else {
			// return the aver speed
			option_p->speed = s_curl_info.speed_download;
			option_p->total_time = s_curl_info.total_time;
		}

		if(CURLE_OK != res) {
			/* we failed */
			printf(LOGHEAD "curl told us %d\n", res);
			curl_global_cleanup();
			return res;
		}
	} else {
		printf(LOGHEAD "cannot do the curl_easy_init().\n");
		return -1;
	}

	if(ftpfile.stream)
		fclose(ftpfile.stream); /* close the local file */

	curl_global_cleanup();

	return 0;
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	/* in real-world cases, this would probably get this data differently
	   as this fread() stuff is exactly what the library already would do
	   by default internally */
	size_t retcode = fread(ptr, size, nmemb, stream);

	printf(LOGHEAD "*** We read %d bytes from file\n", retcode);
	return retcode;
}

/* parse headers for Content-Length */
size_t getcontentlengthfunc(void *ptr, size_t size, size_t nmemb, void *stream) {
	int r;
	long len = 0;

	/* _snscanf() is Win32 specific */
	//r = _snscanf(ptr, size * nmemb, "Content-Length: %ld\n", &len);
	r = sscanf(ptr, "Content-Length: %ld\r\n", &len);

	if (r) /* Microsoft: we don't read the specs */ {
		// added by xzpeter for debugging
		printf(LOGHEAD "remote file length updated to %ld.\n", len);
		*((long *) stream) = len;
	}

	return size * nmemb;
}

int ftp_upload_resumable(char *remote_file, char *local_file, 
		FTP_CURL_OPTIONS *option_p)
{
	int ret;
	CURL *curl;
	CURLcode res;
	FILE *hd_src;
	struct stat file_info;
	curl_off_t uploaded_len = 0;
	curl_off_t fsize;
	double uploaded = 0;

	/* get the file size of the local file */
	if(stat(local_file, &file_info)) {
		printf(LOGHEAD "Couldnt open '%s': %s\n", local_file, strerror(errno));
		return -1;
	}
	fsize = (curl_off_t)file_info.st_size;

	printf(LOGHEAD "Local file size: %ld bytes.\n", (long)fsize);

	/* get a FILE * of the same file */
	hd_src = fopen(local_file, "rb");

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();
	if(curl) {
		// basic option settings
		// set the read function
		curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		// set the process function of the headers
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, getcontentlengthfunc);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &uploaded_len);
		// set the remote url
		curl_easy_setopt(curl,CURLOPT_URL, remote_file);
		// set verbose or not
		curl_easy_setopt(curl, CURLOPT_VERBOSE, (long)option_p->debug);
		// set timeout
		if (option_p->timeout)
			curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 
					option_p->timeout);

		// settings for the first attempt, to get the remote file size
		// add these 2 opt to send a SIZE cmd in order to get the remote file size
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		// do not upload for the first attempt
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 0L);

		// do the first attempt
		res = curl_easy_perform(curl);
		if(res != CURLE_OK) {
			printf(LOGHEAD "something wrong during curl_easy_perform()[1].\n");
			goto easy_curl_quit;
		}

		// now get the size of the remote file in uploaded_len
		if(fsize < uploaded_len) {
			// should not get in here ...
			printf(LOGHEAD "local file size is %ld, remote file size is %ld, something wrong.\n",
					(long)fsize, (long)uploaded_len);
			goto easy_curl_quit;
		}
		// maybe the file has been uploaded ok
		if(fsize == uploaded_len) {
			printf(LOGHEAD "remote file is equal size of the local file, maybe upload is done before.\n");
			goto easy_curl_quit;
		}
		fseek(hd_src, uploaded_len, SEEK_SET);
		printf(LOGHEAD "fseek to %ld now, preparing for resuming upload.\n", (long)uploaded_len);

		// then upload
		curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
		// set upload and append attribute
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_APPEND, 1L);

		// trace transfer progress
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &uploaded);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);

		res = curl_easy_perform(curl);
		// how many bytes uploaded ?
		option_p->bytes_done = uploaded;

		if(res != CURLE_OK) {
			printf(LOGHEAD "something wrong during curl_easy_perform()[2].\n");
			goto easy_curl_quit;
		}
		printf(LOGHEAD "transfer done.\n");

		// if success, get the upload speed
		if (get_curl_info(curl)) {
			printf(LOGHEAD "error to get_curl_info().");
		} else {
			option_p->speed = s_curl_info.speed_upload;
			option_p->total_time = s_curl_info.total_time;
		}
		/* always cleanup */
easy_curl_quit:
		curl_easy_cleanup(curl);
	} else {
		printf(LOGHEAD "cannot do the curl_easy_init().\n");
	}

	fclose(hd_src); /* close the local file */

	curl_global_cleanup();
	return res;
}

#define		FTP_UNLINK_STR		"DELE "

int ftp_unlink(char *remote_url, FTP_CURL_OPTIONS *options_p)
{
	CURL *curl;
	int ret, n;

	struct curl_slist *headerlist=NULL;
	char tmp_buf[BUF_LEN] = FTP_UNLINK_STR;
	char *pch;
	char *file_name;

	if (remote_url == NULL)
		return -1;

	file_name = strrchr(remote_url, '/');
	if (file_name == NULL)
		return -2;

	// copy the filename to the cmd list
	file_name++;
	strncat(tmp_buf, file_name, BUF_LEN - strlen(FTP_UNLINK_STR) - 1);

	// remove the filename from remote url
	*file_name = 0x00;

	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */ 
	curl = curl_easy_init();
	if(curl) {
		/* build a list of commands to pass to libcurl */ 
		headerlist = curl_slist_append(headerlist, tmp_buf);

		/* specify target */ 
		curl_easy_setopt(curl, CURLOPT_URL, remote_url);

		/* pass in that last of FTP commands to run after the transfer */ 
		curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);

		// set verbose or not
		curl_easy_setopt(curl, CURLOPT_VERBOSE, (long)options_p->debug);

		// set timeout
		if (options_p->timeout)
			curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 
					options_p->timeout);

		/* Now run off and do what you've been told! */ 
		ret = curl_easy_perform(curl);

		/* clean up the FTP commands list */ 
		curl_slist_free_all (headerlist);

		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return ret;
}
