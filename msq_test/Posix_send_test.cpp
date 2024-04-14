#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h> //sysconf
using namespace std;

void test(const char *path)
{
    long mq_prio = sysconf(_SC_MQ_PRIO_MAX);
    printf("mq prio: %ld\n", mq_prio);
    printf("mq path: %s\n", path);

    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1024;
    mqd_t mqd_id = mq_open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, &attr);
    if(mqd_id == (mqd_t) -1){
        perror("mq_open");
        return;
    }

    getchar();

    int v = 100;
    int rc = mq_send(mqd_id, (const char *)&v, sizeof(v), 32767);
    if(rc == -1){
        perror("mq_send");
        return;
    }
}

int main(int argc, char *argv[])
{
    if(argc != 2){
        return -1;
    }

    test(argv[1]);
    return 0;
}
