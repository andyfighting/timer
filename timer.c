#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "klist.h"
#include "timer.h"


struct timeval cur_sys_tv;

u64 jiffies;

static tTimerWorker * sys_timer_worker = NULL;

#ifdef GLB_TIMER_POOL
tMemPool * timer_pool = NULL;
spinlock_t timer_mutex_lock = SPIN_LOCK_INIT;

static int init_timer_pool()
{
	int ret;
	spin_lock(&timer_mutex_lock);
	if (timer_pool == NULL)
	{
		timer_pool = get_mempool(sizeof(tTimer),10000);
	}
	if (timer_pool)
	{
		ret = SUCCESS;
	}
	else
	{
		ret = FAILURE;
	}
	spin_unlock(&timer_mutex_lock);
	return ret;
}
#endif


#ifndef TIMER
static inline void lock_timer(tTimer * timer)
{
	spin_lock(&timer->spin_lock);
}

static inline void unlock_timer(tTimer * timer)
{
	spin_unlock(&timer->spin_lock);
}

static inline void free_timer(tTimer * timer)
{
	if (atomic_dec_and_test(&timer->ref_cnt))
	{
#ifdef GLB_TIMER_POOL
		timer = (tTimer *)free_mem(timer_pool,timer);
#else
		timer = (tTimer *)free_mem(timer->master->timer_pool,timer);
#endif
	}
}

static inline tTimer * alloc_timer(tTimerWorker * timer_worker)
{
	tTimer * timer;
#ifdef GLB_TIMER_POOL
	timer = (tTimer *)get_mem(timer_pool);
#else
	timer = (tTimer *)get_mem(timer_worker->timer_pool);
#endif
	timer->master = timer_worker;
	return timer;
}

int modify_timer(tTimer *timer,timer_type kind,unsigned int seconds, TimerFunc func)
{
	int ret;
	lock_timer(timer);
	if (timer->used)
	{
	    timer->type  = kind;
	    timer->ticks = seconds;
	    timer->pFunc = func;
	    ret = SUCCESS;
	}
	else
	{
		ret = FAILURE;
	}
	unlock_timer(timer);
	return ret;
}

int kill_timer(tTimer *timer)
{
	int ret;
	lock_timer(timer);
	if (timer->used)
	{
		timer->used = 0;
	    ret = SUCCESS;
	}
	else
	{
		ret = FAILURE;
	}
	unlock_timer(timer);
	free_timer(timer);

	return ret;
}

void hold_timer(tTimer * timer)
{
	atomic_inc(&timer->ref_cnt);
}

#endif

#ifndef TIMER_LIST
static inline void init_timer_list(tTimerList * timer_list)
{
	init_spinlock(&timer_list->spin_lock);
	INIT_LIST_HEAD(&timer_list->head);
}


static inline void del_from_timerlist(tTimer * timer)
{
	list_del(&timer->list);
}

static inline void free_timer_list(tTimerList * timer_list)
{
	tTimer * pos;
	tTimer * next;

	spin_lock(&timer_list->spin_lock);
	list_for_each_entry_safe(pos,next,&timer_list->head,list)
	{
		del_from_timerlist(pos);
		kill_timer(pos);
	}
	spin_unlock(&timer_list->spin_lock);
}

static inline void add_to_timerlist(tTimer * timer)
{
	tTimerList * timer_list;
	timer_list = &timer->master->timer_tbl[timer->index];
	list_add(&timer->list,&timer_list->head);
}

static inline void del_from_timerlist_lock(tTimer * timer,tTimerList * list)
{
	spin_lock(&list->spin_lock);
	list_del(&timer->list);
	spin_unlock(&list->spin_lock);
}

static inline void add_to_timerlist_lock(tTimer * timer)
{
	tTimerList * timer_list;
	timer_list = &timer->master->timer_tbl[timer->index];
	spin_lock(&timer_list->spin_lock);
	list_add(&timer->list,&timer_list->head);
	spin_unlock(&timer_list->spin_lock);
}

#endif
#ifndef SYS_TIMER

#define MAX_SLEEP_US (TRIGGER_INTERVAL*TIME_INT_INTERVAL)

static pthread_t timeval_update_thread_id;
static pthread_t jiffies_update_thread_id;

void * timeval_update(void * arg)
{
	while (1)
	{
		gettimeofday(&cur_sys_tv,NULL);
		usleep(1000);
	}
	pthread_exit(0);
}

void * jiffies_update(void * arg)
{
	jiffies = 0;
	while (1)
	{
		usleep(MAX_SLEEP_US);
		jiffies++;
	}
	pthread_exit(0);
}

int init_sys_timer()
{
#ifdef GLB_TIMER_POOL
	if (init_timer_pool() == FAILURE)
	{
		return FAILURE;
	}
#endif
	if (pthread_create(&timeval_update_thread_id,NULL,timeval_update,NULL) < 0)
	{
		return FAILURE;
	}
	if (pthread_create(&jiffies_update_thread_id,NULL,jiffies_update,NULL) < 0)
	{
		return FAILURE;
	}

	sys_timer_worker = alloc_timer_worker();
	if (sys_timer_worker)
	{
		return SUCCESS;
	}
	return FAILURE;
}

tTimer * create_sys_timer(TimerFunc timeout,int sec,timer_type type,void * owner)
{
	return create_timer(sys_timer_worker,timeout,sec,type,owner);
}

int free_sys_timer()
{
	 free_timer_worker(sys_timer_worker);
	printf("free_sys_timer\n");
	 return 0;
}

#endif

static inline void trigger_timer(tTimer * timer)
{
	lock_timer(timer);
	if (timer->used)
	{
		timer->pFunc(timer->owner,timer);
	}
	if (timer->used)
	{
		if (timer->type == cycle_timer)
		{
			u32  index;
			timer->timeup = timer->master->jiffies + SEC2JIFFIES(timer->ticks);
			index  = TIMER_INDEX(timer->timeup);
			if (index == timer->index)
			{
				add_to_timerlist(timer);
			}
			else
			{
				timer->index = index;
				add_to_timerlist_lock(timer);
			}
		}
		else
		{
			timer->used = 0;
		}
	}
	unlock_timer(timer);
	if (!timer->used)
	{
		free_timer(timer);
	}
}


static inline void check_timer_list(tTimerList * timer_list)
{
	tTimer * pos;
	tTimer * next;

	spin_lock(&timer_list->spin_lock);
	list_for_each_entry_safe(pos,next,&timer_list->head,list)
	{
		if (pos->timeup >= pos->master->jiffies)
		{
			del_from_timerlist(pos);
			trigger_timer(pos);
		}
	}
	spin_unlock(&timer_list->spin_lock);
}

static inline void add_timer_to_worker(tTimerWorker * timer_worker,tTimer * timer)
{
	timer->timeup = timer_worker->jiffies + SEC2JIFFIES(timer->ticks);
	timer->index  = TIMER_INDEX(timer->timeup);
	hold_timer(timer);
	add_to_timerlist_lock(timer);
	return ;
}

tTimer * create_timer(tTimerWorker * timer_worker,TimerFunc timeout,int sec,timer_type type,void * owner)
{
	tTimer * timer;

	if (timer_worker->worker_stat == WORKER_STARTED)
	{
		timer = alloc_timer(timer_worker);
		if (timer)
		{
			timer->owner  = owner;
			timer->pFunc  = timeout;
			timer->ticks  = sec;
			timer->type   = type;
			timer->used   = 1;
			init_spinlock(&timer->spin_lock);
			add_timer_to_worker(timer_worker,timer);
			hold_timer(timer);
			return timer;
		}
	}
	return NULL;
}

static void * timer_check_thread(void * arg)
{
	tTimerWorker * timer_worker;
	tTimerList   * timer_list;
	int  ret;
	int  oldtype;
	
	timer_worker = (tTimerWorker *)arg;
	ret = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS ,&oldtype); 
	if (ret == 0)
	{

	}
	else if (EINVAL == ret)
	{
		printf("timer_check_thread pthread_setcanceltype EINVAL\n");
		timer_worker->thread_stat = TIMER_THREAD_STAT_ERR;
	}
	else if (EFAULT == ret)
	{
		printf("timer_check_thread pthread_setcanceltype EFAULT\n");
		timer_worker->thread_stat = TIMER_THREAD_STAT_ERR;
	}

	timer_worker = (tTimerWorker *)arg;
	if (timer_worker == NULL)
	{
		timer_worker->thread_stat = TIMER_THREAD_STAT_ERR;
		return NULL;
	}
	timer_worker->thread_stat = TIMER_THREAD_STAT_STARTED;

	while (timer_worker->thread_stat == TIMER_THREAD_STAT_STARTED)
	{
		while (timer_worker->jiffies <= jiffies)
		{
			timer_list = &timer_worker->timer_tbl[TIMER_INDEX(timer_worker->jiffies)];
			check_timer_list(timer_list);
			timer_worker->jiffies++;
		}
		usleep(1000);
	}

	timer_worker->thread_stat = TIMER_THREAD_STAT_STOPED;
	return NULL;
}

tTimerWorker * alloc_timer_worker()
{
	tTimerWorker * timer_worker;
	int            index;
	timer_worker = (tTimerWorker *)malloc(sizeof(tTimerWorker));
	if (timer_worker)
	{
		memset(timer_worker,0x00,sizeof(tTimerWorker));
		timer_worker->jiffies     = jiffies;
		timer_worker->timer_count = 0;
#ifndef GLB_TIMER_POOL
		timer_worker->timer_pool  = NULL;
		timer_worker->timer_pool  = get_mempool(sizeof(tTimer),10000);
		if (timer_worker->timer_pool == NULL)
		{
			goto err;
		}
#endif
		timer_worker->thread_stat = TIMER_THREAD_STAT_INIT;
		for (index = 0;index < TIMER_TBL_SIZE;index++)
		{
			init_timer_list(&timer_worker->timer_tbl[index]);
		}

		timer_worker->thread_stat = TIMER_THREAD_STAT_STARTING;
		if (pthread_create(&timer_worker->timer_thread,NULL,timer_check_thread,timer_worker) < 0)
		{
			goto err;
		}

		while (timer_worker->thread_stat == TIMER_THREAD_STAT_STARTING);

		if (timer_worker->thread_stat == TIMER_THREAD_STAT_ERR)
		{
			goto err;
		}
		timer_worker->worker_stat = WORKER_STARTED;
	}
	return timer_worker;
err:
	if (timer_worker)
	{
		free(timer_worker);
	}
	return NULL;
}

void free_timer_worker(tTimerWorker * timer_worker)
{
	int index;

	if (timer_worker)
	{
		timer_worker->worker_stat = WORKER_STOPED;
		usleep(1000*10);
		timer_worker->thread_stat = TIMER_THREAD_STAT_STOPPING;

		while (timer_worker->thread_stat == TIMER_THREAD_STAT_STOPPING);

		for (index = 0;index < TIMER_TBL_SIZE;index++)
		{
			free_timer_list(&timer_worker->timer_tbl[index]);
		}
#ifndef GLB_TIMER_POOL
		if (timer_worker->timer_pool == NULL)
		{
			free_mempool(timer_worker->timer_pool);
		}
		timer_worker->timer_pool = NULL;
#endif
	}
}


struct tm *get_localtime()
{
	///////////////////////////////////////////////////////
		
		time_t now;	//实例化time_t结构
	    struct tm *timenow;	//实例化tm结构指针
		time(&now);
		//time函数读取现在的时间(国际标准时间非北京时间)，然后传值给now
		timenow = localtime(&now);
		//localtime函数把从time取得的时间now换算成你电脑中的时间(就是你设置的地区)
		if(timenow != NULL)
		{
			return timenow;
		}
		else
		{
			printf("get local time error!!!\n");
			//ldr_log("get local time error!!!\n");
		}
		return NULL;
}



