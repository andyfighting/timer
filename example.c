#include <stdlib.h>
#include <stdio.h>
#include "timer.h"

#define STEP_ONE "Hello!"
#define STEP_TWO "How are you!"

void step_one_call(void *owner, tTimer *tid)
{
	printf("timer task function do something:%s\n",(char *)owner);
}

void step_two_call(void *owner, tTimer *tid)
{
        printf("timer task function do something:%s\n",(char *)owner);
}

int main(int argc,char *argv[])
{
	timer_type type = cycle_timer;

	init_sys_timer();
	
	create_sys_timer(step_one_call,1,type,(void *)STEP_ONE);
	
	create_sys_timer(step_two_call,3,type,(void *)STEP_TWO);
	
	for(;;)
		sleep(1);

	free_sys_timer();
	
	return 0;
}
