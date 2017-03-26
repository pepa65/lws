# lws
**Light web server**

Simple http server listing directories and downloading files

```
USAGE: lws [option [argument]]...
 Options:
   -a, --address <address>    address to listen on (default localhost)
   -p, --port <port>          port to listen on (default 8090)
   -b, --backlog <number>     number of listeners (default 10)
   -r, --rootdir <rootdir>    base directory to serve (default current)
   -l, --logfile [<logfile>]  logfile location (default lws.log)
   -d, --debug                extra output for debugging (default off)
   -f, --foreground           no forking to the background
   -h, --help                 display this help text
```
