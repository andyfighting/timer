#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "klist.h"

#define MEM_POOL_ALIGN_SIZE (4)

typedef struct st_mem_pool
{
	struct cas_stack stack_head;
	unsigned short   node_size;
	unsigned short   real_node_size;
	unsigned short   node_num;
	unsigned int     range;

	void          *  start;

	int              res_num;
}tMemPool;

void * get_mem(struct st_mem_pool * pMemPool);
int free_mem(struct st_mem_pool * pMemPool,void * mem);

struct st_mem_pool * get_mempool(unsigned short node_size,unsigned short node_num);
int free_mempool(struct st_mem_pool * pMemPool);

#endif
