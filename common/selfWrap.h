/**
* 自定义封装系统调用
* 主要完成异常处理
**/
#ifndef __SELFWRAP_H
#define __SELFWRAP_H

#include <sys/types.h>
#include <sys/socket.h>

/** perror + exit **/
void perr_exit(const char *err_str);	

int Socket(int domain, int type, int protocol);
int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int Listen(int sockfd, int backlog);
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
int Close(int fd);

ssize_t Readn(int fd, void *buf, size_t n);
ssize_t Writen(int fd, const void *buf, size_t n);

ssize_t Readline(int fd, void *buf, size_t maxlen);

#endif