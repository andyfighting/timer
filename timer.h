#ifndef __MY_TIMER_H__
#define __MY_TIEMR_H__

#include <pthread.h>
#include "klist.h"
#include "mem_pool.h"

#define GLB_TIMER_POOL


#define S_INTERVAL(end_tv,start_tv) \
	(MS_INTERVAL(end_tv,start_tv)/1000)
	
#define MS_INTERVAL(end_tv,start_tv) \
	(((end_tv).tv_sec - (start_tv).tv_sec)*1000 + ((end_tv).tv_usec - (start_tv).tv_usec)/1000)

//4294967295 1000000
#define US_INTERVAL(end_tv,start_tv) \
	(((end_tv).tv_sec - (start_tv).tv_sec)*1000*1000 + ((end_tv).tv_usec - (start_tv).tv_usec))

#define TIMER_THREAD_STAT_ERR       (-1)
#define TIMER_THREAD_STAT_INIT      (0)
#define TIMER_THREAD_STAT_STARTING  (1)
#define TIMER_THREAD_STAT_STARTED   (2)
#define TIMER_THREAD_STAT_STOPPING  (3)
#define TIMER_THREAD_STAT_STOPED    (4)

#define WORKER_INIT                 (0)
#define WORKER_STARTED              (1)
#define WORKER_STOPED               (2)

#define TRIGGER_INTERVAL     (50)   //ms
#define TIME_INT_INTERVAL    (1000)  //us

#define DAY2SEC(day)   ((day)*3600*24)
#define HOUR2SEC(hour) ((hour)*60*60)
#define MIN2SEC(min)   ((min)*60)
#define SEC2MS(sec)    ((sec)*1000)

#define MIN2MS(min) (SEC2MS(MIN2SEC(min)))

#define TIMER_TBL_SIZE (MIN2MS(6)/TRIGGER_INTERVAL)

#define SEC2JIFFIES(sec) (SEC2MS(sec)/TRIGGER_INTERVAL)
#define TIMER_INDEX(jiffies) ((jiffies)%TIMER_TBL_SIZE)

typedef struct _st_timer_worker tTimerWorker;
typedef struct _st_Timer tTimer;
typedef void  (*TimerFunc)(void *owner, tTimer *tid);

struct _st_Timer
{
    void     *owner;
    TimerFunc pFunc; // this is callback function.
	spinlock_t     spin_lock;
	s32   ref_cnt;
    u64   timeup;        // when the timer is expired (jiffies).
	u32   index;
    s16   ticks;         // expired time (s).
    u8    type;     // normal timer or cycle timer.
    u8    used;     // in use or not in use.

	tTimerWorker * master;

    struct list_head list;
};

typedef struct st_timer_list
{
	spinlock_t   spin_lock;
	struct list_head head;
}tTimerList;


struct _st_timer_worker
{
#ifndef GLB_TIMER_POOL
	tMemPool   * timer_pool;
#endif
	u32          timer_count;
	u64          jiffies;
	tTimerList   timer_tbl[TIMER_TBL_SIZE];
	s32          thread_stat;
	pthread_t    timer_thread;
	volatile s32 worker_stat;
};

typedef enum timer_type
{
	normal_timer,
	cycle_timer,
	timer_type_num
}timer_type;


extern struct timeval cur_sys_tv;

extern u64 jiffies;


tTimer * create_sys_timer(TimerFunc timeout,int sec,timer_type type,void * owner);
tTimer * create_timer(tTimerWorker * timer_worker,TimerFunc timeout,int sec,timer_type type,void * owner);
void hold_timer(tTimer * timer);
int modify_timer(tTimer *timer,timer_type kind,unsigned int seconds, TimerFunc func);
int kill_timer(tTimer *timer);
tTimerWorker * alloc_timer_worker();
void free_timer_worker(tTimerWorker * timer_worker);

int init_sys_timer();
int free_sys_timer();

struct tm *get_localtime();


#endif

