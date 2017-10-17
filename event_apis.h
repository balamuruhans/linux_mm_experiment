#include "event.c"
#define MAX_MM_EVENTS    6

struct event mm_events[MAX_MM_EVENTS];

void setup_event_user(struct event *e, u64 config, char *name, int type)
{
	event_init_opts(e, config, type, name);
	e->attr.disabled = 1;
	e->attr.exclude_kernel = 1;
	e->attr.exclude_hv = 1;
	e->attr.exclude_idle = 1;

}

void setup_event_kernel(struct event *e, u64 config, char *name, int type)
{
	event_init_opts(e, config, type, name);
	e->attr.disabled = 1;
	e->attr.exclude_idle = 1;
	e->attr.exclude_user = 1;
	e->attr.exclude_hv = 1;
}

void setup_events(void)
{
	setup_event_user(&mm_events[0], PERF_COUNT_SW_PAGE_FAULTS, "page-faults", PERF_TYPE_SOFTWARE);
	setup_event_user(&mm_events[1], PERF_COUNT_HW_CACHE_MISSES, "hw-cache-miss", PERF_TYPE_HARDWARE);
	setup_event_user(&mm_events[2], PERF_COUNT_HW_CACHE_REFERENCES, "hw-cache-reference", PERF_TYPE_HARDWARE);
	setup_event_user(&mm_events[3], PERF_COUNT_HW_INSTRUCTIONS, "instructions", PERF_TYPE_HARDWARE);
	setup_event_user(&mm_events[4], 0x10003, "dTLB-load-misses", PERF_TYPE_HW_CACHE);
	setup_event_kernel(&mm_events[5], PERF_COUNT_HW_INSTRUCTIONS, "instructions", PERF_TYPE_HARDWARE);
}

void open_events()
{

	for (int i = 0; i < MAX_MM_EVENTS; i++) {
		if (event_open(&mm_events[i]))
			perror("event_open() failed");
	}
}

void prepare_events(void)
{
	int i;
	for (i = 0; i < MAX_MM_EVENTS; i++)
		event_reset(&mm_events[i]);
	for (i = 0; i < MAX_MM_EVENTS; i++)
		event_enable(&mm_events[i]);
}

void display_events(void)
{
	int i;
	for (i = 0; i < MAX_MM_EVENTS; i++)
		event_disable(&mm_events[i]);

	for (i = 0; i < MAX_MM_EVENTS; i++)
		event_read(&mm_events[i]);

	for (i = 0; i < MAX_MM_EVENTS; i++)
		printf("[%20s]: \t %llu\n", mm_events[i].name, mm_events[i].result.value);
}

void close_events(void)
{
	for (int i = 0; i < MAX_MM_EVENTS; i++)
		event_close(&mm_events[i]);
}
