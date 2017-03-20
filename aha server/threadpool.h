#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <list>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>


template<typename T>
class Threadpool
{
public:
	//初始化线程池
	Threadpool(int td_num,int rq_num);
	//销毁线程池
	~Threadpool();
	//往工作队列添加用户请求
	bool append(T *request);
private:
	int thread_num;
	int max_requests_num;
	sem_t queue_stat;
	pthread_mutex_t queue_locker;
	pthread_t *threads;
	std::list<T *>workqueue;
	bool stop;

private:
	//线程调度执行的方法
	static void* worker(void *arg);
	//启动线程
	void run();
};


//初始化线程池
template<typename T>
Threadpool<T>::Threadpool(int td_num,int rq_num)
{
	thread_num = td_num;
	max_requests_num = rq_num;
	threads = new pthread_t[thread_num];
	for(int i = 0;i < thread_num;i ++)
		pthread_create(threads+i,NULL,worker,this);
	pthread_mutex_init(&queue_locker,NULL);
	sem_init(&queue_stat,0,0);
}

//启动线程
template<typename T>
void Threadpool<T>::run()
{
	while(!stop)
	{
		sem_wait(&queue_stat);
		pthread_mutex_lock(&queue_locker);
		if(workqueue.empty())
		{
			pthread_mutex_unlock(&queue_locker);
			continue;
		}
		T* request = workqueue.front();
		workqueue.pop_front();
		pthread_mutex_unlock(&queue_locker);
		if(!request)
			continue;
		request->process();
	}
}

//线程调度执行的方法
template<typename T>
void* Threadpool<T>::worker(void* arg)
{
	Threadpool *pool = (Threadpool *)arg;
	pool->run();
	return pool;
}



//往工作队列添加用户请求
template<typename T>
bool Threadpool<T>::append(T *request)
{
	pthread_mutex_lock(&queue_locker);
	if(workqueue.size() >=  max_requests_num)
	{
		pthread_mutex_unlock(&queue_locker);
		printf("request is to much!!!!!!!!!!!!!!!!!!\n");
		return false;
	}
	workqueue.push_back(request);
	sem_post(&queue_stat);
	pthread_mutex_unlock(&queue_locker);

}


//销毁线程池
template<typename T>
Threadpool<T>::~Threadpool()
{
	delete [] threads;
	stop = true;
	pthread_mutex_destroy(&queue_locker);
	sem_destroy(&queue_stat);
}

#endif