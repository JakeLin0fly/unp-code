#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <strings.h>		//bzero
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define SERVER_PORT	8888
#define BUF_SIZE 1024

int main(int argc, char const *argv[])
{
	int l_socket, c_socket;
	struct sockaddr_in server_addr, client_addr;
	int ret, n;
	socklen_t client_addr_len;
	pid_t pid;
	char buf[BUF_SIZE];

	//第一步：socket()
	l_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == l_socket) {
		perror("socket error: ");
		exit(1);
	}

	//第二步：bind()
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//or
	//inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);
	ret = bind(l_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(-1 == ret) {
		perror("bind error: ");
		exit(1);
	}

	//第三步：listen()	系统最大支持128
	ret = listen(l_socket, 128);
	if(-1 == ret) {
		perror("listen error: ");
		exit(1);
	}

	while(true) {
		//第四步：accept()
		client_addr_len = sizeof(client_addr);
		c_socket = accept(l_socket, (struct sockaddr*)&client_addr, &client_addr_len);

		pid = fork();
		if(pid < 0) {
			perror("fork error: ");
			exit(1);
		}
		else if(pid == 0) { //子进程
			break;
		} else {	//父进程
		//	printf("client %s:%d\n", 
		//		inet_ntop(AF_INET, &client_addr.sin_addr.s_addr);
			close(c_socket);
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
					perror("listen error: ");
					exit(1);
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