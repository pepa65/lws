#include "lws.h"

// get the upper directory of curpath and store in upperpath
int dirup(const char* curpath, char* upperpath) {
    size_t len = strlen(curpath);
    if(len >= MAXPATH) {
        errno = 36;
        return -1;
    }
    strncpy(upperpath,curpath,len+1);
    if(len > 1 && upperpath[len - 1] == '/') --len;
    while(len > 1 && upperpath[len] != '/') --len;
    upperpath[len] = '\0';
    errno = 0;
    return 0;
}

// listdir: list files under path realpath
// cskfile = client sock file
// realpath is a absolute path in the system
// path is a absolute path in the webserver
void listdir(FILE* cskfile, char* path, char* realpath) {
    DIR* dir;
    struct dirent* dent;
    struct stat filestat;
    dir = opendir(realpath);
    char filename[MAXBUFF];
    fprintf(cskfile,
            "HTTP/1.1 200 OK\r\nServer: %s\r\nConnection: close\r\n\r\n"
            "<html><head><meta http-equiv=Content-Type content=\"text/html;charset=utf-8\">"
            "<title>%s</title></head>\n"
            "<body><h1>%s</h1>"
            "<table cols=\"4\" width=\"96%%\">"
            "<tr><td>type</td><td>name</td><td>size</td><td>modified</td></tr>\n",
            SELF, SELF, realpath);
    if(!dir) {
        fprintf(cskfile, "</table><p class=\"error\">Error %s</p>"
                "</body></html>", strerror(errno));
        return;
    }
    while((dent = readdir(dir)) != NULL) {
        if(strcmp(path,"/") == 0) sprintf(filename,"/%s",dent->d_name);
        else {
            if(path[strlen(path) - 1] == '/')
                sprintf(filename,"%s%s",path,dent->d_name);
            else sprintf(filename,"%s/%s",path,dent->d_name);
        }
        fprintf(cskfile,"<tr>");
        size_t len = strlen(rootdir) + strlen(filename) + 1;
        char* realfilename = (char*)malloc(len);
        sprintf(realfilename,"%s%s",rootdir,filename);
        int ret = stat(realfilename,&filestat);
        if(ret) {
            puterrmsg("In function listdir()");
            return ;
        }
        if(strcmp(dent->d_name,".") == 0) continue;
        else {
            if(S_ISDIR(filestat.st_mode)) fprintf(cskfile,"<td>dir</td>");
            else if(S_ISREG(filestat.st_mode)) fprintf(cskfile,"<td>file</td>");
            else if(S_ISLNK(filestat.st_mode)) fprintf(cskfile, "<td>link</td>");
            else if (S_ISCHR(filestat.st_mode)) fprintf(cskfile, "<td>char</td>");
            else if (S_ISBLK(filestat.st_mode)) fprintf(cskfile, "<td>block</td>");
            else if (S_ISFIFO(filestat.st_mode)) fprintf(cskfile, "<td>fifo</td>");
            else if (S_ISSOCK(filestat.st_mode)) fprintf(cskfile, "<td>sock</td>");
            else fprintf(cskfile, "<td>?</td>");
            fprintf(cskfile,"<td><a href=\"//%s%s%s%s\">%s</a></td>",
                    host, filename, atoi(port) == 80 ? "" : ":",
                    atoi(port) == 80 ? "" : port, dent->d_name);
            #ifdef DEBUG
                printf("path: %s\n", filename);
            #endif
        }
        if(S_ISREG(filestat.st_mode))
            fprintf(cskfile,"<td class=\"size\">%u B</td>", (unsigned)filestat.st_size);
        else fprintf(cskfile,"<td></td>");
        fprintf(cskfile,"<td class=\"date\">%s</td></tr>\n",ctime(&filestat.st_ctime));
        free(realfilename);
    }
    fprintf(cskfile,"</table></body></html>");
}

// download file according to realpath
void getfile(FILE *cskfile, char* realpath) {
    int fd = open(realpath,O_RDONLY);
    int len = lseek(fd,0,SEEK_END);
    char* buf = (char*)malloc(len + 1);
    bzero(buf,len+1);
    lseek(fd,0,SEEK_SET);
    read(fd,buf,len);
    close(fd);
    fprintf(cskfile, "HTTP/1.1 200 OK\r\nServer: %s\r\n"
            "Connection: keep-alive\r\n"
            "Content-type: application/*\r\n"
            "Content-Length:%d\r\n\r\n", SELF, len);
    fwrite(buf,len,1,cskfile);
    free(buf);
}

// response to requests from clients
void serverResponse(FILE* cskfile, char* path) {
    struct stat filestat;
    char* realpath;
    int len = strlen(rootdir) + strlen(path);
    realpath = (char*)malloc(len + 1);
    bzero(realpath, len+1);
    sprintf(realpath,"%s%s",rootdir,path);
    if(stat(realpath, &filestat)) {
        fprintf(cskfile,"HTTP/1.1 200 OK\r\nServer: %s\r\n"
                "Connection: close\r\n\r\n"
                "<html><head><meta http-equiv=Content-Type content=\"text/html;charset=utf-8\">"
                "<title>%s - %d</title></head>"
                "<body><h1>lws</h1>"
                "<p class=\"error\">%d %s %s</p>"
                "</body></html>",
                SELF, SELF, errno, errno, strerror(errno), path);
    }
    else {
        // regular file to download
        if(S_ISREG(filestat.st_mode)) getfile(cskfile,realpath);
        // directory to list
        else if(S_ISDIR(filestat.st_mode)) listdir(cskfile,path,realpath);
        // neither file nor directory
        else fprintf(cskfile,
                    "HTTP/1.1 200 OK\r\nServer: %s\r\n"
                    "Connection: close\r\n\r\n"
                    "<html><head><meta http-equiv=Content-Type content=\"text/html;charset=utf-8\">"
                    "<title>%s - denied</title></head>"
                    "<body><h1>lws</h1>"
                    "<p class=\"error\">'%s' is inaccessible</p>"
                    "</body></html>", SELF, SELF, path);
    }
    free(realpath);
    return ;
}

// commandline options
// --host|-h  IP address to listen on
// --port|-p  port to listen on
// --back|-b  number of listeners
// --dir|-d  base directory to serve
// --log|-l  log file location
// --fg|-f  force operation in the foreground
void getoption(int argc, char** argv) {
    int c;
    char* buf = NULL;
    opterr = 0;
    struct option long_opts[] = {
        {"host", 1, 0, 0},
        {"port", 1, 0, 0},
        {"back", 1, 0, 0},
        {"dir", 1, 0, 0},
        {"log", 1, 0, 0},
        {"fg", 1, 0, 0},
        {0, 0, 0, 0}
    };
    size_t len;
    while(1) {
        int long_ind;
        c = getopt_long(argc, argv, "h:p:b:d:l:f", long_opts, &long_ind);
        if(c == -1 || c == '?') break;
        if(optarg) len = strlen(optarg);
        else len = 0;
        if((!c && !strcasecmp(long_opts[long_ind].name,"host")) || (c == 'h'))
            buf = host = (char*) malloc(len + 1);
        else if((!c && !strcasecmp(long_opts[long_ind].name,"port")) || (c == 'p'))
            buf = port = (char*)malloc(len + 1);
        else if((!c && !strcasecmp(long_opts[long_ind].name,"back")) || (c == 'b'))
            buf = backlog = (char*)malloc(len + 1);
        else if((!c && !strcasecmp(long_opts[long_ind].name,"dir")) || (c == 'd'))
            buf = rootdir = (char*)malloc(len + 1);
        else if((!c && !strcasecmp(long_opts[long_ind].name,"log")) || (c == 'l'))
            buf = logfile = (char*)malloc(len + 1);
        else if((!c && !strcasecmp(long_opts[long_ind].name,"fg"))) isdaemon = 0;
        else break;
        bzero(buf, len + 1);
        memcpy(buf, optarg, len);
    }
}

// get the config infomation from the command line input
void myinit(int argc, char** argv) {
    getoption(argc, argv);
    if(!host) allocMemory(&host, DEFAULTIP, strlen(DEFAULTIP));
    if(!port) allocMemory(&port, DEFAULTPORT, strlen(DEFAULTPORT));
    if(!backlog) allocMemory(&backlog, DEFAULTBACK, strlen(DEFAULTBACK));
    if(!rootdir) allocMemory(&rootdir, DEFAULTDIR, strlen(DEFAULTDIR));
    if(!logfile) allocMemory(&logfile, DEFAULTLOG, strlen(DEFAULTLOG));
    printf("host: %s\nport: %s\nqsize: %s\ndir: %s\nlog: %s\npid: %d %s\n",
             host, port, backlog, rootdir, logfile, getpid(), isdaemon?"(daemon)":"");
}

//convert the special uri code in the given path to normal characters
void uri_decode(char *path) {
    int len = strlen(path);
    char buf[MAXPATH + 1];
    bzero(buf, MAXPATH + 1);
    int high,low;
    #define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')
    int ind = 0;
    int i;
    for(i = 0; i < len; i++) {
        switch(path[i]) {
        case '%':
            if(isxdigit(((unsigned char*)path)[i+1])
                    && isxdigit(((unsigned char*)path)[i+2])) {
                high = tolower(((unsigned char*)path)[i+1]);
                low = tolower(((unsigned char*)path)[i+2]);
            }
            buf[ind++] = (HEXTOI(high) << 4) | HEXTOI(low);
            i += 2;
            break;
        default:
            buf[ind++] = path[i];
            break;
        }
    }
    buf[ind++] = '\0';
    bzero(path, len + 1);
    strncpy(path, buf, ind);
}

void puterrmsg(char* msg) {
    if (!isdaemon){
        errmsgprt(msg);
    }
    else {
       errmsgwrt(msg);
    }
}

void putinfomsg(char* msg) {
    if (!isdaemon){
        infomsgprt(msg);
    }
    else {
       infomsgwrt(msg);
    }
}

void allocMemory(char** dest, char* src, size_t len) {
    *dest = (char*)malloc(len + 1);
    bzero(*dest,len + 1);
    memcpy(*dest, src, len);
}

void run() {
    struct sockaddr_in addr;
    struct sockaddr_in client_addr;
    int sock_fd;
    socklen_t addrlen;

    // abort the main process to make child process a daemon
    if(isdaemon) {
        if(fork()) exit(EXIT_SUCCESS);
        if(fork()) exit(EXIT_SUCCESS);
        close(0),close(1),close(2);
        logfp = fopen(logfile,"a+");
        if(!logfp) exit(EXIT_FAILURE);
    }
    // ignore the SIGCHLD signal explicitly to avoid zombie
    signal(SIGCHLD, SIG_IGN);
    sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0) puterrmsg("socket()");

    // set the addr info reusable
    int value = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

    // fill the addr info
    addr.sin_family = PF_INET;
    inet_pton(PF_INET, host, &addr.sin_addr);
    addr.sin_port = htons(atoi(port));

    addrlen = sizeof(struct sockaddr);
    if(bind(sock_fd, (struct sockaddr*)&addr, addrlen) < 0) puterrmsg("bind()");

    if(listen(sock_fd, atoi(backlog)) < 0) puterrmsg("listen()");

    while(1) {
        int new_fd;
        addrlen = sizeof(struct sockaddr_in);
        new_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &addrlen);
        if(new_fd < 0) {
            puterrmsg("accept()");
            break;
        }
        bzero(buffer, MAXBUFF + 1);
        sprintf(buffer,"Connection from %s:%d\r\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        putinfomsg(buffer);
        if(!fork()) {
            bzero(buffer, MAXBUFF+1);
            int len = recv(new_fd, buffer, MAXBUFF, 0);
            if(len > 0) {
                FILE *fp = fdopen(new_fd,"w");
                if(!fp)
                    puterrmsg("fdopen()");
                char reqstr[MAXPATH + 1];
                sscanf(buffer, "GET %s HTTP", reqstr);
                bzero(buffer, MAXBUFF + 1);
                if(reqstr[strlen(reqstr) - 1] == '/')
                    reqstr[strlen(reqstr) - 1] = '\0';
                uri_decode(reqstr);
                sprintf(buffer, "Request to get file :%s\r\n", reqstr);
                putinfomsg(buffer);
                serverResponse(fp, reqstr);

                #ifdef MYDEBUG
                    printf("reqpath: %s\n",reqstr);
                #endif

                fclose(fp);
            }
            exit(EXIT_SUCCESS);
        }
        close(new_fd);
    }
    close(sock_fd);
}

int main(int argc, char **argv) {
    myinit(argc, argv);
	  run();
    return 0;
}
