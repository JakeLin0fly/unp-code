/**
****************多路I/O转接服务器****************
	select阻塞监听，不再阻塞于真正的I/O系统调用上
	select优势在于可以等待多个描述符就绪
	当有请求到达（即描述符准备就绪）调用真正的I/O系统调用读取并处理请求
	
	select监听集合 fd_set 有最大数值 FD_SETSIZE 默认1024
	/usr/include/linux/posix_types.h
		#undef __FD_SETSIZE
		#define __FD_SETSIZE    1024 	
	/usr/include/sys/select.h 
		Maximum number of file descriptors in `fd_set'.  
   		#define FD_SETSIZE              __FD_SETSIZE
			
**/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h> //bzero
#include <ctype.h>
#include <sys/types.h>

#include "../common/unp.h"

#define SERVER_IP	"127.0.0.1"
#define SERVER_PORT	8888
#define BUF_SIZE 1024

int main(int argc, char const *argv[])
{
	int max_index = -1;	//allfd_set 中最大存储位置下标
	int listenfd, connfd, sockfd, maxfd;
	/* 自定义数组client, 防止遍历1024个文件描述符  FD_SETSIZE默认为1024 */
	int client[FD_SETSIZE], n_ready_client, client_index;
	char buf[BUF_SIZE+1], addr_str[INET_ADDRSTRLEN]; /* #define INET_ADDRSTRLEN 16 */
	int read_bytes;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addr_len;
	fd_set readfd_set, allfd_set;	//文件描述符集合

	/* 第一步：socket() */
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	/* 端口复用 */
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	/* 第二步：bind() */
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;	//IPV4
	inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);	//ip 主机字节序 --> 网络字节序
	server_addr.sin_port = htons(SERVER_PORT);		//port 主机字节序 --> 网络字节序

	Bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	/* 第三步：listen() */
	Listen(listenfd, 128);

	maxfd = listenfd;
	//将保存客户端连接的网络套接字数组清空
	int i = 0;
	while(i < FD_SETSIZE) {
		client[i] = -1;
		++i;
	}
	FD_ZERO(&allfd_set);	//所有套接字集合清零 select 监控文件描述符集
	FD_SET(listenfd, &allfd_set);	//添加监听套接字 listenfd

	while(1) {
		readfd_set = allfd_set;	//每次循环重置 select 监控集
		/* int select(int nfds, fd_set *readfds, fd_set *writefds, 
 		fd_set *exceptfds, struct timeval *timeout); */
		n_ready_client = select(maxfd+1, &readfd_set, NULL, NULL, NULL);
		if(n_ready_client < 0)
			perr_exit("select error");

		/* 第四步：accept() */
		//判断有新连接请求
		if(FD_ISSET(listenfd, &readfd_set)) {
			client_addr_len = sizeof(client_addr);
			connfd = Accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
			printf("reveived connect from %s:%d\n", 
				inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str)), 
				ntohs(client_addr.sin_port)); 	//打印客户端信息 网络字节序 --> 主机字节序

			/* 找client[]中第一个没有使用的位置 */
			for (client_index = 0; client_index < FD_SETSIZE; ++client_index) {
				if(client[client_index] < 0) {
					client[client_index] = connfd; /* 保存accept返回的文件描述符到client[]里 */
					break;
				}
			}

			/* 达到select能监控的文件个数上限 1024 */
			if (client_index == FD_SETSIZE) {                              
                fputs("too many clients\n", stderr);
                exit(1);
            }

            FD_SET(connfd, &allfd_set);	/* 向监控文件描述符集合allset添加新的文件描述符connfd */
            if(connfd > maxfd)	/* select第一个参数需要 */
            	maxfd = connfd;
            if(client_index > max_index) /* 保证 max_index 存的总是client[]最后一个元素下标 */
            	max_index = client_index;

            /* 处理完一个连接请求，请求数减1 */
            n_ready_client -= 1;
            if(0 == n_ready_client)	//所有请求处理完成 继续监听
            	continue;	
		}


		/* 第五步：读写 */
		/* 有读写请求 依次检查判断哪个clients 有数据就绪 */
		for(client_index = 0; client_index <= max_index; ++client_index) {
			if(client[client_index] < 0)
				continue;

			sockfd = client[client_index];
			if(FD_ISSET(sockfd, &readfd_set)) {
				read_bytes = Read(sockfd, buf, sizeof(buf));
				/* 当client关闭链接时,服务器端也关闭对应链接 */
				if(0 == read_bytes) {
					Close(sockfd);
					FD_CLR(sockfd, &allfd_set);	 /* 解除select对此文件描述符的监控 */
					client[client_index] = -1;
				}
				else if(0 < read_bytes) {
					/** do something **/
					int i = 0;
					while(i < read_bytes) {
						buf[i] = toupper(buf[i]);
						++i;
					}
					Write(sockfd, buf, read_bytes);	//回复请求结果给客户端
					Write(STDOUT_FILENO, buf, read_bytes);	//服务器端 打印结果
				}

				/* 处理完一个读写请求，请求数减1 */
	            n_ready_client -= 1;
	            if(0 == n_ready_client)	//所有请求处理完成 继续监听
	            	break;
			}
		}
	}//服务器一直循环监听等待
	
	Close(listenfd);

	return 0;
}