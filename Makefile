all:
	gcc -o lws lws.c

install:
	cp lws /usr/local/bin/

uninstall:
	rm -- /usr/local/bin/lws
