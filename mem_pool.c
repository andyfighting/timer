#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "klist.h"
#include "mem_pool.h"
typedef struct st_mem_node
{
	struct st_mem_pool * mem_pool;
	struct cas_stack list;
	char   data[0];
}tMemNode;

void * get_mem(struct st_mem_pool * pMemPool)
{
	struct st_mem_node * pNode;
	cas_stack_pop_entry(pNode,&pMemPool->stack_head,list);
	if (pNode)
	{
//		pNode->mem_pool = pMemPool;
		atomic_dec(&pMemPool->res_num);
		return (void *)pNode->data;	
	}
	return NULL;
}

int free_mem(struct st_mem_pool * pMemPool,void * mem)
{
	struct st_mem_node * pNode;
	if (!mem || !pMemPool)
	{
		return -1;
	}
	pNode = container_of(mem,struct st_mem_node,data);
	if (pNode->mem_pool != pMemPool)
	{
		return -1;
	}
	cas_stack_push(&pNode->list,&pMemPool->stack_head);
	atomic_inc(&pMemPool->res_num);
	return 0;
}

struct st_mem_pool * get_mempool(unsigned short node_size,unsigned short node_num)
{
	struct st_mem_pool * pMemPool;
	struct st_mem_node * pMemNode;
	void               * ptr;
	unsigned short       index;
	unsigned short       real_node_size;
	if (node_size >= 0xffff - sizeof(struct st_mem_node))
	{
		return NULL;
	}
	pMemPool = (struct st_mem_pool *)malloc(sizeof(struct st_mem_pool));
	if (pMemPool)
	{
		real_node_size = node_size + sizeof(struct st_mem_node);
		real_node_size = real_node_size + ((real_node_size%MEM_POOL_ALIGN_SIZE)?(MEM_POOL_ALIGN_SIZE - (real_node_size%MEM_POOL_ALIGN_SIZE)):0);
		pMemPool->real_node_size  = real_node_size;
		pMemPool->node_size       = node_size;
		pMemPool->node_num        = node_num;
		pMemPool->stack_head.next = NULL;
		ptr = malloc(real_node_size*node_num);
		if (ptr == NULL)
		{
			goto error;
		}
		pMemPool->start = ptr;
		pMemPool->range = real_node_size*node_num;
		for (index = 0;index < node_num;index++)
		{
			pMemNode = (struct st_mem_node *)(ptr + index*real_node_size);
			pMemNode->mem_pool = pMemPool;
			free_mem(pMemPool,pMemNode->data);
		}
	}
	return pMemPool;
error:
	free(pMemPool);
	return NULL;
}

int free_mempool(struct st_mem_pool * pMemPool)
{
	if (pMemPool)
	{
		if (pMemPool->start)
		{
			memset(pMemPool->start,0x00,pMemPool->range);
			free(pMemPool->start);
		}
		free(pMemPool);
		return 0;
	}
	return -1;
}

