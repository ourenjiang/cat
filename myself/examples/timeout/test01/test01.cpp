#include <iostream>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

int main() {
    // 设置文件描述符集合
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    // 设置超时时间为0秒，表示非阻塞
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    std::cout << "Enter something (or press Enter to exit):" << std::endl;

    while (true) {
        // 使用非阻塞的 select，监视 stdin 是否有可读事件
        int ready = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);

        if (ready == -1) {
            perror("select");
            break;
        } else if (ready > 0) {
            // 如果 stdin 有可读事件，读取输入
            char buffer[1024];
            ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));

            if (bytesRead == -1) {
                perror("read");
                break;
            }

            if (bytesRead == 0) {
                // 输入结束
                std::cout << "Exiting..." << std::endl;
                break;
            }

            // 处理输入
            buffer[bytesRead] = '\0';
            std::cout << "Read from stdin: " << buffer;
        } else {
            // 没有可读事件，可以执行其他任务
            // std::cout << "Waiting for input..." << std::endl;
        }

        // 清空文件描述符集合
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        // 重新设置超时时间为0秒
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
    }

    return 0;
}
