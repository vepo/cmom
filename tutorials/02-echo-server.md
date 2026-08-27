# 02 – Create Async Echo Server

Now we have a working C project using autotools. We want to start a server and read/write request. The simpler server we can build is the Echo Server. Echo Server are very simple, we only send back the same request we receive! It looks simple, but we have a RFC for that [RFC-862](https://datatracker.ietf.org/doc/html/rfc862) submited in 1983. It's older than me by some months! If you read the RFC, you will see that's simple! Don´t read! 😉 I show the RFC to explain that even a very simple server has a specification.

## Architecture details

Before we start working on it, we need to talk about the server design. For this server, I don´t want to use synchronous I/O, I want async. In C, we read data from any source looks like reading a file. There is no difference between a socket, a file or other sources. All have a number as descriptor, a file descriptor and we should use this descriptor in the functions [read](https://man7.org/linux/man-pages/man2/read.2.html) and [write](https://man7.org/linux/man-pages/man2/write.2.html). 

You can see that I point to [man](https://man7.org/linux/man-pages/index.html). Man is the Linux manual, you can access it directly in your console, just type `man` or `man <something>`. Check more on [Julia Evans drawing](https://drawings.jvns.ca/man/).

![](/tutorials/man.png)

The biggest challange is that read and write are sync and when switch than to async, we will need to find a way to listen when the data is read. I found the solution on the blog post ["How Async I/O Actually Works: From read() to C++ co_await"](https://olafuraron.is/blog/async-io-explained/). We will use [fcntl](https://man7.org/linux/man-pages/man2/fcntl.2.html) system call and [epoll](https://man7.org/linux/man-pages/man7/epoll.7.html) notifications for that!

### Why using Non-blocking I/O?

The traditional way of reading data is blocking the application thread. When the program call `read`, the code will delegate the execution to the operating system and only return when all data is read. The operating system will not give back the execution and the thread will be blocked. This is an issue because threads are system resources and they are a limited resource.

If we use non-blocking I/O, we can accept more requests and do not lose time on blocked threads.

### Non-blocking I/O

Non-blocking I/O can be use by setting the flag `O_NONBLOCK` on the file descriptor. But if we enable this flag and do not change the way we read data we will have a higher CPU usage, as the program will start to call `read` and `write` continuously. 

```c
void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}
```

### Using `epoll`

epoll is a file descriptor notification service. We can add files descripts to it and wait for events. The main thread will be blocked only if there is no network activity. Internally `epoll` works with two list. The `interest` with all file descriptors that have added and `ready` with file descriptors ready to be read.

Fun fact is that the `epoll` is also a file descriptor. Everything in Linux is a file descriptor. 

We can control `epoll` with the functions `epoll_create`, `epoll_ctl` and `epoll_wait`.

## Get everyting together

1. Listen to a TCP port
   ```c
   // 1. Creates the Socket and using IPv4 AF_INET and TCP SOCK_STREAM
   // DOC: https://man7.org/linux/man-pages/man2/socket.2.html
   int server = socket(AF_INET, SOCK_STREAM, 0); 
   
   // 2. Set socket options. Using TCP
   // DOC: https://man7.org/linux/man-pages/man2/setsockopt.2.html
   int opt = 1;
   setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
   // 3. Bind the socket
   struct sockaddr_in addr = { .sin_family = AF_INET,        // Use IPv4 https://man7.org/linux/man-pages/man7/address_families.7.html
                               .sin_port = htons(8080),      // Use port 8080
                               .sin_addr.s_addr = INADDR_ANY // Accept from any address. 
                                                               // Other options are INADDR_LOOPBACK   127.0.0.1
                                                               //                   INADDR_ANY        0.0.0.0
                                                               //                   INADDR_BROADCAST  255.255.255.255
                               };
   bind(server, (struct sockaddr*)&addr, sizeof(addr));
   // 4. Set file descriptor to non_blocking
   set_nonblocking(server);
   // 5. Mark the socket as passive accepting at most 128 connections in queue
   // DOC: https://man7.org/linux/man-pages/man2/listen.2.html
   listen(server, 128);
   ```
2. Create e `epoll` instance and add socket file descript to it.
   ```c
   // Start epoll. The epoll is a kernel data structure to map a set of file descriptors (fd) to wacth. 
   // The epoll contains internally two lists. The interest are the fd that the process has registered to watch
   // and the ready are the set of fd ready to be ready.
   // epoll_create1 returns a fd too.
   // DOC: https://man7.org/linux/man-pages/man7/epoll.7.html
   //      https://man7.org/linux/man-pages/man2/epoll_create.2.html
   //      https://man7.org/linux/man-pages/man2/epoll_ctl.2.html 
   //      https://man7.org/linux/man-pages/man2/epoll_wait.2.html
   int epfd = epoll_create1(0);
   struct epoll_event ev = { .events = EPOLLIN, 
                               .data.fd = server 
                           };
   // Add server to epoll 
   epoll_ctl(epfd, EPOLL_CTL_ADD, server, &ev);
   ```