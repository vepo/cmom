#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

int main(void) {
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

    struct epoll_event events[1024];
    char buf[4096];

    for (;;) {
        // 6. Block until events found on epoll or the timeout of 1024 milliseconds is reached
        int n = epoll_wait(epfd, events, 1024, -1);

        for (int i = 0; i < n; i++) {
            // 7. Check if the change is on the server or the connection fd
            if (events[i].data.fd == server) {
                // 8. Accept the new connection
                // DOC: https://man7.org/linux/man-pages/man2/accept.2.html
                int client = accept(server, NULL, NULL);
                if (client < 0) continue;
                printf("Connection open!\n");
                // 9. Set the new connection as async
                set_nonblocking(client);
                // 10. Register it to epoll
                struct epoll_event new_conn_evt = { .events = EPOLLIN, 
                                                    .data.fd = client 
                                                  };
                epoll_ctl(epfd, EPOLL_CTL_ADD, client, &new_conn_evt);
            } else {
                // 11. There is new data on connection, read it
                int bytes = read(events[i].data.fd, buf, sizeof(buf));
                if (bytes > 0) {
                    // 12. If there are bytes, write to output
                    printf("Data Received: %.*s\n", bytes, buf);
                    write(events[i].data.fd, buf, bytes);  // echo back
                } else {
                    // 13. Otherwise, remove from epoll and close the connection
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);
                    printf("Removing connection!\n");
                }
            }
        }
    }
    return 0;
}