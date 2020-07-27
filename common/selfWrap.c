// #include "selfWrap.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>


#define READLINE_ONCE_BYTES	100

/** perror + exit **/
void perr_exit(const char *err_str) {
	perror(err_str);
	exit(1);
}

/*	
	打开一个网络通讯端口
	成功：返回一个新的文件描述符  
	失败：设置errno, exit退出
*/
int Socket(int domain, int type, int protocol) {
	int sockfd;
	sockfd = socket(domain, type, protocol);
	if(-1 == sockfd) {
		perr_exit("socket error");
	}

	return sockfd;
}

/*	
	将参数sockfd和addr绑定在一起，使sockfd这个用于网络通讯的文件描述符监听addr所描述的地址和端口号
	成功：返回 0
	失败：设置errno, exit退出
*/
int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	int ret;
	ret = bind(sockfd, addr, addrlen);
	if(-1 == ret) {
		perr_exit("bind error");
	}

	return ret;
}

/*	
	主动连接套接口变为被动连接套接口，使得一个进程可以接受其它进程的请求，从而成为一个服务器进程。
	指定允许多少个客户端同时与我建立连接（系统限制最大同时发起连接数128）
	成功：返回 0
	失败：设置errno, exit退出
*/
int Listen(int sockfd, int backlog) {
	int ret;
	ret = listen(sockfd, backlog);
	if(-1 == ret) {
		perr_exit("listen error");
	}

	return ret;
}

/*	
	阻塞等待接收客户端连接请求
	成功：返回一个新的socket文件描述符(非负整数)
	失败：设置errno, exit退出
*/
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
	int accepted_socket;

accept_again:
	accepted_socket = accept(sockfd, addr, addrlen);
	if(-1 == accepted_socket) {
		if(ECONNABORTED == errno || EINTR == errno)
			goto accept_again;
		else
			perr_exit("accept error");
	}

	return accepted_socket;
}

/*
	客户端调用connect()连接服务器	
	成功：返回 0
	失败：设置errno, exit退出
*/
int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	int ret;
	ret = connect(sockfd, addr, addrlen);
	if(-1 == ret) {
		perr_exit("connect error");
	}

	return ret;
}

/*	
	从文件描述符 fd (一次)读取数据到 buf,最大读取字节数count
	成功：返回实际读取字节数
	失败：设置errno, 返回 -1
*/
ssize_t Read(int fd, void *buf, size_t count) {
	ssize_t read_bytes;

read_again:
	read_bytes = read(fd, buf, count);
	if(-1 == read_bytes) {
		if(EINTR == read_bytes)
			goto read_again;
		else
			return -1;
	}

	return read_bytes;
}

/*	
	将 buf中前count个字节(一次)写入文件描述符 fd 
	成功：返回实际写入字节数
	失败：设置errno, 返回 -1
*/
ssize_t Write(int fd, const void *buf, size_t count) {
	ssize_t write_bytes;

write_again:
	write_bytes = write(fd, buf, count);
	if(-1 == write_bytes) {
		if(EINTR == errno)
			goto write_again;
		else
			return -1;
	}

	return write_bytes;
}

/*
	关闭指定文件描述符fd
	成功：返回 0
	失败：设置errno, exit退出
*/
int Close(int fd) {
	int ret;
	ret = close(fd);
	if(-1 == ret) {
		perr_exit("close error");
	}
	return ret;
}

/*	
	从文件描述符 fd 读取数据到 buf,最大读取字节数 n
	可多次读取，直到异常、读取到 n 字节数据或读取到末尾结束
	成功：返回实际读取字节数
	失败：设置errno, 返回 -1
*/
ssize_t Readn(int fd, void *buf, size_t n) {
	size_t remain_bytes;	//剩余未读取字节数
	size_t read_bytes;		//实际读取字节数
	char *pbuf;

	pbuf =buf;
	remain_bytes = n;

	while(remain_bytes > 0) {
		read_bytes = read(fd, pbuf, remain_bytes);
		if(-1 == read_bytes) {
			if(EINTR == errno) 
				read_bytes = 0;
			else
				return -1;
		}
		else if(0 == read_bytes) //字节读取完
			break;
		remain_bytes -= read_bytes;
		pbuf += read_bytes;
	}

	return n - remain_bytes;
}

/*	
	将 buf中前 n 个字节写入文件描述符 fd,连续写入，直到全部写入或异常退出
	成功：返回实际写入字节数
	失败：设置errno, 返回 -1
*/
ssize_t Writen(int fd, const void *buf, size_t n) {
	size_t remain_bytes;
	size_t write_bytes;
	const char *pbuf;

	pbuf = (const char *)buf;
	remain_bytes = n;

	while(remain_bytes > 0) {
		write_bytes = write(fd, pbuf, remain_bytes);
		if(-1 == write_bytes) { 
			if(EINTR == errno)
				write_bytes = 0;
			else
				return -1;
		}
		remain_bytes -= write_bytes;
		pbuf += write_bytes;
	}

	return n - remain_bytes;
}

/*
	从文件描述符fd中读取一行数据到buf,最大读取字节数maxlen
	成功：返回实际读取字节数
	失败：设置errno, 返回-1
*/
ssize_t Readline(int fd, void *buf, size_t maxlen) {
	static char read_buf[READLINE_ONCE_BYTES];
	static char *p_read_buf; 
	static int read_buf_bytes = 0;	//read_buf 未做判断的字节数
	char *pbuf = buf;
	size_t read_bytes = 0;	//实际buf读取字节数

	while(read_bytes < maxlen) {
		//静态缓冲区 read_buf 为空，I/O读取一次数据
		if(read_buf_bytes <= 0) {
read_once:	//一次I/O操作读取READLINE_ONCE_BYTES字节
			read_buf_bytes = read(fd, read_buf, READLINE_ONCE_BYTES);
			if(-1 == read_buf_bytes) {
				if(EINTR == errno) 
					goto read_once;
				else
					return -1;	//异常 退出
			}
			else if(0 == read_buf_bytes)
				break;	//读取到文件末尾

			p_read_buf = read_buf;	//读取一次 缓冲区指针指向缓冲区第一个字符
		}

		while(read_bytes < maxlen && read_buf_bytes > 0) {
			if('\n' == *p_read_buf) {	//读取到换行符
				read_buf_bytes--;
				if(read_bytes > 0)	//防止指针越界
					p_read_buf++;
				return read_bytes;	//读取到一行 返回实际读取字节数
			}
			else {	//读取一个字符
				*pbuf++ = *p_read_buf++;
				read_bytes++;
			}
		}
	}

	return read_bytes;
}