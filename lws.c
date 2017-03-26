// lws.c

#include "lws.h"

// output message; s=0: as-is, s=1: info, s=2: error, s=3: debug, s=4: exit
void msg(int s, const char* format, ...) {
	if(s == 4) lfp = stdout;
	va_list args;
	va_start(args, format);
	va_end(args);
	if(s == 1) fprintf(lfp, "Info: ");
	if(s == 2) fprintf(lfp, "Error: ");
	if(s == 3 && debug) fprintf(lfp, "Debug: ");
	if(s == 4) fprintf(lfp, "Exit: ");
	if(s != 3 || debug) {
		vfprintf(lfp, format, args);
		if(s) fprintf(lfp, "\n");
	}
	if(s == 4) exit(EXIT_FAILURE);
}

// convert %hh to characters
void uri_decode(char* string) {
	if (string == NULL) return;
	msg(3, "encoded:%s", string);
	char* i = string;
	char hex[3];
	hex[2] = 0;

	while ((string=strstr(string,"%"))!=NULL) {
		hex[0] = string[1];
		hex[1] = string[2];
		memmove(string+1, string+3, strlen(string+3)+1);
		string[0] = strtol(hex, NULL, 16);
		string++;
	}
}

// encoding characters in name to %hh in encoded (malloc, pass len_alloc!)
static void href_encode(char* name, char* encoded, int len_alloc) {
	encoded[0]=0;
	if(name == NULL) return;

	char* code = " \"#&\\<?:%>";
	size_t len_name = strlen(name);
	size_t len_code = strlen(code);
	size_t len_encoded = 0;
	size_t inc;
	char* format;

	for(size_t i = 0; i < len_name; i++) { // Check every character of name
		format = "%c";
		inc = 1;
		for(size_t j = 0; j < len_code; j++) // Encode every character of code
			if(name[i] == code[j]) {
				format = "%%%02x";
				inc = 3;
				break;
			}
		if(len_encoded+inc >= len_alloc) return;
		len_encoded += sprintf(encoded+len_encoded, format, name[i]);
	}
	msg(3, "uri:%s", encoded);
}

// encoding characters in name to &code; in encoded (malloc, pass len_alloc!)
static void html_encode(char* name, char* encoded, int len_alloc) {
	encoded[0] = 0;
	if(name == NULL) return;
	size_t len = 0;
	size_t inc;
	char* code;
	char ni[2] = "X";

	for(size_t i = 0; i < strlen(name); i++) {
		ni[0] = name[i];
		code = ni;
		inc = 1;
		switch(name[i]) {
		case '&':
			code = "&amp;";
			inc = 5;
			break;
		case '<':
			code = "&lt;";
			inc = 4;
			break;
		case '>':
			code = "&gt;";
			inc = 4;
			break;
		case ' ':
			code = "&nbsp;";
			inc = 6;
			break;
		}
		if(len+inc >= len_alloc) return;
		strcpy(encoded+len, code);
		len += inc;
	}
	msg(3, "name:%s\nhtml:%s\n", name, encoded);
}


void allocMemory(char** dest, char* src, size_t len) {
	*dest = (char*)malloc(len+1);
	memcpy(*dest, src, len);
	*dest[len+1] = 0;
}

// get the upper directory of curpath and store in upperpath
int dirup(const char* curpath, char* upperpath) {
	size_t len = strlen(curpath);
	if(len >= MAXPATH) {
		errno = 36;
		return -1;
	}
	strncpy(upperpath, curpath, len+1);
	if(len > 1 && upperpath[len-1] == '/') --len;
	while(len > 1 && upperpath[len] != '/') --len;
	upperpath[len] = 0;
	errno = 0;
	return 0;
}

// listdir: list files under path fspath
//  cskfile: client sock file
//  path: absolute path on webserver
//  fspath: absolute path on the filesystem
void listdir(FILE* cskfile, char* path, char* fspath) {
	DIR* dir;
	struct dirent* dent;
	struct stat filestat;
	dir = opendir(fspath);
	char filename[MAXBUFF];
	int linecolor = 0;
	unsigned long long int size;
	char* measure;
	char* href=malloc(MAXBUFF);
	char* html=malloc(MAXBUFF);
	if(!dir) {
		fprintf(cskfile, HTTP CLOSE HTML " error" BODY2 " error" DIV
				"<p class=\"error\">Error %d - %s</p>" FOOTER,
				address, port, errno, strerror(errno));
		msg(3, "error:%d - %s", errno, strerror(errno));
		return;
	}
	else fprintf(cskfile, HTTP CLOSE HTML BODY2 DIV
			"<table cols=\"4\"><tr class=\"top\">"
			"<td>type</td><td>name</td><td>size</td><td>modified</td></tr>\n",
			address, port);
	while((dent = readdir(dir)) != NULL) {
		if(strcmp(path, "/") == 0) sprintf(filename, "/%s", dent->d_name);
		else {
			if(path[strlen(path)-1] == '/')
				sprintf(filename, "%s%s", path, dent->d_name);
			else sprintf(filename, "%s/%s", path, dent->d_name);
		}
		fprintf(cskfile, "<tr class=\"%s\">", linecolor ? "dark" : "light");
		size_t len = strlen(rootdir)+strlen(filename)+1;
		char* realfilename = (char*)malloc(len);
		sprintf(realfilename, "%s%s", rootdir, filename);
		int ret = stat(realfilename, &filestat);
		if(ret) { // not OK: should never happen
			msg(2, "stat failed, error %d - %s", errno, strerror(errno));
			return;
		}
		if(strcmp(dent->d_name, ".") == 0) continue;
		else {
			fprintf(cskfile, "<td class=\"type\">");
			if(S_ISDIR(filestat.st_mode)) fprintf(cskfile, "dir</td>");
			else if(S_ISREG(filestat.st_mode)) fprintf(cskfile, "file</td>");
			else if(S_ISLNK(filestat.st_mode)) fprintf(cskfile, "link</td>");
			else if(S_ISCHR(filestat.st_mode)) fprintf(cskfile, "char</td>");
			else if(S_ISBLK(filestat.st_mode)) fprintf(cskfile, "block</td>");
			else if(S_ISFIFO(filestat.st_mode)) fprintf(cskfile, "fifo</td>");
			else if(S_ISSOCK(filestat.st_mode)) fprintf(cskfile, "sock</td>");
			else fprintf(cskfile, "?</td>");
			href_encode(filename, href, MAXBUFF);
			html_encode(dent->d_name, html, MAXBUFF);
			fprintf(cskfile, "<td><a href=\"//%s%s%s%s\">%s</a></td>",
					address, p80 ? "" : ":", p80 ? "" : port, href, html);
		}
		if(S_ISREG(filestat.st_mode)) {
			size = (unsigned)filestat.st_size;
			if(size < 1024) measure = " B";
			else if((size=size/1024) < 1024) measure=" KiB";
			else if((size=size/1024) < 1024) measure=" MiB";
			else { size=size / 1024; measure=" GiB";}
			fprintf(cskfile,
					"<td class=\"size\">%llu %s</td>", size, measure);
		}
		else fprintf(cskfile, "<td></td>");
		fprintf(cskfile,
				"<td class=\"date\">%s</td></tr>", ctime(&filestat.st_mtime));
		linecolor=!linecolor;
		free(realfilename);
	}
	fprintf(cskfile, "</table>" FOOTER);
}

// download file according to fspath
void getfile(FILE* cskfile, char* fspath) {
	int fd = open(fspath, O_RDONLY);
	int len = lseek(fd, 0, SEEK_END);
	char* buf = (char*)malloc(len);
	lseek(fd, 0, SEEK_SET);
	read(fd, buf, len);
	close(fd);
	fprintf(cskfile, HTTP DOWNLOAD, len);
	fwrite(buf, len, 1, cskfile);
	free(buf);
}

// response to requests from clients
void serverResponse(FILE* cskfile, char* path) {
	struct stat filestat;
	char* fspath;
	int len = strlen(rootdir)+strlen(path);
	fspath = (char*)malloc(len+1);
	sprintf(fspath, "%s%s", rootdir, path);
	fspath[len+1] = 0;
	msg(3, "fspath:%s", fspath);
	if(stat(fspath, &filestat)) {
		fprintf(cskfile, HTTP CLOSE HTML " error" BODY2 " error" DIV
				"<p class=\"error\">Error %d - %s</p>%s" FOOTER,
				address, port, errno, strerror(errno), fspath);
		msg(3, "error:%d - %s: %s", errno, strerror(errno), fspath);
	}
	else {
		// regular file to download
		if(S_ISREG(filestat.st_mode)) getfile(cskfile, fspath);
		// directory to list
		else if(S_ISDIR(filestat.st_mode)) listdir(cskfile, path, fspath);
		// neither file nor directory
		else {
			fprintf(cskfile, HTTP CLOSE HTML " denied" BODY2 " denied" DIV
					"<p class=\"error\">Path inaccessible</p>%s" FOOTER,
					address, port, path);
			msg(3, "denied:%s", path);
		}
	}
	free(fspath);
	return ;
}

void helptext() {
	msg(0, " %s%s\n"
			" Homepage: http:%s\n\n"
			" USAGE: %s [option [argument]]...\n"
			" Options:\n"
			"   -a, --address <address>    address to listen on (default %s)\n"
			"   -p, --port <port>          port to listen on (default %s)\n"
			"   -b, --backlog <number>     number of listeners (default %s)\n"
			"   -r, --rootdir <rootdir>    base directory to serve (default %s)\n"
			"   -l, --logfile [<logfile>]  logfile location (default %s)\n"
			"   -d, --debug                extra output for debugging (default %s)\n"
			"   -f, --foreground           no forking to the background\n"
			"   -h, --help                 display this help text\n",
			SELF, TAGLINE, URL, SELF, DEFAULT_ADDRESS, DEFAULT_PORT, DEFAULT_BACKLOG,
			strcmp(DEFAULT_ROOTDIR, ".") ? DEFAULT_ROOTDIR : "current",
			DEFAULT_LOGFILE, DEFAULT_DEBUG ? "on" : "off");
	exit(EXIT_SUCCESS);
}

// get the config infomation from the command line input
void getoption(int argc, char** argv) {
	int c;
	char* buf;
	opterr = 0; // don't output errors
	lfp = stdout;
	struct option long_opts[] = {
		// option-name, has-argument, flag, return-value
		{"address", required_argument, 0, 'a'},
		{"port", required_argument, 0, 'p'},
		{"backlog", required_argument, 0, 'b'},
		{"rootdir", required_argument, 0, 'r'},
		{"logfile", optional_argument, 0, 'l'},
		{"debug", no_argument, 0, 'd'},
		{"foreground", no_argument, 0, 'f'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	char* short_opts = ":a:p:b:r:l::dfh";
	size_t len;
	unsigned char log_set = 0;

	while(1) {
		c = getopt_long(argc, argv, short_opts, long_opts, 0);
		if(c == -1) break; // end of commandline processing
		if(c == ':') msg(4, "missing argument to commandline option %c", optopt);
		if(c == 0 || c == '?') msg(4, "unknown commandline option: %c", optopt);
		if(c == 'd') { debug = 1; continue;}
		if(c == 'f') { isdaemon = 0; continue;}
		if(c == 'h') { helptext(); continue;}
		if(c == 'l') {
			log_set = 1;
			if(!optarg) continue;
		}
		if(!optarg) msg(4, "argument required for option %c", optopt);
		else {
			len = strlen(optarg);
			buf = (char*)malloc(len+1);
			memcpy(buf, optarg, len);
			buf[len] = 0;
			if(c == 'a') address = buf;
			else if(c == 'p') port = buf;
			else if(c == 'b') backlog = buf;
			else if(c == 'l') logfile = buf;
			else if(c == 'r') rootdir = buf;
			msg(3, "option:%c:%s", c, optarg);
		}
	}
	if(!log_set) logfile = NULL;
	p80 = atoi(port) == 80;
}

void catch_exit(int signo) {
	msg(4, "%s killed, signal %d", SELF, signo);
}

void run() {
	struct sockaddr_in addr;
	struct sockaddr_in client_addr;
	int sock_fd;
	socklen_t addrlen;
	pid_t pid = getpid();

	// abort the main process to make child process a daemon
	if(isdaemon) {
		if(pid = fork()) exit(EXIT_SUCCESS);
		else msg(4, "fork failed");
		signal(SIGTERM, catch_exit);
	}

	if(logfile) {
		msg(1, "logfile %s in use, no more stdout", logfile);
		lfp = fopen(logfile, "a+");
		if(!lfp) msg(4, "could not fork as daemon");
	}
	time_t t;
	time(&t);
	msg(0, "\ndate: %s%s serving %s on http://%s:%s\n"
			"backlog: %s, log: %s, debug: %s, pid: %d %s\n", ctime(&t), SELF,
			strcmp(rootdir, ".") ? rootdir : get_current_dir_name(),
			address, port, backlog, logfile ? logfile : "none", debug ? "on" : "off",
			pid, isdaemon ? "(daemon)" : "(foreground)");

	sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	if(sock_fd < 0) msg(4, "can't open socket");

	// set the addr info reusable
	int value = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

	// fill the addr info
	addr.sin_family = PF_INET;
	inet_pton(PF_INET, address, &addr.sin_addr);
	addr.sin_port = htons(atoi(port));
	addrlen = sizeof(struct sockaddr);
	if(bind(sock_fd, (struct sockaddr*)&addr, addrlen) < 0)
		msg(4, "can't bind on %s:%s", address, port);
	if(listen(sock_fd, atoi(backlog)) < 0)
		msg(4, "can't listen on socket %d, error %d - %s", sock_fd, errno, strerror(errno));

	while(1) {
		int new_fd;
		addrlen = sizeof(struct sockaddr_in);
		msg(3, "1addrlen:%d",addrlen);
		new_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &addrlen);
		if(new_fd < 0) msg(4, "can't accept from fd %d", sock_fd);
		msg(3, "2addrlen:%d",addrlen);
		sprintf(buffer, "connection from %s:%d",
				inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		msg(3, "buffer:%s", buffer);
		if(!fork()) {
			int len = recv(new_fd, buffer, MAXBUFF, 0);
			if(len > 0) {
				msg(3, "length:%d: %s", len, buffer);
				FILE* fp = fdopen(new_fd, "w");
				if(!fp) msg(4, "can't open file for writing");
				char reqstr[MAXPATH+1];
				sscanf(buffer, "GET %s HTTP", reqstr);
				if(reqstr[strlen(reqstr)-1] == '/') reqstr[strlen(reqstr)-1] = 0;
				uri_decode(reqstr);
				msg(3, "requesting:%s", reqstr);
				serverResponse(fp, reqstr);
				fclose(fp);
			}
			else if(len < 0) msg(3, "zero bytes received on socket");
			else msg(2, "%d - %s", errno, strerror(errno));
			exit(EXIT_SUCCESS);
		}
		close(new_fd);
	}
	close(sock_fd);
}

int main(int argc, char** argv) {
	getoption(argc, argv);
	run();
	return 0;
}
