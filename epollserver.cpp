#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <memory>

int create_and_bind(char* port)
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result;
    int s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(s) << std::endl;
        return -1;
    }

    int fd = -1;
    for (auto rp = result; rp != nullptr; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd != -1) {
            if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
                break;
            } else {
                close(fd);
                fd = -1;
            }
        }
    }
    
    freeaddrinfo(result);
    return fd;
}

int make_socket_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        flags |= O_NONBLOCK;
        if (fcntl(fd, F_SETFL, flags) == -1)
            return -1;
    }
    return 0;
}

void handle_io_on_socket(int fd)
{
    char buf[2048];
    int count = read(fd, buf, sizeof(buf));
    if (count == -1) {
        std::cout << "read error " << std::endl;
    } else {
        write(fd, buf, count);
    }
}

void open_new_connection(int fd, int efd)
{
    sockaddr in_addr;
    socklen_t in_len = sizeof(in_addr);
    std::cout << "try to accept fd=" << fd << std::endl;
    int client_fd = accept(fd, &in_addr, &in_len);
    
    if (client_fd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::cout << "no blocking, can not open connection" << std::endl;
            return;
        } else {
            std::cerr << "accept error " << errno << std::endl;
            return;
        }
    }
    
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    if (getnameinfo(&in_addr, in_len, hbuf, sizeof(hbuf), 
            sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
        std::cout << "accepted connection on fd " << client_fd << ", host=" <<
            hbuf << " , port=" << sbuf << std::endl;
    }
    
    make_socket_non_blocking(client_fd);
    
    epoll_event event;
    event.data.fd = client_fd;
    event.events = EPOLLIN;
    epoll_ctl(efd, EPOLL_CTL_ADD, client_fd, &event);
}

int main(int argc, char** argv)
{
    int fd = create_and_bind("8000");
    if (fd == -1) {
        std::cout << "create and bind error " << std::endl;
        return -1;
    }
    std::cout << "create socket fd " << fd << std::endl;
    
    if (make_socket_non_blocking(fd) == -1) {
        std::cout << "make non-blocking error " << std::endl;
        return -1;
    }
    
    if (listen(fd, SOMAXCONN) == -1) {
        std::cout << "listen error " << std::endl;
        return -1;
    }   
    
    int efd = epoll_create1(0);
    if (efd == -1) {
        std::cout << "epoll create error " << std::endl;
        return -1;
    }
    std::cout << "create epoll fd " << efd << std::endl;
    
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
    
    const int MAXEVENTS = 64;
    std::unique_ptr<epoll_event[]> events(new epoll_event[MAXEVENTS]);
    
    while (true) {
        int n = epoll_wait(efd, events.get(), MAXEVENTS, -1);
        for (int i=0; i<n; ++i) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == fd) {
                    open_new_connection(fd, efd);
                } else {
                    handle_io_on_socket(events[i].data.fd);
                }
            } else {
                std::cout << "other events " << events[i].events << std::endl;
                break;
            }
        }
    }    
    
    return 0;
}
