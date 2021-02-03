#include <iostream>
#include <cstring>
 
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
 
using namespace std;
 
int main()
{
    mqd_t mqID;
    mqID = mq_open("/anonymQueue", O_RDWR | O_CREAT, 0666, NULL);
 
    if (mqID < 0)
    {
        cout<<"open message queue error..."<<strerror(errno)<<endl;
        return -1;
    }
 
    mq_attr mqAttr;
    if (mq_getattr(mqID, &mqAttr) < 0)
    {
        cout<<"get the message queue attribute error"<<endl;
        return -1;
    }
 
    cout<<"mq_flags:"<<mqAttr.mq_flags<<endl;
    cout<<"mq_maxmsg:"<<mqAttr.mq_maxmsg<<endl;
    cout<<"mq_msgsize:"<<mqAttr.mq_msgsize<<endl;
    cout<<"mq_curmsgs:"<<mqAttr.mq_curmsgs<<endl;
}

/* Linux 3.10.0-1062.9.1.el7.x86_64
    mq_flags:0
    mq_maxmsg:10
    mq_msgsize:8192
    mq_curmsgs:0
*/