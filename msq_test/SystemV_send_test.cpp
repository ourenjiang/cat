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
    msg.mtype = 1;
    msg.v = 100;

    int rc = msgsnd(msgid, &msg, sizeof(msg.v), IPC_NOWAIT);//需要ROOT权限
    if(rc == -1){
        perror("msgsnd");
        return;
    }
}

int main(int argc, char *argv[])
{
    test();
    return 0;
}
