#include "log.h"




//打开日志
Log::Log()
{
	openlog(SERVER_NAME, LOG_CONS | LOG_PID, 0);
}




//关闭日志
Log::~Log()
{
	closelog();
}



//往日志文件/var/log/messages写入记录
void Log::write(const char *message)
{
	syslog(LOG_INFO,message);
}
