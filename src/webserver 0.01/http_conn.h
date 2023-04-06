#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include "locker.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

class http_conn {
public:
	static const int FILENAME_LEN = 200;       // 文件名最大长度
	static const int READ_BUFFER_SIZE = 2048;  // 读缓冲区的大小
	static const int WRITE_BUFFER_SIZE = 1024; // 写缓冲区的大小

	// HTTP请求方法，此处仅实现对于 GET 的支持
	enum METHOD {
		GET = 0,
		POST,
		HEAD,
		PUT,
		DELETE,
		TRACE,
		OPTIONS,
		CONNECT,
		PATCH
	};

	// HTTP请求时主机所处状态
	enum CHECK_STATE {
		CHECK_STATE_REQUESTLINE = 0,
		CHECK_STATE_HEADER,
		CHECK_STATE_CONTENT,
	};
	// 服务器处理 HTTP 请求的结果
	// NO_REQUEST 请求不完整，需要继续读取客户端
	// GET_REQUEST 获得了一个完整的客户请求
	// BAD_REQUEST 客户请求有语法错误
	// NO_RESOURCE 服务端无该资源可用
	// FORBIDDEN_REQUEST 客户对资源没有足够访问权限
	// FILE_REQUEST 文件资源请求
	// INTERNAL_ERROR 服务器内部错误
	// CLOSED_CONNECTION 客户端连接已关闭
	enum HTTP_CODE {
		NO_REQUEST,
		GET_REQUEST,
		BAD_REQUEST,
		NO_RESOURCE,
		FORBIDDEN_REQUEST,
		FILE_REQUEST,
		INTERNAL_ERROR,
		CLOSED_CONNECTION
	};

	// 行读取状态
	// 读到一个完整行，行出错，行数据尚不完整
	enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
	http_conn() {}
	~http_conn() {}

public:
	void init(int sockfd, const sockaddr_in& addr); // 初始化新接受的连接
	void close_conn(bool real_close = true);        // 关闭连接
	void process();                                 // 处理客户请求
	bool read();                                    // 非阻塞读操作
	bool write();                                   // 非阻塞写操作

private:
	void init();                       // 初始化连接
	HTTP_CODE process_read();          // 解析 HTTP请求
	bool process_write(HTTP_CODE ret); // 填充 HTTP应答

	// process_read 调用以下函数分析 HTTP 请求
	HTTP_CODE parse_request_line(char* text);
	HTTP_CODE parse_headers(char* test);
	HTTP_CODE parse_content(char* test);
	HTTP_CODE do_request();
	char* get_line() { return m_read_buf + m_start_line; }
	LINE_STATUS parse_line();

	// process_write 调用以下函数以填充 HTTP 问答
	void unmap();
	bool add_response(const char* format, ...);
	bool add_content(const char* content);
	bool add_status_line(int status, const char* title);
	bool add_headers(int content_length);
	bool add_content_length(int content_length);

	bool add_linger();
	bool add_blank_line();

public:
	// 所有 socket 上事件注册到同一个 epoll 内核事件表
	// 将 epoll 文件描述符设置为静态的
	static int m_epollfd;

	// 统计用户数量
	static int m_user_count;

private:
	// HTTP连接 socket 和对方 socket 地址
	int m_sockfd;
	sockaddr_in m_address;

	char m_read_buf[READ_BUFFER_SIZE];      // 读缓冲区
	int m_read_idx;         // 标识读缓冲区已读入客户数据的最后一个字节的下一个位置
	int m_checked_idx;      // 当前正在分析字符在读缓冲区内的位置
	int m_start_line;       // 当前正在解析的行起始位置

	char m_write_buf[WRITE_BUFFER_SIZE];    // 写缓冲区
	int m_write_idx;        // 写缓冲区中待发送字节数

	CHECK_STATE m_check_state;  // 主状态机状态
	METHOD m_method;        // 请求方法

	char m_real_file[FILENAME_LEN];     // 客户请求文件路径
	char* m_url;        // 客户请求文件文件名

	char* m_version;    // HTTP 协议版本号，此处支持 HTTP/1.1
	char* m_host;       // 主机名
	int m_content_length;   // HTTP请求消息的长度
	bool m_linger;      // HTTP请求是否要求保持连接

	char* m_file_address;   // 客户端请求目标文件 mmap 到内存的起始位置
	struct stat m_file_stat;    // 目标文件状态

	// writev 执行写操作
	struct iovec m_iv[2];
	int m_iv_count;

};

#endif