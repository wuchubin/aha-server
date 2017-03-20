#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <signal.h>

#include "config.h"
#include "log.h"
#include "threadpool.h"
#include "http_conn.h"

using namespace std;

#define MAX_FD 65536
#define MAX_EVENT_NUM 10000
int main( int argc, char* argv[] )
{
	Config config;
	//初始化内存池
	Http_conn::mempool.init(config.get_memblock_num(), config.get_memblock_size());
	//设置接收缓冲区大小 
	Http_conn::readbuf_size = config.get_memblock_size();
	//设置发送缓冲区大小
	Http_conn::writebuf_size = config.get_memblock_size();
	//设置服务器工作目录 ./docroot
	strcpy(Http_conn::docroot,".");
	strcat(Http_conn::docroot,config.get_docroot());
	

	Http_conn *users = new Http_conn[MAX_FD];
	//创建线程池
	Threadpool <Http_conn >threadpool(config.get_worker_num(), config.get_rq_max_size());
	//忽略SIGPIPE，SIGPIPE的默认动作是杀死进程
	signal(SIGPIPE,SIG_IGN);

	//创建监听套接字
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	//设置IP地址
	inet_pton(AF_INET, config.get_ip(), &address.sin_addr);
	address.sin_port = htons(config.get_port());
	int option = 1;
	//设置TIME_WAIT状态，方便服务器快速重启
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    //绑定监听套接字和相应的套接字地址结构
	bind(listenfd, (struct sockaddr *)&address, sizeof(address));
	//设置正在等待接收的连接数为5
	listen(listenfd, 5);
	epoll_event events[MAX_EVENT_NUM];
    int epollfd = epoll_create(5);
    Http_conn::epollfd = epollfd;
	addfd(epollfd, listenfd, false);
	while(true)
	{
		int num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
		if((num < 0) && (errno != EINTR))
		{
			printf("epoll failure\n");
			break;
		}
		for(int i = 0;i < num; i ++)
		{
			int sockfd = events[i].data.fd;
			if(sockfd == listenfd)
			{
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
				int connfd;
				while((connfd = accept(listenfd,(struct sockaddr *)&client_address, &client_addrlength)) > 0)
				{
					if(Http_conn::user_count >= MAX_FD)
					{
						printf("Server is to busy!");
						break;
					}
					users[connfd].init(connfd, client_address);
					printf("user_count:%d\n",Http_conn::user_count);
				}
				if(connfd < 0)
				{
					if (errno != EAGAIN  && errno != EINTR)     
						printf("accept error!\n");
				}
			}
			else if( events[i].events & ( EPOLLRDHUP | EPOLLHUP | EPOLLERR ) )
           	{//文件描述符被挂断或者发生错误，关闭连接
           		printf("up..............\n");
               	users[sockfd].close_conn();
            }
           	else if( events[i].events & EPOLLIN )
            {
            	printf("reading..............\n");
            	//读取请求报文
          	   	if( users[sockfd].read() )
             	{   //添加到用户请求队列
                	threadpool.append( users + sockfd );
            	}
            	else
            	{//读取失败，关闭连接
            	   users[sockfd].close_conn();
            	}
           	}
           	else if( events[i].events & EPOLLOUT )
            {
            	//发送响应报文
            	if( !users[sockfd].write() )
           		{//关闭连接
            		printf("close conn\n");
            		users[sockfd].close_conn();
            	}
            }
           	
		}
	}
	return 0;
}
