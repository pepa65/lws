#ifndef GLOBALVAR_H_INCLUDED
#define GLOBALVAR_H_INCLUDED
#include<stdio.h>

#define SELF "lws"
#define DEFAULTIP "localhost"
#define DEFAULTPORT "8090"
#define DEFAULTBACK "10"
#define DEFAULTDIR "."
#define DEFAULTLOG SELF ".log"
#define MAXBUFF 102400
#define MAXPATH 511

char buffer[MAXBUFF + 1];
char* host = NULL;
char* port = NULL;
char* backlog = NULL;
char* rootdir = NULL;
char* logfile = NULL;
unsigned char isdaemon = 1;
FILE* logfp;

#endif // GLOBALVAR_H_INCLUDED

#ifndef HEADFILE_H_INCLUDED
#define HEADFILE_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <time.h>

//#define MYDEBUG

#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED
#include <errno.h>
#define errmsgprt(msg){ perror(msg); abort();}
#define errmsgwrt(msg){ fputs(msg,logfp); fputs(strerror(errno),logfp); fflush(logfp); abort();}
extern void puterrmsg(char* msg);
#endif // ERRORS_H_INCLUDED

#ifndef INFOMSG_H_INCLUDED
#define INFOMSG_H_INCLUDED
#define infomsgprt(msg) { fputs(msg,stdout);}
#define infomsgwrt(msg) { fputs(msg,logfp); fflush(logfp);}
extern void putinfomsg(char* msg);
#endif // INFOMSG_H_INCLUDED

#ifndef MEMMANAGER_H_INCLUDED
#define MEMMANAGER_H_INCLUDED
#include <string.h>
extern void allocMemory(char** dest, char* src, size_t len);
#endif // MEMMANAGER_H_INCLUDED

#endif // HEADFILE_H_INCLUDED

#ifndef MAJORWORK_H_INCLUDED
#define MAJORWORK_H_INCLUDED

#include<stdio.h>
// get the upper directory of curpath and store in upperpath
extern int dirup(const char* curpath, char* upperpath);

// listdir: list files under path realpath
// cskfile = client sock file
// realpath is a absolute path in the system
// path is a absolute path in the webserver
extern void listdir(FILE* cskfile, char* path, char* realpath);

// download file according to realpath
extern void getfile(FILE *cskfile, char* realpath);

// response to requests from clients
extern void serverResponse(FILE* cskfile, char* path);

// --host|-H --port|-P --back|-B --dir|-D --log|-L --daemon
extern void getoption(int argc, char** argv);

// get the config infomation from the command line input
extern void myinit(int argc, char** argv);
extern void uri_decode(char *path);
extern void run();

// gnulib functions
extern int isdigit(int c);
extern int isxdigit(int c);
extern int tolower(int c);

#endif // MAJORWORK_H_INCLUDED
