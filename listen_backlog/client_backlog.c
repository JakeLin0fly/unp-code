#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<strings.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>


struct sockaddr_in serv_addr;

void *func() 
{
    int conn_fd;
    conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("conn_fd : %d\n",conn_fd);

    if( connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) == -1) {
        printf("connect error\n");
    }

    while(1) {} // 不退出
}

int main(int argc,char *argv[])
{
    if( argc <= 3 )
    {
        printf( "usage: %s ip_address port_number thread_num\n", argv[0]);
        return -1;
    }
    const char* ip = argv[1];       // IP 地址
    int port = atoi(argv[2]);       // 端口号
    int thread_num = atoi(argv[3]);    // thread_num 线程数

    memset(&serv_addr,0,sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);    
    inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);	
    int retval;

    //创建线程并且等待线程完成
    pthread_t pid[thread_num];
    for(int i = 0 ; i < thread_num; ++i)
    {
        pthread_create(&pid[i], NULL, &func, NULL);

    }

    for(int i = 0 ; i < thread_num; ++i)
    {
        pthread_join(pid[i], (void*)&retval);
    }

    return 0;
}
