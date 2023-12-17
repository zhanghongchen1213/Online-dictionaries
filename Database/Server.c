#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <signal.h>
#include "Tcp.h"
#include <time.h>

#define DATABASE "my.db"
#define FILENAME "/home/k/Linux_Learn/Database/dict.txt"
int Userflag = 0;//root登录标志

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
    /*检查参数*/
    Argment(argc, argv);
    /*打开数据库*/
    if (sqlite3_open(DATABASE, &db) != SQLITE_OK) {
        printf("sqlite3_open: %s\n", sqlite3_errmsg(db));
        return -1;
    } else
        printf("sqlite3_open success\n");
    /*创建套接字*/
    fd = SocketInit(argv, true);
    /*处理僵尸进程*/
    signal(SIGCHLD, SIG_IGN);//忽略SIGCHLD信号
    /*接受客户端连接*/
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
        } else if (pid == 0) {//子进程，处理客户端数据
            close(fd);//关闭父进程的套接字
            do_client(newfd, client_addr, db);
            exit(0);
        } else //父进程，继续接受客户端连接
            close(newfd);
    }
    close(fd);
    return 0;
}

/*处理客户端数据*/
void do_client(int sockfd, Addr_in client_addr, sqlite3 *db) {
    MSG msg;
    int ret;
    while (1) {
        int ret = SocketDataHandle(sockfd, &msg, sizeof(MSG), recv);
        if (ret == 0) {//客户端强制退出
            printf("[%s:%d]:client out\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            break;
        } else {//客户端正常发送数据
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
        //插入失败
        printf("%s\n", errmsg);
        strcpy(msg->data, "User already exists");
    } else {
        //插入成功
        printf("Insert success\n");
        strcpy(msg->data, "OK!");
    }
    //向客户端发送数据，提示注册成功或者失败
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //发送失败
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
        //查询失败,用户不存在
        printf("%s\n", errmsg);
        return -1;
    }
    if (nrow == 1) {
        //查询成功
        if (strcmp(msg->name, "root") == 0)
            Userflag = 1;
        else
            Userflag = 0;
        strcpy(msg->data, "OK!");
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
        if (ret < 0) { //发送失败
            ErrExit("send");
            return -1;
        }
        return 1;
    } else {
        //密码或用户名错误
        strcpy(msg->data, "User does not exist");
        //向客户端发送数据，提示注册成功或者失败
        ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
        if (ret < 0) { //发送失败
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
    strcpy(word, msg->data);//保存单词
    found = do_searchword(sockfd, msg, word);//查找单词
    if (found == 1) {//找到单词，将用户名和时间和单词插入到历史记录表中
        get_date(date);
        sprintf(sql, "insert into record values('%s', '%s', '%s');", msg->name, date, word);
        printf("sql = %s\n", sql);
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
            //插入失败
            printf("%s\n", errmsg);
            return -1;
        } else {
            //插入成功
            printf("Insert success\n");
        }
    } else { //未找到单词
        strcpy(msg->data, "Not found");
    }
    //向客户端发送数据，提示注册成功或者失败
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //发送失败
        ErrExit("send");
        return -1;
    }
    return 0;
}

/*查找单词*/
int do_searchword(int sockfd, MSG *msg, char word[]) {
    //打开文件
    FILE *fp;
    char temp[512] = {};
    int result;
    char *p;
    int len = strlen(word);
    //打开文件
    fp = fopen(FILENAME, "r");
    if (fp == NULL) {
        strcpy(msg->data, "Failed open to dict.txt");
        SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
        ErrExit("fopen");
        return -1;
    }
    printf("word = %s , len = %d\n", word, len);
    //读取文件，查询单词
    while ((fgets(temp, 512, fp)) != NULL) {
        if (strncmp(temp, word, len) == 0 && (temp[len] == ' ' || temp[len] == '\n')) {
            // 找到单词，处理注释
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

/*获取时间*/
int get_date(char *date) {
    time_t t;
    struct tm *tp;
    time(&t);
    tp = localtime(&t);
    sprintf(date, "[%d-%d-%d %d:%d:%d]", tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min,
            tp->tm_sec);
    return 0;
}

/*查询历史记录*/
int do_history(int sockfd, MSG *msg, sqlite3 *db) {
    char sql[512];
    char *errmsg;
    char **result;
    int nrow;//行数
    int ncolumn;//列数
    int ret, index;
    if (Userflag == 1) {
        sprintf(sql, "select * from record");
    } else {
        sprintf(sql, "select * from record where name = '%s'", msg->name);
    }
    if (sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg) != SQLITE_OK) { // 执行sql语句
        printf("%s\n", errmsg);
        return -1;
    } else {
        printf("Query record done.\n");
    }
    // 清空msg->data
    strcpy(msg->data, "");
// 首先添加标题
    if (Userflag == 1) {
        // 首先添加标题
        sprintf(msg->data, "%-20s  %-20s  %-20s\n", result[0], result[1], result[2]);
        // 添加每行数据
        for (int i = 1; i <= nrow; i++) {
            index = i * ncolumn;
            sprintf(msg->data + strlen(msg->data), "%-20s  %-20s  %-20s\n", result[index], result[index + 1],
                    result[index + 2]);
        }
    } else {
        // 首先添加标题
        sprintf(msg->data, "%-20s    %-20s\n", result[1], result[2]);
        // 添加每行数据
        for (int i = 1; i <= nrow; i++) {
            index = i * ncolumn;
            sprintf(msg->data + strlen(msg->data), "%-20s    %-20s\n", result[index + 1], result[index + 2]);
        }
    }
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //发送失败
        ErrExit("send");
        return -1;
    }
    //历史记录发送完毕，发送over
    msg->data[0] = '\0';
    ret = SocketDataHandle(sockfd, msg, sizeof(MSG), (DataHandle_t) send);
    if (ret < 0) { //发送失败
        ErrExit("send");
        return -1;
    }
    return 0;
}
