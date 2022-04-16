## About

Trying to learn C by doing. This is an attempt on a very basic http server that serves static files out of a directory.

## Usage

```sh
docker build -t c-program .

docker run \
-it \
--name c-program-dev \
-v "$(pwd)"/src:/usr/src/myapp/src \
-p 80:80 \
c-program bash

# For subsequent runs
docker start -i c-program-dev

make

./bin/startServer port=8080 dir=./website
```

## Todos

1. Parse header up to /r/n/r/n point and extract target uri and get static file location

2. Create struct to store connection data and set its pointer to epoll_event data \*ptr

- source file descriptor
- \*method
- \*uri
- \*header_field
  - key (strdup)
  - value (strdup)
- header_size
- \*number of bytes sent

3. Use sendfile to send data to client. Close connection once complete. If error EGAIN - continue, if zero means EOF

## References

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/)
- [Using poll() instead of select()](https://www.ibm.com/docs/en/i/7.4?topic=designs-using-poll-instead-select)
- [Blocking I/O, Nonblocking I/O, And Epoll](https://eklitzke.org/blocking-io-nonblocking-io-and-epoll)
- [LibHTTP – Open Source HTTP Library in C](https://github.com/lammertb/libhttp)
- [HTTP Server: Everything you need to know to Build a simple HTTP server from scratch](https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa)
- [HTTP Server in C](https://dev-notes.eu/2018/06/http-server-in-c/)
- https://gist.github.com/bdahlia/7826649
- https://gist.github.com/laobubu/d6d0e9beb934b60b2e552c2d03e1409e
- [epoll tutorial](https://suchprogramming.com/epoll-in-3-easy-steps/)

## MacOS syscalls

- [sendfile for osx]()
- [kqueue as epoll replacement](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/kqueue.2.html)
