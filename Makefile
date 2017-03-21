all: lws.o
	gcc -o lws lws.o

clean: lws.o
	rm -f lsw.o

cleanall: lws.o lws
	rm -f lws.o lws
