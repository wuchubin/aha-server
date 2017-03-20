#ifndef  _HTTP_CONN_H_
#define _HTTP_CONN_H_

#include <sys/sendfile.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "mempool.h"

//文件名最大长度
#define FILENAME_LEN 100



//设置套接字描述符为非阻塞模式
int setnoblocking(int fd);
//往epoll中注册fd，如果one_shot为true，则只监听一次fd事件
void addfd(int epollfd, int fd, bool one_shot);
//从epoll中注销一个fd
void removefd(int epollfd,int fd);
//在epoll中修改fd的监听事件,如果one_shot为true，则只修改一次
void modfd(int epollfd,int fd,int ev,bool one_shot = false);



class Http_conn
{
public:
	//监听套接字描述符 
	static int epollfd;
	//内存池
	static Mempool mempool;
	//服务器当前用户数目
	static int user_count;
	//接收缓冲区大小
	static int readbuf_size;
	//发送缓冲区大小
	static int writebuf_size;
	//服务器工作目录
	static char docroot[100];
	//服务器支持的方法
	enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, OPTIONS};
	//请求报文的解析状态，CHECK_STATE_REQUESTLINE表示正要解析请求行，CHECK_STATE_HEADER表示正要解析首部，CHECK_STATE_CONTENT表示正要解析主体
	enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
	//HTTP标识码
    enum HTTP_CODE 
    { 
    	//非请求报文
    	NO_REQUEST,	
    	//GET请求	 
    	GET_REQUEST,
    	//请求报文格式出错
    	BAD_REQUEST, 
    	//没有该请求资源
    	NO_RESOURCE, 
    	//客户未获得管理权限
    	FORBIDDEN_REQUEST, 
    	//文件请求
    	FILE_REQUEST,
    	//服务器内部错误
    	INTERNAL_ERROR, 
    	//关闭连接
    	CLOSED_CONNECTION 
    };
    //请求行解析状态，如果正在解析请求行的某一行但并未解析到该行末尾，则为LINE_OPEN，解析成功当前行，则为LINE_OK，若该行格式有问题，则为LINE_BAD
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
	//初始化连接
	void init(int fd,const sockaddr_in &addr);
	//处理请求
	void process();
	//读取请求报文
	bool read();
	//发送响应报文
	bool write();
	//关闭连接
	void close_conn();


private:
	//url解码
	void decode(char *p);
	//重置连接
	void reset();
	//从接收缓冲区获取一行
	char *get_line() {return read_buf + start_line;}
	//判断是否是完整的一行
	LINE_STATUS parse_line(); 
	//解析起始行
	HTTP_CODE parse_request_line(char* text);
	//解析首部
	HTTP_CODE parse_headers(char* text);
	//解析主体
	HTTP_CODE parse_content(char *text);
	//解析请求报文
	HTTP_CODE process_read();
	//处理请求
	HTTP_CODE do_request();
	//处理get请求
	HTTP_CODE do_get();
	//处理post请求
	HTTP_CODE do_post();
	//处理head请求
	HTTP_CODE do_head();
	//处理delete请求
	HTTP_CODE do_delete();
	//处理put请求
	HTTP_CODE do_put();
	//构造响应报文
	bool process_write(HTTP_CODE ret);
	//填充相应报文
	bool add_response(const char *format, ... );
	//添加起始行
	bool add_status_line(int status,const char *title);
	//添加首部
	bool add_headers(int content_len);
	bool add_allow();
	//添加content-length首部
	bool add_content_length(int conten_len);
	//添加content_language首部
	bool add_content_language();
	//添加content_type首部
	bool add_content_type();
	//添加connection首部
	bool add_linger();
	//添加空白行
	bool add_blank_line();
	//添加主体
	bool add_content(const char *content);

private:
	//连接套接字
	int sockfd;
	//用户套接字地址
	sockaddr_in user_address;
	//指向接收缓冲区
	char *read_buf;
	//用于接收缓冲区的内存块
	memblock *read_block;
	//接收缓冲区已读下标索引
	int read_index;
	//接收缓冲区已处理下标索引
	int check_index;
	int start_line;
	CHECK_STATE check_state;
	//方法
	METHOD method;
	//请求的url
	char *url;
	//http版本
	char *version;
	//请求中是否带参数
	bool have_parameters;
	//主机
	char *host;
	//指向主体
	char *content;
	//主体长度
	size_t content_length;
	//是否保持长连接
	bool linger;
	//指向写缓冲区
	char *write_buf;
	//用于写缓冲区的内存块
	memblock *write_block;
	//写缓冲区已写下标
	int write_index;
	//文件名
	char real_file[FILENAME_LEN];
	//用于判断put请求时上传文件是否已存在，如果存在则为true，否则为false
	bool file_exist;
	//用于记录real_file的文件信息
	struct stat file_stat;

};

#endif
