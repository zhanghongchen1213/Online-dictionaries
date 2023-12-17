#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <signal.h>
#include "Tcp.h"
#include <time.h>

#define DATABASE "my.db"
#define FILENAME "/home/k/Linux_Learn/Database/dict.txt"
int Userflag = 0;//root��¼��־

int do_register(int sockfd, MSG *msg, sqlite3 *db);

int do_login(int sockfd, MSG *msg, sqlite3 *db);

int do_query(int sockfd, MSG *msg, sqlite3 *db);

int do_history(int sockfd, MSG *msg, sqlite3 *db);

void do_client(int sockfd, Addr_in client_addr, sqlite3 *db);

int do_searchword(int sockfd, MSG *msg, char word[]);

int get_date(char *date);

int main(int argc, char *argv[]) {
    int fd, newfd, ret;
    char buf[BUFSIZ];
    int Cmd;
    sqlite3 *db;
    Addr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    pid_t pid;
    /*������*/
    Argment(argc, argv);
    /*�����ݿ�*/
    if (sqlite3_open(DATABASE, &db) != SQLITE_OK) {
        printf("sqlite3_open: %s\n", sqlite3_errmsg(db));
        return -1;
    } else
        printf("sqlite3_open success\n");
    /*�����׽���*/
    fd = SocketInit(argv, true);
    /*����ʬ����*/
    signal(SIGCHLD, SIG_IGN);//����SIGCHLD�ź�
    /*���ܿͻ�������*/
    while (1) {
        newfd = accept(fd, (Addr *) &client_addr, &addrlen);
        if (newfd < 0) {
            ErrExit("accept");
            return -1;
        }
        printf("addr:%s\n port=%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        if ((pid = fork()) < 0) {
            ErrExit("Fork");
            return -1;
        } else if (pid == 0) {//�ӽ��̣�����ͻ�������
            close(fd);//�رո����̵��׽���
            do_client(newfd, client_addr, db);
            exit(0);
        } else //�����̣��������ܿͻ�������
            close(newfd);
    }
    close(fd);
    return 0;
}

/*����ͻ�������*/
void do_client(int sockfd, Addr_in client_addr, sqlite3 *db) {
    MSG msg;
    int ret;
    while (1) {
        int ret = SocketDataHandle(sockfd, &msg, sizeof(MSG), recv);
        if (ret == 0) {//�ͻ���ǿ���˳�
            printf("[%s:%d]:client out\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            break;
        } else {//�ͻ���������������
            printf("[%s:%d]:type-->%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg.type);
            switch (msg.type) {
                case R:
                    do_register(sockfd, &msg, db);
                    break;
                case L:
                    do_login(sockfd, &msg, db);
                    break;
                case Q:
                    do_query(sockfd, &msg, db);
                    break;
                case H:
                    do_history(sockfd, &msg, db);
                    break;
                default:
                    printf("Invalid data msg.\n");
                    break;
            }
        }
    }
}

int do_register(int sockfd, MSG *msg, sqlite3 *db) {
    int ret;
    char *errmsg;
    char sql[512];
    sprintf(sql, "insert into user values('%s', '%s');", msg->name, msg->data);
    printf("sql = %s\n", sql);
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        //����ʧ��
        printf("%s\n", errmsg);
        strcpy(msg->data, "User already exists");
    } else {
        //����ɹ�
        printf("Insert success\n");
        strcpy(msg->data, "OK!");
    }
    //��ͻ��˷������ݣ���ʾע��ɹ�����ʧ��
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //����ʧ��
        ErrExit("send");
        return -1;
    }
    return 0;
}

int do_login(int sockfd, MSG *msg, sqlite3 *db) {
    int ret, nrow, ncolumn;
    char *errmsg;
    char sql[512];
    char **result;
    sprintf(sql, "select * from user where name = '%s' and pass = '%s';", msg->name, msg->data);
    printf("sql = %s\n", sql);
    if (sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg) != SQLITE_OK) {
        //��ѯʧ��,�û�������
        printf("%s\n", errmsg);
        return -1;
    }
    if (nrow == 1) {
        //��ѯ�ɹ�
        if (strcmp(msg->name, "root") == 0)
            Userflag = 1;
        else
            Userflag = 0;
        strcpy(msg->data, "OK!");
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
        if (ret < 0) { //����ʧ��
            ErrExit("send");
            return -1;
        }
        return 1;
    } else {
        //������û�������
        strcpy(msg->data, "User does not exist");
        //��ͻ��˷������ݣ���ʾע��ɹ�����ʧ��
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
        if (ret < 0) { //����ʧ��
            ErrExit("send");
            return -1;
        }
    }
    return 0;
}

int do_query(int sockfd, MSG *msg, sqlite3 *db) {
    int ret;
    char word[64];
    int found = 0;
    char date[128];
    char sql[512];
    char *errmsg;
    strcpy(word, msg->data);//���浥��
    found = do_searchword(sockfd, msg, word);//���ҵ���
    if (found == 1) {//�ҵ����ʣ����û�����ʱ��͵��ʲ��뵽��ʷ��¼����
        get_date(date);
        sprintf(sql, "insert into record values('%s', '%s', '%s');", msg->name, date, word);
        printf("sql = %s\n", sql);
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
            //����ʧ��
            printf("%s\n", errmsg);
            return -1;
        } else {
            //����ɹ�
            printf("Insert success\n");
        }
    } else { //δ�ҵ�����
        strcpy(msg->data, "Not found");
    }
    //��ͻ��˷������ݣ���ʾע��ɹ�����ʧ��
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //����ʧ��
        ErrExit("send");
        return -1;
    }
    return 0;
}

/*���ҵ���*/
int do_searchword(int sockfd, MSG *msg, char word[]) {
    //���ļ�
    FILE *fp;
    char temp[512] = {};
    int result;
    char *p;
    int len = strlen(word);
    //���ļ�
    fp = fopen(FILENAME, "r");
    if (fp == NULL) {
        strcpy(msg->data, "Failed open to dict.txt");
        SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
        ErrExit("fopen");
        return -1;
    }
    printf("word = %s , len = %d\n", word, len);
    //��ȡ�ļ�����ѯ����
    while ((fgets(temp, 512, fp)) != NULL) {
        if (strncmp(temp, word, len) == 0 && (temp[len] == ' ' || temp[len] == '\n')) {
            // �ҵ����ʣ�����ע��
            p = temp + len;
            while (*p == ' ') {
                p++;
            }
            strcpy(msg->data, p);
            printf("find word= %s\n", msg->data);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

/*��ȡʱ��*/
int get_date(char *date) {
    time_t t;
    struct tm *tp;
    time(&t);
    tp = localtime(&t);
    sprintf(date, "[%d-%d-%d %d:%d:%d]", tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min,
            tp->tm_sec);
    return 0;
}

/*��ѯ��ʷ��¼*/
int do_history(int sockfd, MSG *msg, sqlite3 *db) {
    char sql[512];
    char *errmsg;
    char **result;
    int nrow;//����
    int ncolumn;//����
    int ret, index;
    if (Userflag == 1) {
        sprintf(sql, "select * from record");
    } else {
        sprintf(sql, "select * from record where name = '%s'", msg->name);
    }
    if (sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg) != SQLITE_OK) { // ִ��sql���
        printf("%s\n", errmsg);
        return -1;
    } else {
        printf("Query record done.\n");
    }
    // ���msg->data
    strcpy(msg->data, "");
// ������ӱ���
    if (Userflag == 1) {
        // ������ӱ���
        sprintf(msg->data, "%-20s  %-20s  %-20s\n", result[0], result[1], result[2]);
        // ���ÿ������
        for (int i = 1; i <= nrow; i++) {
            index = i * ncolumn;
            sprintf(msg->data + strlen(msg->data), "%-20s  %-20s  %-20s\n", result[index], result[index + 1],
                    result[index + 2]);
        }
    } else {
        // ������ӱ���
        sprintf(msg->data, "%-20s    %-20s\n", result[1], result[2]);
        // ���ÿ������
        for (int i = 1; i <= nrow; i++) {
            index = i * ncolumn;
            sprintf(msg->data + strlen(msg->data), "%-20s    %-20s\n", result[index + 1], result[index + 2]);
        }
    }
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //����ʧ��
        ErrExit("send");
        return -1;
    }
    //��ʷ��¼������ϣ�����over
    msg->data[0] = '\0';
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //����ʧ��
        ErrExit("send");
        return -1;
    }
    return 0;
}
