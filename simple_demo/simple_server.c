#include "../common/selfWrap.h"

#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h> // bzero

/*
    子进程操作
*/
void dosomthing(int* listenfd, int* connfd, struct sockaddr* client_addr){
    Close(*listenfd); // 子进程 首先关闭监听

    /* do something */
    while(1) sleep(3);

    Close(*connfd);
    exit(0); // 子进程终止
}

int main(int argc, char* argv[]){
    if( argc <= 3 )
    {
        printf( "usage: %s ip_address port_number backlog\n", argv[0]);
        return -1;
    }
    const char* ip = argv[1];       // IP 地址
    int port = atoi(argv[2]);       // 端口号
    int backlog = atoi(argv[3]);    // listen 参数：限制监听队列大小
    pid_t pid;

    int listenfd, connfd; // 监听请求的描述符、已连接的描述符
    struct sockaddr_in serv_addr, client_addr; // 服务端地址、客户端地址
    socklen_t client_addr_len; // 客户端地址长度
    

    /* 第一步：socket() 
     * 打开一个网络通讯端口（类似open返回一个文件描述符）
     * AF_INET		IPv4协议
	 * SOCK_STREAM	字节流套接口
	 * 0，让系统根据family和type的取值，选取合适的protocol的值
    */
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	
	/* 第二步：bind() 
     * 服务器程序所监听的网络地址和端口号通常是固定的，因此服务器需要调用bind绑定一个固定的网络地址和端口号
    */
    // 初始化 serv_addr
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;	//IPV4
	inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);	//ip 主机字节序 --> 网络字节序
	serv_addr.sin_port = htons(port);		//port 主机字节序 --> 网络字节序
	Bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	/* 第三步：listen() 
     * 主动连接套接口变为被动连接套接口 
     * backlog --- 已建立连接等待accept接收的套接字队列最大长度（backlog+1)
    */
	Listen(listenfd, backlog);
    
    /* 第四步：accept()
     * 默认为阻塞等待接收客户端连接请求
     * 从已建立连接的套接字队列里取出一个套接字
    */
    client_addr_len = sizeof(client_addr);
    while(1){
        connfd = Accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);

        if(0 == (pid = fork())){
            dosomthing(&listenfd, &connfd, (struct sockaddr*)&client_addr);
            // exit(0);
        }
        Close(connfd); // 父进程关闭已连接的描述符
    }

    Close(listenfd); // 主进程结束 关闭连接描述符
    return 0;
}
