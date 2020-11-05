/**
****************多路I/O转接服务器****************
	epoll 阻塞监听，不再阻塞于真正的I/O系统调用上
	可以等待多个描述符就绪
	当有请求到达（即描述符准备就绪）调用真正的I/O系统调用读取并处理请求
	
	优先选择！！！		
**/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h> //bzero
#include <ctype.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <errno.h>

#include "../common/unp.h"

#define SERVER_IP	"127.0.0.1"
#define SERVER_PORT	8888
#define BUF_SIZE 	1024
#define MAX_OPEN_FD	1024

int main(int argc, char const *argv[])
{
	/* epoll */
	struct epoll_event tep;
	struct epoll_event ep_client[MAX_OPEN_FD];	//
	int epfd;	//保存 epoll_create 返回的文件描述符（一颗红黑树树根）
	int ret;

	int listenfd, connfd, sockfd;
	int client_fd[MAX_OPEN_FD];	//记录连接的 fd
	int max_client_index = -1;	//记录 client_fd[] 中最大存储位置下标
	int n_ready_client;			//满足条件 就绪的 fd 数量
	int client_index; //client_fd[client_index] 循环变量  gcc C99才允许在for中定义循环变量

	char buf[BUF_SIZE], addr_str[INET_ADDRSTRLEN];  /* #define INET_ADDRSTRLEN 16 */
	int read_bytes;	//实际读取字节数

	struct sockaddr_in server_addr, client_addr;	//地址结构体
	socklen_t client_addr_len;


	/**************** 第一步：socket() ******************/
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	/* 端口复用 */
	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	/**************** 第二步：bind() ********************/
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET; //ipv4
	inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr); //ip 主机字节序 to 网络字节序
	server_addr.sin_port = htons(SERVER_PORT); //port 主机字节序 to 网络字节序

	Bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));


	/**************** 第三步：listen() ******************/
	Listen(listenfd, 128);

	epfd = epoll_create(MAX_OPEN_FD);
	if(-1 == ret)
		perr_exit("epoll_create error");

	tep.events = EPOLLIN;	//读事件
	tep.data.fd = listenfd;	//文件描述符
	/* int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) */
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &tep);
	if(-1 == ret)
		perr_exit("epoll_ctl ADD error");

	while(1) {
		/* int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) */
		//阻塞等待请求
		n_ready_client = epoll_wait(epfd, ep_client, MAX_OPEN_FD, -1);
		if(-1 == n_ready_client)
			perr_exit("epoll_wait error");

		//有请求的 fd 由 ep_client 返回
		for(client_index = 0; client_index < n_ready_client; ++client_index) {
			if(!(EPOLLIN & ep_client[client_index].events))	//不是读事件 忽略
				continue;
	/**************** 第四步：accept() ******************/
			/* 新的连接请求 */
			if(ep_client[client_index].data.fd == listenfd) {
				client_addr_len = sizeof(client_addr);
				connfd = Accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
				printf("reveived connect from %s:%d\n", 
				inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str)), 
				ntohs(client_addr.sin_port)); 	//打印客户端信息 网络字节序 --> 主机字节序

				//添加到监听 红黑树上
				tep.events = EPOLLIN;
				tep.data.fd = connfd;
				ret = epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &tep);
				if(-1 == ret)
					perr_exit("epoll_ctl ADD error");
			}
			else {
	/**************** 第五步：读写 **********************/
				sockfd = ep_client[client_index].data.fd;
				read_bytes = Read(sockfd, buf, sizeof(buf));
				if(read_bytes < 0) {
					perror("read bytes < 0 error");
					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
					if(-1 == ret)
						perr_exit("epoll_ctl DEL error");
					Close(sockfd);
				}
				/* 当client关闭链接时,服务器端也关闭对应链接 */
				else if(0 == read_bytes) {
					 /* 解除对此文件描述符的监控 */
					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
					if(-1 == ret)
						perr_exit("epoll_ctl DEL error");
					printf("client[%d] close ...\n", client_index);
					Close(sockfd);

				}
				else if(read_bytes > 0) {
					/** do something **/
					int i = 0;
					while(i < read_bytes) {
						buf[i] = toupper(buf[i]);
						++i;
					}
					Writen(sockfd, buf, read_bytes);	//回复请求结果给客户端
					// Write(STDOUT_FILENO, buf, read_bytes);	//服务器端 打印结果
				}		
			}
		}
	}


	Close(listenfd);
	Close(epfd);


	return 0;
}