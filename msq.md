## 1.1 消息队列接口（System V)

头文件

`#include <sys/msg.h>`

创建或获取消息队列

`int msgget(key_t key, int msgflg);`

发送消息

`int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);`

接收消息

`ssize_t msgrcv(int msgid, void *msgp, size_t msgsz, long msgtyp, int msgflg);`



## 1.2 消息队列接口（Posix)

头文件

`#include <mqueue.h>`

创建或打开消息队列

`mqd_t mq_open(const char *name, int oflag);`

`mqd_t mq_open(const char *name, int oflag, mode_t mode, struct mq_attr *attr);`

 发送消息

`int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio);`

接收消息

`ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio);`