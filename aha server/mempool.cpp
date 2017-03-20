#include "mempool.h"








//创建内存数量为_mb_num的内存池，内存块大小为_mb_size
void Mempool::init(size_t _mb_num,size_t _mb_size)
{
	memblock *memnew, *memptr;
	mb_num = _mb_num;
	mb_size = _mb_size;
	first = last = memnew = memptr = NULL;
	for(int i = 0;i < mb_num;i ++)
	{
		memnew = (memblock *)malloc(sizeof(memblock));
		assert(memnew);
		memnew->data = (void *)malloc(mb_size);
		assert(memnew->data);
		memnew->next = NULL;
		if(first == NULL)
		{
			first = memptr = memnew;
		}
		else
		{
			memptr->next = memnew;
			memptr = memptr->next;
		}
	}
	last = memnew;
}







//销毁内存池
Mempool::~Mempool()
{
	memblock *delptr = NULL;     
	while(first != NULL)
	{
		delptr = first;
		first = first->next;
		free(delptr->data);
		free(delptr);
	}
}







//往内存池添加指定数量内存块
void Mempool::mempool_insert_mblocks(int num)
{
	memblock *memnew = NULL;
	for(int i = 0;i < num;i ++)
	{
		memnew = (memblock *)malloc(sizeof(memblock));
		assert(memnew);
		memnew->data = (void *)malloc(mb_size);
		assert(memnew->data);
		memnew->next = NULL;
		if(last == NULL)
		{
			first = last = memnew;
		}
		else
		{
			last->next = memnew;
			last = last->next;
		}
	}
	mb_num += num;
}










//调整内存池大小
void Mempool::mempool_decrease(int num)
{
	if(mb_num > num)
	{
		memblock *delptr = NULL;
		int total = mb_num - num;
		for(int i = 0;i < total;i ++)
		{
			delptr = first;
			first = first->next;
			free(delptr->data);
			free(delptr);
		}
		mb_num = num;
	}
}










//从内存池获取指定数量的内存块
memblock *Mempool::mempool_take_mblocks_by_num(int num)
{
	memblock *memptr, *head;
	while(num > mb_num)
	{
		mempool_insert_mblocks();
	}
	head = first;
	for(int i = 0;i < num;i ++)
	{
		memptr = first;
		first = first->next;
	}
	memptr->next = NULL;
	mb_num -= num;
	return head;
}








//从内存池获取指定大小的若干内存块
memblock *Mempool::mempool_take_mblocks_by_size(int size)
{
	int num = size/mb_size;
	if(size % mb_size)
		num ++;
	return mempool_take_mblocks_by_num(num);
}






//将用完的内存块放回内存池
void Mempool::mempool_put_mblocks(memblock *mbptr)
{
	if(mbptr == NULL)
		return;
	int num = 0;
	if(last == NULL)
		first = last = mbptr;
	else
		last->next = mbptr;
	while(mbptr)
	{
		num ++;
		last = mbptr;
		mbptr = mbptr->next;
	}
	mb_num += num;
}
