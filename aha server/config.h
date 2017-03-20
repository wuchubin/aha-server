#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#define CONFIG_FILE "server.conf"

class Config
{
public:

	//读取配置信息
	Config();
	~Config();

	//返回服务器ip地址 
	const char *get_ip(){
		return ip;
	}
	//返回服务器端口号
	int get_port(){
		return port;
	}
	//返回服务器的工作目录
	char *get_docroot(){
		return docroot;
	}
	//返回工作线程数目
	int get_worker_num(){
		return worker_num;
	}
	//返回内存块数目
	size_t get_memblock_num(){
		return memblock_num; 
	}
	//返回内存块大小
	size_t get_memblock_size(){
		return memblock_size;
	}
	//返回请求队列的最大大小
	size_t get_rq_max_size() {
		return rq_max_size;
	}

private:
	//ip地址
	char * ip;
	//端口号
	int port;
	//工作目录
	char * docroot;
	//工作线程数目
	int worker_num;
	//初始内存块数目
	size_t memblock_num;
	//内存块大小
	size_t memblock_size;
	//用户请求队列大小
	size_t rq_max_size;
};






#endif