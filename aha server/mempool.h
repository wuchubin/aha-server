#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>


//内存块默认大小
#define MBSIZE 4096
#define MBNUM 1204




/*
内存池(Memory Pool)是一种内存分配方式。 
通常我们习惯直接使用new、malloc等API申请分配内存，这样做的缺点在于：由于所申请内存块的大小不定，当频繁使用时会造成大量的内存碎片并进而降低性能。
内存池则是在真正使用内存之前，先申请分配一定数量的、大小相等(一般情况下)的内存块留作备用。
当有新的内存需求时，就从内存池中分出一部分内存块，若内存块不够再继续申请新的内存。这样做的一个显著优点是尽量避免了内存碎片，使得内存分配效率得到提升。
用于存放客户端的请求数据和服务器的响应数据
*/




//内存块
struct memblock
{
	//指向内存数据
	void *data;
	//指向下一个内存块
	memblock *next;
};




//内存池
class Mempool
{
public:
	Mempool(){}
	//销毁内存池
	~Mempool();
	//创建内存数量为_mb_num的内存池，内存块大小为_mb_size
	void init(size_t _mb_num = MBNUM,size_t _mb_size = MBSIZE);
	//往内存池添加指定数量内存块
	void mempool_insert_mblocks(int num = 10);
	//调整内存池大小
	void mempool_decrease(int num = MBNUM);
	//从内存池获取指定数量的内存块
	memblock *mempool_take_mblocks_by_num(int num = 1);
	//从内存池获取指定大小的若干内存块
	memblock *mempool_take_mblocks_by_size(int size);
	//将用完的内存块放回内存池
	void mempool_put_mblocks(memblock *mbptr);

	int get_blocksize() {return mb_size;}
//测试用
	void print(){printf("mb_num = %d,mb_size = %d\n",mb_num,mb_size);}


private:
	//指向内存池第一个内存块
	memblock *first;
	//指向内存池的最后一个内存块
	memblock *last;
	//内存池当前大小
	size_t mb_num;
	//内存块大小
	size_t mb_size;

};


#endif
