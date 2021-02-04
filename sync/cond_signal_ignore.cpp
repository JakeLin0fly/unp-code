/**
 * 【场景】条件变量的信号产生后，子线程才阻塞于等待条件满足
 * 【结论】条件满足但信号丢失，不再告知条件满足，子线程一直阻塞
**/

#include <unistd.h>
#include <pthread.h>
#include <iostream>

using namespace std;

struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond; 
} mutex_cond = {
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER
};


void *thread_son (void *arg) {
    sleep(4); // 等主线程 signal 结束
    pthread_mutex_lock(&mutex_cond.mutex);
    cout << "子线程：获取到互斥锁" << endl;
    
    cout << "子线程：尝试获取条件变量..." << endl;
    pthread_cond_wait(&mutex_cond.cond, &mutex_cond.mutex);
    cout << "子线程：获取到条件变量！" << endl;

    pthread_mutex_unlock(&mutex_cond.mutex);
    cout << "子线程：释放掉互斥锁" << endl;

    return 0;
}

int main(int argc, char const *argv[])
{
    pthread_t tid;

    pthread_create(&tid, NULL, thread_son, NULL);
    cout << "主线程：子线程创建完毕" << endl;

    pthread_mutex_lock(&mutex_cond.mutex);
    cout << "主线程：获取到互斥锁" << endl;

    cout << "主线程：发出 signal... " << endl;
    pthread_cond_signal(&mutex_cond.cond);

    pthread_mutex_unlock(&mutex_cond.mutex);
    cout << "主线程：释放掉互斥锁" << endl;

    pthread_join(tid, NULL); // 等待回收子线程

    return 0;
}
