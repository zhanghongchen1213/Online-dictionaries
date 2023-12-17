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
    /*������*/
    Argment(argc, argv);
    /*�����׽���*/
    fd = SocketInit(argv, false);
    /*��������*/
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
                if (do_login(fd, &msg) == 1) { //��¼�ɹ��������¼��˵�
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

/*�û�ע��*/
int do_register(int sockfd, MSG *msg) {
    int ret;
    msg->type = R;
    printf("Input username->");
    scanf("%s", msg->name);
    getchar();
    printf("Input password->");
    scanf("%s", msg->data);

    printf("name = %s, data = %s\n", msg->name, msg->data);
    /*���������������*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //����ʧ��
        ErrExit("send");
        return -1;
    }
    /*���ܷ��������ص�����*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), recv);
    if (ret < 0) {
        ErrExit("recv");
        return -1;
    }
    //����OK���߷����û����Ѵ��ڣ�������ʾ�û�
    printf("%s\n", msg->data);
    return 0;
}

/*�û���¼*/
int do_login(int sockfd, MSG *msg) {
    int ret;
    msg->type = L;
    printf("Input username->");
    scanf("%s", msg->name);
    getchar();
    printf("Input password->");
    scanf("%s", msg->data);

    printf("name = %s, data = %s\n", msg->name, msg->data);
    /*���������������*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //����ʧ��
        ErrExit("send");
        return -1;
    }
    /*���ܷ��������ص�����*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), recv);
    if (ret < 0) {
        ErrExit("recv");
        return -1;
    }
    //����OK���߷���������󣬽�����ʾ�û�
    printf("%s\n", msg->data);
    if (strncmp(msg->data, "OK", 2) == 0) {
        printf("Login success!\n");
        return 1; //��¼�ɹ�
    } else {
        printf("Login failed!\n");
        return 0;//��¼ʧ��
    }
}

/*��ѯ����*/
int do_query(int sockfd, MSG *msg) {
    int ret;
    msg->type = Q;
    puts("-------------------");
    while (1) {
        printf("Input word->");
        scanf("%s", msg->data);
        getchar();
        /*����#�˳���ѯ*/
        if (strncmp(msg->data, "#", 1) == 0)
            break;
        /*���������������*/
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
        if (ret < 0) { //����ʧ��
            ErrExit("send");
            return -1;
        }
        /*���ܷ��������ص�����*/
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), recv);
        if (ret < 0) {
            ErrExit("recv");
            return -1;
        }
        //���ص���ע����Ϣ���߷��ص��ʲ����ڣ�������ʾ�û�
        printf("%s\n", msg->data);
    }
    return 0;
}


/*��ѯ��ʷ��¼*/
int do_history(int sockfd, MSG *msg) {
    int ret;
    msg->type = H;
    /*���������������*/
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //����ʧ��
        ErrExit("send");
        return -1;
    }
    while (1) {
        /*���ܷ��������ص�����*/
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), recv);
        if (ret < 0) {
            ErrExit("recv");
            return -1;
        }
        if (msg->data[0] == '\0')
            break;
        //�����ʷ��¼��Ϣ
        printf("%s\n", msg->data);
    }
    return 0;
}