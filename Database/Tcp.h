//
// Created by k on 23-12-16.
//

#ifndef DATABASE_TCP_H
#define DATABASE_TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

#define Debug 0
#define BACKLOG 5
#define ErrExit(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define  N 32
#define R 1 //ע��
#define L 2 //��¼
#define Q 3 //��ѯ
#define H 4 //��ʷ��¼
typedef struct {
    int type;
    char name[N];
    char data[430];
} MSG;

typedef struct sockaddr Addr;
typedef struct sockaddr_in Addr_in;

typedef int (*func_t)(int, const Addr *, socklen_t);//����ָ��
typedef ssize_t (*DataHandle_t)(int, void *, size_t, int);//����ָ��

void Argment(int argc, char *argv[]);//������

int SocketInit(char *argv[], bool server);//�����׽��֣��������������󶨵�ַ

int SocketDataHandle(int fd, void *buf, size_t len, DataHandle_t dataHandle);//���ջ�������


#endif //DATABASE_TCP_H
