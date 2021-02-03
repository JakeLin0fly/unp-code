/**
 * 【场景】子线程在获得 互斥锁 后，等待获取 条件变量 时被取消
 * 【结论】子线程终止没有再持有互斥锁（与《UNP卷二》P151 所述不同）
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
    sleep(2); // 保证子线程获得互斥锁并阻塞在条件变量上

    /* 证明线程阻塞在条件变量上时释放了互斥锁 */
    // pthread_mutex_lock(&mutex_cond.mutex);
    // cout << "主线程：获取到互斥锁" << endl;

    // pthread_mutex_unlock(&mutex_cond.mutex);
    // cout << "主线程：释放掉互斥锁" << endl;

    cout << "主线程：正在取消子线程..." << endl;
    pthread_cancel(tid);
    cout << "主线程：已取消子线程，尝试获取互斥锁..." << endl;

    pthread_mutex_lock(&mutex_cond.mutex);
    cout << "主线程：获取到互斥锁" << endl;

    pthread_mutex_unlock(&mutex_cond.mutex);
    cout << "主线程：释放掉互斥锁" << endl;

    return 0;
}
