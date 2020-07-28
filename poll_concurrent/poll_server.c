/**
****************多路I/O转接服务器****************
	poll阻塞监听，不再阻塞于真正的I/O系统调用上
	可以等待多个描述符就绪
	当有请求到达（即描述符准备就绪）调用真正的I/O系统调用读取并处理请求
	
	1. 无FD_SETSIZE限制
	2. 监听集合、返回集合分离
	3. 不用在整合集合（包括未使用的位置）中查找满足条件的事件，搜索范围缩小
			
**/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h> //bzero
#include <ctype.h>
#include <sys/types.h>
#include <poll.h>
#include <errno.h>

#include "../common/selfWrap.h"

#define SERVER_IP	"127.0.0.1"
#define SERVER_PORT	8888
#define BUF_SIZE 	1024
#define MAX_OPEN_FD	1024

int main(int argc, char const *argv[])
{
	int listenfd, connfd, sockfd;
	/* pollfd 结构体数组 */
	struct pollfd client[MAX_OPEN_FD];
	int max_index = -1;	//client[] 中最大存储位置下标
	int n_ready_client;
	int client_index; //client[client_index] 循环变量  gcc C99才允许在for中定义循环变量
	
	char buf[BUF_SIZE], addr_str[INET_ADDRSTRLEN]; /* #define INET_ADDRSTRLEN 16 */
	int read_bytes;	//实际读取字节数

	struct sockaddr_in server_addr, client_addr;	//地址结构体
	socklen_t client_addr_len;

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

	client[0].fd = listenfd; /* 监听的第一个文件描述符 listenfd 存入 client[0] */
	client[0].events = POLLIN;	/* listenfd 监听普通读事件 */
	//将保存客户端连接的网络套接字数组清空
	for(client_index = 1; client_index < MAX_OPEN_FD; ++client_index) {
		client[client_index].fd = -1; /* 用-1初始化client[]里剩下元素 */
	}
	max_index = 0;	//当前client[] 最大有效数据下标为 0

	while(1) {
		/* int poll(struct pollfd *fds, nfds_t nfds, int timeout); */
		n_ready_client = poll(client, max_index+1, -1);	//-1 阻塞等待
		if(n_ready_client < 0)
			perr_exit("select error");

		/* 第四步：accept() */
		//判断有新连接请求
		if(client[0].revents & POLLIN) {
			client_addr_len = sizeof(client_addr);
			connfd = Accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
			printf("reveived connect from %s:%d\n", 
				inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str)), 
				ntohs(client_addr.sin_port)); 	//打印客户端信息 网络字节序 --> 主机字节序

			/* 找client[]中第一个没有使用的位置 */
			for (client_index = 1; client_index < MAX_OPEN_FD; ++client_index) {
				if(client[client_index].fd < 0) {
					client[client_index].fd = connfd; /* 保存accept返回的文件描述符到client[]里 */
					client[client_index].events = POLLIN;	//监听读事件
					break;
				}
			}

			/* 达到select能监控的文件个数上限 1024 */
			if (client_index == MAX_OPEN_FD) {                              
                fputs("too many clients\n", stderr);
                exit(1);
            }

            if(client_index > max_index) /* 保证 max_index 存的总是client[]最后一个元素下标 */
            	max_index = client_index;

            /* 处理完一个连接请求，请求数减1 */
            n_ready_client -= 1;
            if(0 == n_ready_client)	//所有请求处理完成 继续监听
            	continue;	
		}


		/* 第五步：读写 */
		/* 有读写请求 依次检查判断哪个 client 有数据就绪 */
		for(client_index = 1; client_index <= max_index; ++client_index) {
			if(client[client_index].fd < 0)
				continue;

			/* 判断该描述符是否有读事件 */
			if(client[client_index].revents & POLLIN) {
				sockfd = client[client_index].fd;
				read_bytes = Read(sockfd, buf, sizeof(buf));
				if(read_bytes < 0) {
					if(ECONNRESET == errno) {	/* 收到 RST 标志 */
						printf("client[%d] aborted.\n", client_index);
						Close(sockfd);
						client[client_index].fd = -1;
					} else
						perr_exit("read error");
				}
				/* 当client关闭链接时,服务器端也关闭对应链接 */
				else if(0 == read_bytes) {
					printf("client[%d] closed.\n", client_index);
					Close(sockfd);
					client[client_index].fd = -1;	 /* 解除对此文件描述符的监控 */
				}
				else if(0 < read_bytes) {
					/** do something **/
					int i = 0;
					while(i < read_bytes) {
						buf[i] = toupper(buf[i]);
						++i;
					}
					Writen(sockfd, buf, read_bytes);	//回复请求结果给客户端
					// Write(STDOUT_FILENO, buf, read_bytes);	//服务器端 打印结果
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