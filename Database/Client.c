#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include "Tcp.h"


int do_register(int sockfd, MSG *msg);

int do_login(int sockfd, MSG *msg);

int do_query(int sockfd, MSG *msg);

int do_history(int sockfd, MSG *msg);

int main(int argc, char *argv[]) {
    int fd, ret;
    int Cmd;
    MSG msg;
    char buf[BUFSIZ] = {};
    /*检查参数*/
    Argment(argc, argv);
    /*创建套接字*/
    fd = SocketInit(argv, false);
    /*发送数据*/
    while (1) {
        printf("*****************************************\n");
        printf("      1.register   2.login   3.quit      \n");
        printf("*****************************************\n");
        printf("Please choose->");
        scanf("%d", &Cmd);
        getchar();
        switch (Cmd) {
            case 1:
                do_register(fd, &msg);
                break;
            case 2:
                if (do_login(fd, &msg) == 1) { //登录成功，进入下级菜单
                    goto next;
                }
                break;
            case 3:
                close(fd);
                exit(0);
                break;
            default:
                printf("Cmd Error!\n");
                break;
        }
    }
    next:
    while (1) {
        printf("*****************************************\n");
        printf("  1.query_word  2.history_record  3.quit \n");
        printf("*****************************************\n");
        printf("Please choose->");
        scanf("%d", &Cmd);
        getchar();
        switch (Cmd) {
            case 1:
                do_query(fd, &msg);
                break;
            case 2:
                do_history(fd, &msg);
                break;
            case 3:
                close(fd);
                exit(0);
                break;
            default:
                printf("Cmd Error!\n");
                break;
        }
    }
    close(fd);
    return 0;
}

/*用户注册*/
int do_register(int sockfd, MSG *msg) {
    int ret;
    msg->type = R;
    printf("Input username->");
    scanf("%s", msg->name);
    getchar();
    printf("Input password->");
    scanf("%s", msg->data);

    printf("name = %s, data = %s\n", msg->name, msg->data);
    /*向服务器发送数据*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //发送失败
        ErrExit("send");
        return -1;
    }
    /*接受服务器返回的数据*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), recv);
    if (ret < 0) {
        ErrExit("recv");
        return -1;
    }
    //返回OK或者返回用户名已存在，进而提示用户
    printf("%s\n", msg->data);
    return 0;
}

/*用户登录*/
int do_login(int sockfd, MSG *msg) {
    int ret;
    msg->type = L;
    printf("Input username->");
    scanf("%s", msg->name);
    getchar();
    printf("Input password->");
    scanf("%s", msg->data);

    printf("name = %s, data = %s\n", msg->name, msg->data);
    /*向服务器发送数据*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //发送失败
        ErrExit("send");
        return -1;
    }
    /*接受服务器返回的数据*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), recv);
    if (ret < 0) {
        ErrExit("recv");
        return -1;
    }
    //返回OK或者返回密码错误，进而提示用户
    printf("%s\n", msg->data);
    if (strncmp(msg->data, "OK", 2) == 0) {
        printf("Login success!\n");
        return 1; //登录成功
    } else {
        printf("Login failed!\n");
        return 0;//登录失败
    }
}

/*查询单词*/
int do_query(int sockfd, MSG *msg) {
    int ret;
    msg->type = Q;
    puts("-------------------");
    while (1) {
        printf("Input word->");
        scanf("%s", msg->data);
        getchar();
        /*输入#退出查询*/
        if (strncmp(msg->data, "#", 1) == 0)
            break;
        /*向服务器发送数据*/
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
        if (ret < 0) { //发送失败
            ErrExit("send");
            return -1;
        }
        /*接受服务器返回的数据*/
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), recv);
        if (ret < 0) {
            ErrExit("recv");
            return -1;
        }
        //返回单词注释信息或者返回单词不存在，进而提示用户
        printf("%s\n", msg->data);
    }
    return 0;
}


/*查询历史记录*/
int do_history(int sockfd, MSG *msg) {
    int ret;
    msg->type = H;
    /*向服务器发送数据*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //发送失败
        ErrExit("send");
        return -1;
    }
    while (1) {
        /*接受服务器返回的数据*/
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), recv);
        if (ret < 0) {
            ErrExit("recv");
            return -1;
        }
        if (msg->data[0] == '\0')
            break;
        //输出历史记录信息
        printf("%s\n", msg->data);
    }
    return 0;
}