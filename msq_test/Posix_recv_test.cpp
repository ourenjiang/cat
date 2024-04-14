#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h> //sysconf
using namespace std;

void test(const char *path)
{
    // long mq_prio = sysconf(_SC_MQ_PRIO_MAX);
    // printf("mq prio: %ld\n", mq_prio);
    printf("mq path: %s\n", path);

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1024;
    mqd_t mqd_id = mq_open(path, O_RDWR | O_CREAT, 0600, &attr);
    if(mqd_id == (mqd_t) -1){
        perror("mq_open");
        return;
    }

    getchar();

    int v = 100;
    int rc = mq_getattr(mqd_id, &attr);
    if(rc == -1){
        perror("mq_getattr");
        return;
    }
    printf("mq_maxmsg: %ld, mq_msgsize: %ld\n", attr.mq_maxmsg, attr.mq_msgsize);

    void *ptr = malloc(attr.mq_msgsize);
    
    unsigned mq_prio;
    rc = mq_receive(mqd_id, (char*)ptr, attr.mq_msgsize, &mq_prio);
    if(rc == -1){
        perror("mq_send");
        return;
    }
    printf("recv prio: %u, bytes: %d\n", mq_prio, rc);
}

int main(int argc, char *argv[])
{
    if(argc != 2){
        return -1;
    }

    test(argv[1]);
    return 0;
}
