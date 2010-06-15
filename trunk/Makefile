#!/usr/bin/make

CFLAGS = -O9 -x c -pipe -std=gnu99
LDFLAGS = -s
httpd: main.o webrequest.o webresponse.o webserver.o
	$(CC) -o httpd *.o	

clean:
	rm -f httpd *.o
