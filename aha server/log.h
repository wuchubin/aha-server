#ifndef _LOG_H_
#define _LOG_H_
#include <syslog.h>

#define SERVER_NAME "aha server"


class Log
{
public:
	//打开日志
	Log();
	//关闭日志
	~Log();
	//往日志文件/var/log/messages写入记录
	void write(const char *message);
};




//测试专用
/*

	Log log;
	log.write("Let us do a test!");
*/




#endif