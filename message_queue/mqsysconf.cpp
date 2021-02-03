#include <iostream>
#include <cstring>
 #include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
 
using namespace std;
 
int main()
{
   cout << "MQ_OPEN_MAX = " << sysconf(_SC_MQ_OPEN_MAX) << endl
        << "MQ_PRIO_MAX = " << sysconf(_SC_MQ_PRIO_MAX) << endl;
    
    // printf("MQ_OPEN_MAX = %ld, MQ_PRIO_MAX = %ld\n", sysconf(_SC_MQ_OPEN_MAX), sysconf(_SC_MQ_PRIO_MAX));

    return 0;
}

/* Linux 3.10.0-1062.9.1.el7.x86_64
    MQ_OPEN_MAX = -1
    MQ_PRIO_MAX = 32768 
*/