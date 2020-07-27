/**
********************多进程并发服务器demo********************
父进程：监听等待接收客户端连接
子进程：处理客户端请求

信号处理：解决子进程结束后成为僵尸进程问题
*******************************************************
**/
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <strings.h>		//bzero
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#define SERVER_IP	"127.0.0.1"
#define SERVER_PORT	8888
#define BUF_SIZE 1024

/** 信号处理 回收子进程 **/
void sig_wait_child(int signo) {
	//0 回收同用户组的任意子进程
	//WNOHANG 非阻塞
	while(waitpid(0, NULL, WNOHANG) > 0);

	return ;
}

int main(int argc, char const *argv[])
{
	int l_socket, c_socket;
	struct sockaddr_in server_addr, client_addr;
	int ret, n;
	socklen_t client_addr_len;
	pid_t pid;
	char buf[BUF_SIZE], clinet_IP[BUF_SIZE];
	// struct sigaction new_action, old_action;

	//第一步：socket()
	l_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == l_socket) {
		perror("socket error");
		exit(1);
	}

	//第二步：bind()
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//or
	inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);
	ret = bind(l_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(-1 == ret) {
		perror("bind error");
		exit(1);
	}

	//第三步：listen()	系统最大支持128
	ret = listen(l_socket, 128);
	if(-1 == ret) {
		perror("listen error");
		exit(1);
	}

	// //注册信号处理
	// new_action.sa_handler = sig_wait_child;
	// sigemptyset(&new_action.sa_mask);
	// sigaddset(&new_action.sa_mask, SIGCHLD);
	// new_action.sa_flags = 0;

	while(true) {
		//第四步：accept()
		client_addr_len = sizeof(client_addr);
		c_socket = accept(l_socket, (struct sockaddr*)&client_addr, &client_addr_len);

		pid = fork();
		if(pid < 0) {
			perror("fork error");
			exit(1);
		}
		else if(pid == 0) { //子进程
			break;
		} else {	//父进程
			close(c_socket);	//关闭已连接的套接字
//			sigaction(SIGCHLD, &new_action, &old_action);
			signal(SIGCHLD, sig_wait_child);
			printf("client %s:%d\n", 
				inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, clinet_IP, sizeof(clinet_IP)),
					ntohs(client_addr.sin_port)
					);
		}
	}
//子进程 处理连接
	if(pid == 0) {
		close(l_socket);
		while(true) {
read_again:
			n = read(c_socket, buf, sizeof(buf));
			if(-1 == n) {
				if(errno == EINTR)
					goto read_again;
				else {
					perror("read error");
					close(c_socket);
					exit(1);	//不关闭文件描述符 子进程使用exit无法终止
					// _exit(1);	//不检查文件的打开情况，强行退出进程
				}
			}
			else if(0 == n) {
				close(c_socket);
				return 0;
			}
			/** do something **/
			for(int i = 0; i < n; ++i) {
				buf[i] = toupper(buf[i]);
			}
			write(c_socket, buf, n);
		}

	}

	return 0;
}