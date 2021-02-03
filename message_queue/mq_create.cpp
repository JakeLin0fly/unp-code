#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>

int main(int argc, char const *argv[])
{
    int flag = O_RDWR | O_CREAT;
    mode_t mode = 0666;

    mqd_t m_mqd; // 消息队列描述符

    if (argc != 2) {
        perror("Please input like : ./a.out pathname \n");
        exit(1);
    }

    m_mqd = mq_open(argv[1], flag, mode, NULL);
    if (-1 == m_mqd) {
        perror("create messagequeue error!");
        exit(1);
    }
    sleep(10);
    mq_close(m_mqd);

    return 0;
}
