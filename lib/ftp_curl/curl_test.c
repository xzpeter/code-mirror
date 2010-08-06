#include <stdio.h>
#include <string.h>
#include "ftp_curl.h"
#include <curl/curl.h>

int main(int c, char **argv)
{
	int res;
	FTP_CURL_OPTIONS options;
	// test unlink
	init_curl_options(&options, 30, 1);
	res = ftp_unlink(argv[1], &options);
	printf("return is %d.\n", res);
	return 0;
// 	printf("CURLcode\texpression\n");
// 	for(i = 0; i <= CURL_LAST; i++)
// 		printf("%d\t\t%s\n", i, ftp_curl_error_str(i));
// 	return 0;

// 	if(c != 3) {
// 		printf("usage : %s localfile ftp://user:pass@host/remotefile\n", argv[0]);
// 		return -1;
// 	}
// 
// 	char *local_file;
// 	char *remote_file;
// 	local_file = argv[1];
// 	remote_file = argv[2];
// 	FTP_CURL_OPTIONS options;
// 	options.debug = 1;
// 	options.timeout = 30;
// // 	int ret = ftp_upload_resumable(remote_file, local_file, &options);
// 	int ret = ftp_download_resumable(local_file, remote_file, &options);
// 	if(ret) {
// 		printf("upload error, ret = %d\n", ret);
// 		return -1;
// 	}
// 	printf("file trans ok, speed is %f, %f bytes transfered.\n", 
// 			options.speed, options.bytes_done);
// 	return 0;
}
