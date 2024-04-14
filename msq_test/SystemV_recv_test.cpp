#include <iostream>
#include <sys/msg.h>
using namespace std;

struct s_msgbuf{
    long mtype;//固定类型
    int v;//可选类型
};

void test()
{
    int msgid = msgget(11, IPC_CREAT);
    if(msgid == -1){
        perror("msgget");
        return;
    }

    s_msgbuf msg;

    int rc = msgrcv(msgid, &msg, sizeof(msg.v), 1, IPC_NOWAIT);//需要ROOT权限
    if(rc == -1){
        perror("msgsnd");
        return;
    }
    printf("mtype: %ld   v: %d\n", msg.mtype, msg.v);
}

int main(int argc, char *argv[])
{
    test();
    return 0;
}
