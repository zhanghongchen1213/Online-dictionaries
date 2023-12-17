# 定义编译器
CC=gcc

# 定义编译选项
CFLAGS=-I.

# 定义链接选项
LDFLAGS=-lsqlite3

# 默认目标
all: Client Server
	rm -f *.o

# 编译client
Client: Client.o Tcp.o
	$(CC) -o Client Client.o Tcp.o $(CFLAGS)

# 编译server
Server: Server.o Tcp.o
	$(CC) -o Server Server.o Tcp.o $(CFLAGS) $(LDFLAGS)

# 生成对象文件
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)