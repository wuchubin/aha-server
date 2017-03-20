#include "http_conn.h"


const char* ok_200_title = "OK";
const char* ok_201_title = "Create";
const char *ok_204_title = "No Content";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";


//初始化类的静态成员
int Http_conn::epollfd = -1;
int Http_conn::user_count = 0;
Mempool Http_conn::mempool;
int Http_conn::readbuf_size = -1;
int Http_conn::writebuf_size = -1;
char Http_conn::docroot[100] = "";


//设置套接字描述符为非阻塞模式
int setnoblocking(int fd)
{
	int val = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, val | O_NONBLOCK);
	return val;
}

//往epoll中注册fd，如果one_shot为true，则只监听一次fd事件
void addfd(int epollfd, int fd, bool one_shot)
{
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	if(one_shot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnoblocking(fd);
}


//从epoll中注销一个fd
void removefd(int epollfd,int fd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}


//在epoll中修改fd的监听事件,如果one_shot为true，则只修改一次
void modfd(int epollfd,int fd,int ev,bool one_shot)
{
	epoll_event event;
	event.data.fd = fd;
	event.events = ev | EPOLLET | EPOLLRDHUP;
	if(one_shot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

//url解码
void Http_conn::decode(char *p)
{
	//编码过程为:将一个字符转换为其对应ascli码的十六进制，并在前面加上一个'%'，同时' '会被转换成'+'
	//解码过程为:将'%'去掉，并将十六进制转换成十进制,'+'转换成' '
	//由于大小写字符相差0x20,所以原字符&上0xDF即可转换成小写字母
  	int i = 0;
  	while(*(p+i) != '\0')
  	{
  		if((*p = *(p+i)) == '%')
  		{
  			*p = *(p+i+1) >= 'A' ? (*(p+i+1) & 0xDF) - 'A' + 10 : (*(p+i+1)) - '0';
  			*p *= 16;
  			*p += *(p+i+2) >= 'A' ? (*(p+i+2) & 0xDF) - 'A' + 10 : (*(p+i+2)) - '0';
  			i += 2;
  		}
  		else if(*p == '+')
  		{
  			*p = ' ';
  		}
  		p ++;
  	}
   	*p = '\0';
}


//初始化连接
void Http_conn::init(int fd, const sockaddr_in &addr)
{
	sockfd = fd;
	user_address = addr;
	read_block = mempool.mempool_take_mblocks_by_num(1);
	write_block = mempool.mempool_take_mblocks_by_num(1);
	read_buf = (char *)read_block->data;
	write_buf = (char *)write_block->data;
	read_index = 0;
	write_index = 0;
	check_index = 0;
	start_line = 0;
	check_state = CHECK_STATE_REQUESTLINE;
	method = GET;
	url = NULL;
	version = NULL;
	have_parameters = false;
	host = NULL;
	content = NULL;
	content_length = 0;
	linger = false;
	file_exist = false;
	user_count ++;
	addfd(epollfd, sockfd, true);
}

//重置连接
void Http_conn::reset()
{
	read_index = 0;
	write_index = 0;
	check_index = 0;
	start_line = 0;
	check_state = CHECK_STATE_REQUESTLINE;
	method = GET;
	url = NULL;
	version = NULL;
	host = NULL;
	have_parameters = false;
	content = NULL;
	content_length = 0;
	linger = false;
	file_exist = false;
	modfd(epollfd,sockfd,EPOLLIN,false);
}

//关闭连接
void Http_conn::close_conn()
{
	removefd(epollfd,sockfd);
	mempool.mempool_put_mblocks(read_block);
	mempool.mempool_put_mblocks(write_block);
	user_count --;
}

//读取请求报文
bool Http_conn::read()
{
	if(read_index > readbuf_size)
		return false;
	int bytes_read = 0;
	while(true)
	{printf("mmmm\n");
		bytes_read = recv(sockfd,read_buf + read_index,readbuf_size - read_index, 0);
		if(bytes_read == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else if(errno == EINTR)
				continue;
			else
				return false;
		}
		else if(bytes_read == 0)
		{
			return false;
		}
		read_index += bytes_read;
		read_buf[bytes_read] = '\0';printf("%s",read_buf);
	}
	return true;
}



//判断是否是完整的一行
Http_conn::LINE_STATUS Http_conn::parse_line()
{
	char temp;
	for(;check_index < read_index; ++check_index)
	{
		temp = read_buf[check_index];
		if(temp == '\r')
		{
			if(check_index + 1 < read_index && read_buf[check_index+1] == '\n')
			{
				read_buf[check_index] = '\0';
				check_index += 2;
				return LINE_OK;
			}
			else
				return LINE_BAD;
		}
		else if(temp == '\n')
		{
			read_buf[check_index ++] = '\0';
			return LINE_OK;
		}
	}
	return LINE_OPEN;
}

//解析起始行
Http_conn::HTTP_CODE Http_conn::parse_request_line(char* text)
{
	url = strpbrk(text," \t");
	if(!url)
		return BAD_REQUEST;
	*url ++ = '\0';
	char *method_ptr = text;
	if(strcasecmp(method_ptr,"GET") == 0)
		method = GET;
	else if(strcasecmp(method_ptr,"POST") == 0)
		method = POST;
	else if(strcasecmp(method_ptr,"HEAD") == 0)
		method = HEAD;
	else if(strcasecmp(method_ptr,"PUT") == 0)
		method = PUT;
	else if(strcasecmp(method_ptr,"DELETE") == 0)
		method = DELETE;
	else if(strcasecmp(method_ptr,"OPTIONS") == 0)
		method = OPTIONS;
	else 
		return BAD_REQUEST;
	version = strpbrk(url," \t");
	*version ++ = '\0';

	if(strcasecmp(version,"HTTP/1.1") != 0)
		return BAD_REQUEST;

	if(strncasecmp(url,"http://",7) == 0)
	{
		url += 7;
		url = strchr(url,'/');
	}
	if(!url || (url[0] != '*' && url[0] != '/'))
	{
		return BAD_REQUEST;
	}
	
	check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
}

//解析首部
Http_conn::HTTP_CODE Http_conn::parse_headers(char* text)
{
	if(text[0] == '\0')
	{
		if(content_length != 0)
		{
			check_state = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		return GET_REQUEST;
	}
	else if(strncasecmp(text,"Connection:",11) == 0)
	{
		text += 11;
		text += strspn(text," \t");
		if(strcasecmp(text,"keep-alive") == 0)
			linger = true;
	}
	else if(strncasecmp(text,"content-Length:",15) == 0)
	{
		text += 15;
		text += strspn(text," \t");
		content_length = atol(text);
	}
	else if(strncasecmp(text,"Host:",5) == 0)
	{
		text += 5;
		text += strspn(text," \t");
		host = text;
	}
	
	return NO_REQUEST;
}

//解析主体
Http_conn::HTTP_CODE Http_conn::parse_content(char *text)
{
	if(read_index == content_length + check_index)
	{
		text[content_length] = '\0';
		content = text;
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

//解析请求报文
Http_conn::HTTP_CODE Http_conn::process_read()
{
	LINE_STATUS line_state = LINE_OK;
	char *text = NULL;
	HTTP_CODE ret;
	while((check_state == CHECK_STATE_CONTENT && line_state == LINE_OK) || (line_state = parse_line()) == LINE_OK)
	{
		text = get_line();
		start_line = check_index;
		switch(check_state)
		{
			case CHECK_STATE_REQUESTLINE:
			{
				ret = parse_request_line(text);
				if(ret == BAD_REQUEST)
					return BAD_REQUEST;
				break;
			}
			case CHECK_STATE_HEADER:
			{ 
				ret = parse_headers(text);
				if(ret == BAD_REQUEST)
					return BAD_REQUEST;
				else if(ret == GET_REQUEST)
					return do_request();
				break;
			}
			case CHECK_STATE_CONTENT:
			{
				if(method == PUT)
					return do_put();
				ret = parse_content(text);
				if(ret == GET_REQUEST)
					return do_request();
				line_state = LINE_OPEN;
				break;
			}
			default:
			{
				return INTERNAL_ERROR;
			}
		}
	}
	return NO_REQUEST;
}

//处理get请求
Http_conn::HTTP_CODE Http_conn::do_get()
{
	decode(url);
	char *ptr = strchr(url,'?');
	//1.如果是带参数的GET请求，提取出url中的参数封装成$_GET数组，格式为<?php $_GET["key"]="value";?>
	//2.新建一个临时的php文件,将$_GET数组写入新建的临时php文件,这里称之为temp_file
	//3.注意:服多个用户可能同时请求执行同个php文件，未防止服务器多个线程同时操作同个php临时文件造成错误，所以该临时文件命名格式为:primary_file.pthreadid
	//4.将primary_file中的内容追加到临时的php文件后面
	//5.执行该临时文件,并将结果保存到一个临时的txt文件,文件名格式为temp_file.txt；
	//6.删除temp_file
	//7.temp_file.txt将一直保存直到服务器发送完响应报文，再将其删除
	if(ptr != NULL) 
	{
		*ptr = '\0';
		have_parameters = true;

		//防止非法用户访问非docroot目录里面的文件或者url里面
		if(strstr(url,"/..") == url || strchr(url,'*') != NULL) 
			return FORBIDDEN_REQUEST;

		//初始化primary_file
		char primary_file[100];
		strcpy(primary_file,docroot);
		strcat(primary_file,url);

		printf("primary_file:%s\n",primary_file);

		//判断文件是否存在
		struct stat primary_file_stat;
		if(stat(primary_file,&primary_file_stat) < 0)
			return NO_RESOURCE;
		//判断文件是否可读
		if(!(primary_file_stat.st_mode & S_IROTH))
			return FORBIDDEN_REQUEST;
		//判断该文件是否为目录文件
		if(S_ISDIR(primary_file_stat.st_mode))
			return BAD_REQUEST;

		//申请一块临时的内存块
		memblock *temp_block = mempool.mempool_take_mblocks_by_num(1);
		char *get_array_str = (char *)temp_block->data;
		int index = 0;


/*-----------------------------------开始构造$_GET数组----------------------------------------*/
		//php标签头
		char tag_start[10] = "<?php ";
		//php标签尾
		char tag_end[10] = "?>";

		//往get_array_str追加"<?php "
		for(int i = 0;tag_start[i] != '\0';i ++)
		{
			get_array_str[index ++] = tag_start[i];
		}

		//key开头部分
		char key_start[10] = "$_GET[\"";
		//key结尾部分
		char key_end[10] = "\"]=\"";

		//往get_array_str追加"$_GET[\""
		for(int i = 0;key_start[i] != '\0';i ++)
		{
			get_array_str[index ++] = key_start[i];
		}

		//提取出url里面的key-value对，'='作为key的结尾标志,'&'作为value的结尾标志同时也作为key的开始标志，'\0'作为value的结束标志
		bool flag = true;
		char ch;
		while(flag)
		{
			ch = *(++ptr);
			switch(ch)
			{
				case '=':
				{
					//往get_array_str追加"\"]=\""
					for(int i = 0;key_end[i] != '\0';i ++)
					{
						get_array_str[index ++] = key_end[i];
					}
					break;
				}
				case '&':
				{
					//往get_array_str追加“\;"
					get_array_str[index ++] = '\"';
					get_array_str[index ++] = ';';

					//往get_array_str追加"$_GET[\""
					for(int i = 0;key_start[i] != '\0';i ++)
					{
						get_array_str[index ++] = key_start[i];
					}
					break;
				}
				case '\0':
				{
					//往get_array_str追加"\"]=\""
					get_array_str[index ++] = '\"';
					get_array_str[index ++] = ';';

					//往get_array_str追加"？>"
					for(int i = 0;tag_end[i] != '\0';i ++)
					{
						get_array_str[index ++] = tag_end[i];
					}

					//往get_array_str追加"\n"
					get_array_str[index++] = '\n';

					get_array_str[index] = '\0';
					flag = false;
					break;
				}
				default:
				{
					//往get_array_str追加字符
					get_array_str[index ++] = ch;
				}
			}
		}
		printf("%s\n",get_array_str);
/*-----------------------------------$_GET数组构造完成----------------------------------------*/

/*-----------------------------创建临时的php文件,往里面填充内容-------------------------------*/
				
				//初始化文件名			
		char temp_file[100];				
		sprintf(temp_file,"%s%s.%lu",docroot,url,pthread_self());
		printf("temp_file:%s\n",temp_file);

		//创建文件
		FILE *ftemp_file = fopen(temp_file,"w");

		//写入$_GET数组
		int cnt, sum = 0;
		while(sum < index)
		{
			cnt = fwrite(get_array_str,1,index,ftemp_file);
			sum += cnt;
		}

		//将primary文件的内容追加到temp_file后面
		FILE *fprimary_file = fopen(primary_file, "r");
		int size = mempool.get_blocksize();
		char *buf = get_array_str;
		while((index = fread(buf,1,size,fprimary_file)) > 0)
		{
			sum = 0;
			while(sum < index)
			{
				cnt = fwrite(get_array_str,1,index,ftemp_file);
				sum += cnt;
			}
		}
		fclose(fprimary_file);
		fclose(ftemp_file);
		mempool.mempool_put_mblocks(temp_block);



		strcpy(real_file, temp_file);
		strcat(real_file,".txt");

		//构建cmd命令
		char cmd[100];
		strcpy(cmd,"php ");
		strcat(cmd,temp_file);
		strcat(cmd," > ");
		strcat(cmd,real_file);
	
		printf("real_file:%s\ncmd:%s\n",real_file,cmd);
	
		//执行
		FILE *stream = popen(cmd,"r");
		pclose(stream);

		//删除临时文件temp_file
		remove(temp_file);
		stat(real_file,&file_stat);
	} 
	else 
	{
		//防止非法用户访问非docroot目录里面的文件
		if(strstr(url,"/..") == url || strchr(url,'*') != NULL) 
			return FORBIDDEN_REQUEST;
	
		strcpy(real_file,docroot);
		if(strcmp(url,"/") == 0)
			strcat(real_file,"/index.html");
		else
			strcat(real_file,url);

		//判断文件是否存在
		if(stat(real_file,&file_stat) < 0)
		return NO_RESOURCE;
		//判断文件是否可读
		if(!(file_stat.st_mode & S_IROTH))
			return FORBIDDEN_REQUEST;
		//判断该文件是否为目录文件
		if(S_ISDIR(file_stat.st_mode))
			return BAD_REQUEST;
	}
	printf("real_file:%s\n",real_file);
	return FILE_REQUEST;	
}

//处理post请求
Http_conn::HTTP_CODE Http_conn::do_post()
{
	//1.提取出content中的参数封装成$_POST数组，格式为<?php $_POST["key"]="value";?>
	//2.新建一个临时的php文件,将$_POST数组写入新建的临时php文件,这里称之为temp_file
	//3.注意:服多个用户可能同时请求执行同个php文件，未防止服务器多个线程同时操作同个php临时文件造成错误，所以该临时文件命名格式为:primary_file.pthreadid
	//4.将primary_file中的内容追加到临时的php文件后面
	//5.执行该临时文件,并将结果保存到一个临时的txt文件,文件名格式为temp_file.txt；
	//6.删除temp_file
	//7.temp_file.txt将一直保存直到服务器发送完响应报文，再将其删除

	have_parameters = true;

	//防止非法用户访问非docroot目录里面的文件或url里面含有*
	if(strstr(url,"/..") == url || strchr(url,'*') != NULL) 
			return FORBIDDEN_REQUEST;


	//初始化primary_file
	char primary_file[100];
	strcpy(primary_file,docroot);
	strcat(primary_file,url);

	printf("primary_file:%s\n",primary_file);

	//判断文件是否存在
	struct stat primary_file_stat;
	if(stat(primary_file,&primary_file_stat) < 0)
		return NO_RESOURCE;
	//判断文件是否可读
	if(!(primary_file_stat.st_mode & S_IROTH))
		return FORBIDDEN_REQUEST;
	//判断该文件是否为目录文件
	if(S_ISDIR(primary_file_stat.st_mode))
		return BAD_REQUEST;

	//申请一块临时的内存块
	memblock *temp_block = mempool.mempool_take_mblocks_by_num(1);
	char *post_array_str = (char *)temp_block->data;
	int index = 0;

/*-----------------------------------开始构造$_POST数组----------------------------------------*/
	//php标签头
	char tag_start[10] = "<?php ";
	//php标签尾
	char tag_end[10] = "?>";

	//往post_array_str追加"<?php "
	for(int i = 0;tag_start[i] != '\0';i ++)
	{
		post_array_str[index ++] = tag_start[i];
	}

	//key开头部分
	char key_start[10] = "$_POST[\"";
	//key结尾部分
	char key_end[10] = "\"]=\"";

	//往post_array_str追加"$_POST[\""
	for(int i = 0;key_start[i] != '\0';i ++)
	{
		post_array_str[index ++] = key_start[i];
	}

	decode(content);
	bool flag = true;
	//提取出url里面的key-value对，'='作为key的结尾标志,'&'作为value的结尾标志同时也作为key的开始标志，'\0'作为value的结束标志
	while(flag)
	{
		switch(*content)
		{
			case '=':
			{
				//往post_array_str追加"\"]=\""
				for(int i = 0;key_end[i] != '\0';i ++)
				{
					post_array_str[index ++] = key_end[i];
				}
				break;
			}
			case '&':
			{
				//往post_array_str追加“\;"
				post_array_str[index ++] = '\"';
				post_array_str[index ++] = ';';

				//往post_array_str追加"$_POST[\""
				for(int i = 0;key_start[i] != '\0';i ++)
				{
					post_array_str[index ++] = key_start[i];
				}
				break;
			}
			case '\0':
			{
				//往post_array_str追加“\;"
				post_array_str[index ++] = '\"';
				post_array_str[index ++] = ';';

				//往post_array_str追加"？>"
				for(int i = 0;tag_end[i] != '\0';i ++)
				{
					post_array_str[index ++] = tag_end[i];
				}

				//往post_array_str追加"\n"
				post_array_str[index++] = '\n';

				post_array_str[index] = '\0';
				flag = false;
				break;
			}
			default:
			{
				//往post_array_str追加字符
				post_array_str[index ++] = *content;
			}
		}
		content ++;
	}
	printf("%s\n",post_array_str);
/*-----------------------------------$_POST数组构造完成----------------------------------------*/

			//初始化文件名
	char temp_file[100];			
	sprintf(temp_file,"%s%s.%lu",docroot,url,pthread_self());
	printf("temp_file:%s\n",temp_file);

	//创建文件
	FILE *ftemp_file = fopen(temp_file,"w");

	//写入$_POST数组
	int cnt, sum = 0;
	while(sum < index)
	{
		cnt = fwrite(post_array_str,1,index,ftemp_file);
		sum += cnt;
	}

	//将primary文件的内容追加到temp_file后面
	FILE *fprimary_file = fopen(primary_file, "r");
	int size = mempool.get_blocksize();
	char *buf = post_array_str;
	while((index = fread(buf,1,size,fprimary_file)) > 0)
	{
		sum = 0;
		while(sum < index)
		{
			cnt = fwrite(post_array_str,1,index,ftemp_file);
			sum += cnt;
		}
	}
	fclose(fprimary_file);
	fclose(ftemp_file);
	mempool.mempool_put_mblocks(temp_block);
			


	strcpy(real_file, temp_file);
	strcat(real_file,".txt");

	//构建cmd命令
	char cmd[100];
	strcpy(cmd,"php ");
	strcat(cmd,temp_file);
	strcat(cmd," > ");
	strcat(cmd,real_file);

	printf("real_file:%s\ncmd:%s\n",real_file,cmd);

	//执行
	FILE *stream = popen(cmd,"r");
	pclose(stream);

	//删除临时文件temp_file
	remove(temp_file);
	stat(real_file,&file_stat);
	return FILE_REQUEST;
}

//处理head请求
Http_conn::HTTP_CODE Http_conn::do_head()
{
	//防止非法用户访问非docroot目录里面的文件或者url里面含有*
	if(strstr(url,"/..") == url || strchr(url,'*') != NULL) 
			return FORBIDDEN_REQUEST;

	strcpy(real_file,docroot);
	if(strcmp(url,"/") == 0)
		return FORBIDDEN_REQUEST;
	else
		strcat(real_file,url);

	//判断文件是否存在
	if(stat(real_file,&file_stat) < 0)
		return NO_RESOURCE;
	//判断文件是否可读
	if(!(file_stat.st_mode & S_IROTH))
		return FORBIDDEN_REQUEST;
	//判断该文件是否为目录文件
	if(S_ISDIR(file_stat.st_mode))
		return BAD_REQUEST;
			
	return FILE_REQUEST;
}

//处理delete请求
Http_conn::HTTP_CODE Http_conn::do_delete()
{
	//防止非法用户访问非docroot目录里面的文件或者url里面含有*
	if(strstr(url,"/..") == url || strchr(url,'*') != NULL) 
			return FORBIDDEN_REQUEST;

	strcpy(real_file,docroot);
	if(strcmp(url,"/") == 0)
		return FORBIDDEN_REQUEST;
	else
		strcat(real_file,url);

	//判断文件是否存在
	if(stat(real_file,&file_stat) < 0)
		return NO_RESOURCE;
	//判断文件是否可有足够的权限删除
	if(!(file_stat.st_mode & S_IWUSR))
		return FORBIDDEN_REQUEST;
	if(remove(real_file) == -1)
		return INTERNAL_ERROR;
	return FILE_REQUEST;
}

//处理put请求
Http_conn::HTTP_CODE Http_conn::do_put()
{
	if(strstr(url,"/..") == url || strchr(url,'*') != NULL || strcmp(url,"/") == 0) 
		return INTERNAL_ERROR;
	strcpy(real_file,docroot);
	strcat(real_file,url);
	if(stat(real_file,&file_stat) == 0)
		file_exist = true;
	FILE *freal_file = fopen(real_file,"w");
	if(freal_file == NULL)
		return INTERNAL_ERROR;
	int sum = 0, cnt, index;
	while(sum < content_length)
	{
		if(read_index > readbuf_size)
			break;
		cnt = fwrite(read_buf+check_index, 1, read_index - check_index, freal_file);
		check_index += cnt;
		sum += cnt;
		if(check_index == read_index)
			break;
	}
	fclose(freal_file);
	return FILE_REQUEST;
}

//处理请求
Http_conn::HTTP_CODE Http_conn::do_request()
{
	switch(method)
	{
		case GET:
			return do_get();
		case POST:
			return do_post();
		case HEAD:
			return do_head();
		case DELETE:
			return do_delete();
	}

}


//填充相应报文
bool Http_conn::add_response(const char *format, ... )
{
	if(write_index >= writebuf_size)
	{
		return false;
	}
	va_list arg_list;
	va_start(arg_list, format);
	int len = vsnprintf(write_buf + write_index, writebuf_size - 1 - write_index, format, arg_list);
	if(len >= writebuf_size - 1 - write_index)
	{
		return false;
	}
	write_index += len;
	va_end(arg_list);
	return true;
}


//添加起始行
bool Http_conn::add_status_line(int status,const char *title)
{
	return add_response("%s %d %s\r\n","HTTP/1.1",status,title);
}


//添加首部
bool Http_conn::add_headers(int content_len)
{
	add_content_type();
	add_content_length(content_len);
	add_linger();
	add_blank_line();
}

//添加content_type首部
bool Http_conn::add_content_type()
{
	return add_response("Content-Type: text/html\r\n");
}
//添加content-length首部
bool Http_conn::add_content_length(int content_len)
{
	return add_response("Content-Length: %d\r\n",content_len);
}

bool Http_conn::add_allow()
{
	return add_response("Allow: GET, POST, HEAD, PUT, DELETE, OPTIONS\r\n");
}
//添加content_language首部
bool Http_conn::add_content_language()
{
	return add_response("Content-Language: zh, en;q=0.9\r\n");
}

//添加connection首部
bool Http_conn::add_linger()
{
	return add_response("Connection: %s\r\n",(linger == true) ? "keep-alive":"close");
}


//添加空白行
bool Http_conn::add_blank_line()
{
	return add_response("%s","\r\n");
}


//添加主体
bool Http_conn::add_content(const char *content)
{
	return add_response("%s",content);
}


//构造响应报文
bool Http_conn::process_write(HTTP_CODE ret)
{
	switch(ret)
	{
		case INTERNAL_ERROR:
		{
			add_status_line( 500, error_500_title );
			add_headers( strlen( error_500_form ) );
            	if ( ! add_content( error_500_form ) )
            	{
               	return false;
           	}
           	break;
		}
		case BAD_REQUEST:
		{
			add_status_line( 400, error_400_title );
           	add_headers( strlen( error_400_form ) );
           	if ( ! add_content( error_400_form ) )
           	{
           	    return false;
           	}
           	break;
		}
		case NO_RESOURCE:
		{
			add_status_line( 404, error_404_title );
           	add_headers( strlen( error_404_form ) );
           	if ( ! add_content( error_404_form ) )
           	{
            	return false;
           	}
           	break;
		}
		case FORBIDDEN_REQUEST:
		{
			add_status_line( 403, error_403_title );
           	add_headers( strlen( error_403_form ) );
           	if ( ! add_content( error_403_form ) )
           	{
               	return false;
           	}
           	break;
		}
		case FILE_REQUEST:
		{
			switch(method)
			{
				case OPTIONS:
				{
					add_status_line(200, ok_200_title);
					add_allow();
					add_content_length(0);
					add_blank_line();
					break;
				}
				case DELETE:
				{
					add_status_line(204, ok_204_title);
					add_content_length(0);
					add_blank_line();
					break;
				}
				case PUT:
				{
					if(file_exist)
					{
						add_status_line(200, ok_200_title);
						add_content_length(0);
						add_blank_line();
					}
					else
					{
						add_status_line(201, ok_201_title);
						add_content_length(0);
						add_blank_line();
					}
					break;
				}
				default:
				{
					add_status_line(200, ok_200_title);
					add_headers(file_stat.st_size);
				}
			}
		}
	}
	return true;
}

//处理请求
void Http_conn::process()
{
	HTTP_CODE read_ret = process_read();
	if(read_ret == NO_REQUEST)
	{
		modfd(epollfd, sockfd, EPOLLIN);
		return;
	}
	bool write_ret = process_write(read_ret);
	if(!write_ret)
	{
		close_conn();
	}
	modfd(epollfd,sockfd, EPOLLOUT);

}

//发送响应报文
bool Http_conn::write()
{
	int send_index = 0;
	printf("response..........\n%s\n",write_buf);
	while(send_index < write_index)
	{
		int len = send(sockfd,write_buf + send_index,write_index - send_index, MSG_DONTWAIT);
		if(len < 0 && errno != EAGAIN)
			return false;
		send_index += len;
	}
	switch(method)
	{
		case GET:
		case POST:
		{
			off_t offset = 0;
			int fd = open(real_file,O_RDONLY);
			sendfile(sockfd, fd, &offset, file_stat.st_size);
			close(fd);
			if(have_parameters)
			{
				remove(real_file);
			}
		}
	}

	if(linger)
	{//重置连接
		reset();
		printf("reseting .....\n");
		return true;
	}
	return false;
}

