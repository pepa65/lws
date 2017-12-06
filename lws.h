// lws.h

#include <stdio.h>
#include <stdarg.h>
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
#include <errno.h>

#define SELF "lws"
#define TAGLINE " - Simple http server listing directories and downloading files"
#define URL "//github.com/pepa65/lws"
#define DEFAULT_ADDRESS "localhost"
#define DEFAULT_PORT "8090"
#define DEFAULT_BACKLOG "10"
#define DEFAULT_ROOTDIR "."
#define DEFAULT_LOGFILE SELF ".log"
#define DEFAULT_DEBUG 0

#define MAXBUFF 102400
#define MAXPATH 511

char buffer[MAXBUFF + 1];
char* address = DEFAULT_ADDRESS;
char* port = DEFAULT_PORT;
char* backlog = DEFAULT_BACKLOG;
char* rootdir = DEFAULT_ROOTDIR;
char* logfile = DEFAULT_LOGFILE;
unsigned char isdaemon = 1;
unsigned char debug = DEFAULT_DEBUG;
unsigned char p80;
FILE* lfp;

#define PROT "HTTP/1.1 200 OK\r\nServer: " SELF "\r\n"
#define CLOSE "Connection: close\r\n\r\n"
#define DOWNLOAD "Connection: keep-alive\r\n\
Content-type: application/*\r\n\
Content-Length: %d\r\n\r\n"

#define CSS "body{font-family:Arial,sans-serif; background-color:#eee;} \
.foot{font-style:italic; font-size:70%%; color:#666; margin: 4px 0 600px;} \
a,a:active{text-decoration:none; color:#116;} \
a:visited{color:#416;} \
a:hover,a:focus{text-decoration:none;} \
h1{color:#000; font-size:160%%;} \
h2{margin-bottom:12px;} \
h4{color:#333;} \
.g{color:#666} \
.s,.s{text-align:right;} \
.list{background-color:#fff; border-top:1px solid #666; \
 border-bottom:1px solid #666; padding:8px 12px 12px 12px;} \
table{width:100%%;} \
th,td{font-size:80%%; text-align:left; overflow:hidden;} \
th{padding-right:14px; padding-bottom:3px;} \
td{padding-right:14px; padding-left:5px;} \
td a{display:block; margin:-10em; padding:10em;} \
td a:hover{background-color:#ffd;} \
.b{font-weight:bold;} \
.i{font-style:italic;} \
.top{background-color:#bbb; font-style:italic;} \
.dark{background-color:#ddd;} \
.light{background-color:#fff;} \
.error{font-size:200%%; color:#f00;} \
.type{font-size:70%%; width:2%%;} \
.size{font:80%% monospace; width:13%%;} \
.date{font:70%% monospace; width:30%%;} \
.root{color:#666;}"

#define HTML "<html><head><meta http-equiv=\"pragma\" content=\"no-cache\" />\
<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\">\
<style>" CSS "</style><title>" SELF

#define BODY2 "</title></head>\n<body><a href=\"//%s:%s\" title=\"root directory\"><h1>"

#define DIV1 "root <span class=\"g\">%s</span></h1></a><div class=\"list\">\n"

#define FOOTER "</div><a href=\"" URL "\" title=\"" SELF \
" github page\" target=\"_blank\"><p class=\"foot\">" SELF TAGLINE "</p></a></body></html>"

extern int isdigit(int c);
extern int isxdigit(int c);
extern int tolower(int c);
extern char* get_current_dir_name(void);
