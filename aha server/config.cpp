#include "config.h"

#include <iostream>

using namespace std;




//读取配置文件，根据配置文件设置相关变量
Config::Config()
{
	char buf[200], *head, *last;
	ip = docroot = NULL;
	port = worker_num = memblock_num = memblock_size = 0;
	//已可读方式打开配置文件
	FILE *pfile = fopen(CONFIG_FILE,"r");
	//每次从配置文件读取一行，分析其中的Key:value对，将相应的value填充到key上
	while(fgets(buf, 200, pfile) != NULL)
	{
		if((head = strstr(buf,"ip"))  != NULL)
		{
			head += 2;
			//跳过':'前面的空格
			while(*head == ' ')
				head ++;
			//跳过':'
			head ++;
			//跳过':'后面的空格
			while(*head == ' ')
				head ++;
			last = buf+ strlen(buf) - 1;
			//将value后面一个字符赋值为'\0';
			while(*last == ' ' || *last == '\n' || *last == '\r')
			{
				*last = '\0';
				last --;
			}
			ip = (char *)malloc(strlen(head)+1);
			strcpy(ip,head);
		}
		else if((head = strstr(buf,"port")) != NULL)
		{
			head += 4;
			while(*head == ' ')
				head ++;
			head ++;
			while(*head == ' ')
				head ++;
			last = buf+ strlen(buf) - 1;
			while(*last == ' ' || *last == '\n' || *last == '\r')
			{
				*last = '\0';
				last --;
			}
			port = atoi(head);
		}
		else if((head = strstr(buf,"docroot")) != NULL)
		{
			head += 7;
			while(*head == ' ')
				head ++;
			head ++;
			while(*head == ' ')
				head ++;
			last = buf+ strlen(buf) - 1;
			while(*last == ' ' || *last == '\n' || *last == '\r')
			{
				*last = '\0';
				last --;
			}
			docroot = (char *)malloc(strlen(head)+1);
			strcpy(docroot,head);
		}
		else if((head = strstr(buf,"worker_num")) != NULL)
		{
			head += 10;
			while(*head == ' ')
				head ++;
			head ++;
			while(*head == ' ')
				head ++;
			last = buf+ strlen(buf) - 1;
			while(*last == ' ' || *last == '\n' || *last == '\r')
			{
				*last = '\0';
				last --;
			}
			worker_num = atoi(head);
		}
		else if((head = strstr(buf,"memblock_num")) != NULL)
		{
			head += 12;
			while(*head == ' ')
				head ++;
			head ++;
			while(*head == ' ')
				head ++;
			last = buf+ strlen(buf) - 1;
			while(*last == ' ' || *last == '\n' || *last == '\r')
			{
				*last = '\0';
				last --;
			}
			memblock_num = atoi(head);
		}
		else if((head = strstr(buf,"memblock_size")) != NULL)
		{
			head += 13;
			while(*head == ' ')
				head ++;
			head ++;
			while(*head == ' ')
				head ++;
			last = buf+ strlen(buf) - 1;
			while(*last == ' ' || *last == '\n' || *last == '\r')
			{
				*last = '\0';
				last --;
			}
			memblock_size = atoi(head);
		}
		else if((head = strstr(buf,"rq_max_size")) != NULL)
		{
			head += 11;
			while(*head == ' ')
				head ++;
			head ++;
			while(*head == ' ')
				head ++;
			last = buf+ strlen(buf) - 1;
			while(*last == ' ' || *last == '\n' || *last == '\r')
			{
				*last = '\0';
				last --;
			}
			rq_max_size = atoi(head);
		}
	}
	fclose(pfile);
}










Config::~Config()
{
	free(ip);
	free(docroot);
}