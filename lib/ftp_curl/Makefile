#cc_prefix = arm-linux-
cc = $(cc_prefix)gcc
ldflags = -lcurl -Wall -Werror # -L/home/peter/workspace/lib 
cflags = -g # -I/home/peter/workspace/include

default : ftp_curl.o curl_test

ftp_curl.o : ftp_curl.c ftp_curl.h
	$(cc) $(cflags) -c $<

curl_test : ftp_curl.o curl_test.c
	$(cc) $(cflags) $(ldflags) -o $@ $^

clean :
	@rm -rf *.o curl_test cscope.* 
