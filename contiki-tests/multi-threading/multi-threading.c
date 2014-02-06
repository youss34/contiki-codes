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
	int i;
	for(i = 0; i < 10; i++) {
		printf("%c ", *ptr);
		ptr++;
		loop++;
	}
	putchar('\n');
	mt_exit();
}

static void thread_count_main(void *data)
{
	int c = 0;
	while(c < 10){
		printf("%d ", c);
		c++;
	}
	putchar('\n');
	mt_exit();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(multi_threading_process, ev, data)
{
	static struct mt_thread alpha_thread;
	static struct mt_thread count_thread;
	
	PROCESS_BEGIN();
	
	puts("Begining process........");
	
	mt_init();
	mt_start(&alpha_thread, thread_alpha_main, "ABCDEFGHIJ");
	mt_start(&count_thread, thread_count_main, NULL);

	mt_exec(&alpha_thread);
	mt_exec(&count_thread);
	
	mt_stop(&alpha_thread);
	mt_stop(&count_thread);
	mt_remove();

	puts("Ending process......");

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
