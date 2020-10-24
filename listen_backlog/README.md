## listen()函数backlog参数测试

> *参考：https://blog.csdn.net/yangbodong22011/article/details/60399728*

```c
#include <sys/socket.h>
int listen(int socket, int backlog);
```

测试环境

> CentOS 7

```shell
# uname -r
3.10.0-1127.13.1.el7.x86_64
# cat /proc/sys/net/ipv4/tcp_max_syn_backlog
128
```

**验证思路：**

1. 客户端开多个线程分别创建socket去连接服务端。
2. 服务端在`listen`之后，不去调用`accept`，也就是不会从`已完成队列`中取走`socket`连接。
3. 观察结果，到底服务端会怎么样？处于`ESTABLISHED`状态的套接字个数是不是就是`backlog`参数指定的大小呢？

```shell
gcc -o server server_backlog.c -std=c99
gcc -o client client_backlog.c -std=c99 -pthread
# 监控
watch -n 1 "netstat -natp | grep 8888"
```

> \# `watch -n 1 "netstat -natp | grep 8888"`  //root执行
>
> watch -n 1 表示每秒显示一次引号中命令的结果
>
> netstat `n: 以数字化显示 a:all t:tcp p:显示pid和进程名字` 

## 总结

* Linux 2.2 后，`backlog` 只表示处于**已完成连接队列**的上限值。
* 实际测试中，**已完成连接队列**最大可容纳 <font color=red>`backlog + 1` </font>个套接字。
* 进程调用 `accept` ，就是从**已完成连接队列**取出队头项返回给进程，如果该队列为**空**，进程睡眠，直到TCP在该队列中放入一项才唤醒它。
* 不同系统对于 backlog 可能会有不同的解释。