#include "contiki.h"
#include "sys/mt.h"
#include <stdio.h>

static short loop = 0;

PROCESS(multi_threading_process, "Multi-threading process");
AUTOSTART_PROCESSES(&multi_threading_process);

/*---------------------------------------------------------------------------*/
static void thread_alpha_main(void *data)
{
	char *ptr = (char*) data;
	while(1) {
		printf("%c ", *ptr);
		ptr++;
		loop++;
		mt_yield();
	}
	mt_exit();
}

static void thread_count_main(void *data)
{
	int c = 0;
	while(1){
		printf("%d ", c);
		c++;
		mt_yield();
	}
	mt_exit();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(multi_threading_process, ev, data)
{
	static struct mt_thread alpha_thread;
	static struct mt_thread count_thread;

	static struct etimer timer;
	static int toggle;
	
	PROCESS_BEGIN();
	
	puts("Begining process........");
	
	mt_init();
	mt_start(&alpha_thread, thread_alpha_main, "ABCDEFGHIJ");
	mt_start(&count_thread, thread_count_main, NULL);

	etimer_set(&timer, CLOCK_SECOND / 2);
	
	while(loop < 10) {
		PROCESS_WAIT_EVENT();
		if(ev == PROCESS_EVENT_TIMER) {
			if(toggle) {
				mt_exec(&alpha_thread);
				toggle--;
			}
			else {
				mt_exec(&count_thread);
				toggle++;
			}

			etimer_set(&timer, CLOCK_SECOND / 2);
		}
	}

	mt_stop(&alpha_thread);
	mt_stop(&count_thread);
	mt_remove();

	puts("\nEnding process......");

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
