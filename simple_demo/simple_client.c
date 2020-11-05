#include "../common/unp.h"

#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h> // bzero

int main(int argc, char* argv[]){
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", argv[0]);
        return -1;
    }
    const char* ip = argv[1];       // IP 地址
    int port = atoi(argv[2]);       // 端口号

    int connfd; // 用于连接的描述符
    struct sockaddr_in serv_addr; // 服务端地

    /* 第一步：socket() */
	connfd = Socket(AF_INET, SOCK_STREAM, 0);
	
	/* 第二步： 初始化 serv_addr*/
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;	//IPV4
	inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);	//ip 主机字节序 --> 网络字节序
	serv_addr.sin_port = htons(port);		//port 主机字节序 --> 网络字节序

	/* 第三步：connet() */
	Connect(connfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    /* do something */
    while(1) sleep(3);

    Close(connfd); // 关闭已连接描述符
    return 0;
}
