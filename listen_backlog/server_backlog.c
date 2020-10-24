#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

void my_err(const char* msg, int line) 
{
    fprintf(stderr, "line:%d", line);
    perror(msg);
}


int main(int argc,char *argv[])
{
    if( argc <= 3 )
    {
        printf( "usage: %s ip_address port_number backlog\n", argv[0]);
        return -1;
    }
    const char* ip = argv[1];       // IP 地址
    int port = atoi(argv[2]);       // 端口号
    int backlog = atoi(argv[3]);    // listen 参数

    int conn_len;
    int sock_fd, conn_fd;
    struct sockaddr_in serv_addr, conn_addr;


    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { 
        my_err("socket", __LINE__); 
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr);	

    if(bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) == -1) {
        my_err("bind",__LINE__);
        exit(1);
    }

    // listen 后不调用 accept
    if(listen(sock_fd, backlog) == -1) {
        my_err("sock", __LINE__);
        exit(1);
    }

    conn_len = sizeof(struct sockaddr_in);


    sleep(10);                  //sleep 10s之后接受一个连接
    printf("I will accept one\n");
    accept(sock_fd,(struct sockaddr *)&conn_addr,(socklen_t *)&conn_len);

    sleep(10);                  //同理，再接受一个
    printf("I will accept one\n");
    accept(sock_fd,(struct sockaddr *)&conn_addr,(socklen_t *)&conn_len);

    sleep(10);                  //同理，再次接受一个
    printf("I will accept one\n");
    accept(sock_fd,(struct sockaddr *)&conn_addr,(socklen_t *)&conn_len);


    while(1) {}  //之后进入while循环,不释放连接
    return 0;
}