
#include "Tcp.h"

void Argment(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}

int SocketInit(char *argv[], bool server) {
    int fd, ret;
    Addr_in addr;
    func_t func = server ? bind : connect;
    /*创建套接字*/
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        ErrExit("socket");
        return -1;
    }
    /*设置通信结构体*/
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    if (inet_aton(argv[1], &addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid address\n");
        exit(EXIT_FAILURE);
    }
    if (server) {
        /*地址快速复用*/
        int b_reuse = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &b_reuse, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(-1);
        }
    }
    /*发起连接请求或绑定地址*/
    ret = func(fd, (Addr *) &addr, sizeof(addr));
    if (ret < 0) {
        ErrExit("connect or bind");
        return -1;
    }
    if (server) {
        /*监听*/
        ret = listen(fd, BACKLOG);
        if (ret < 0) {
            ErrExit("listen");
            return -1;
        }
    }
    return fd;
}

int SocketDataHandle(int fd, void *buf, size_t len, DataHandle_t dataHandle) {
    int ret;
    char *str = dataHandle == recv ? "recv" : "send";//判断是recv还是send
    do {
        ret = dataHandle(fd, buf, len, 0);
    } while (ret < 0 && errno == EINTR);//如果read被信号中断，重新read
    if (ret < 0) {
        ErrExit(str);
    }
    return ret;
}
