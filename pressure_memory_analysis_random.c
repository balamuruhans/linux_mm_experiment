#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>

#include "event_apis.h"

/* Normal page size in Power8 */
#define PAGE_SIZE 65536

/* Huge page size in Power8 */
#define HPAGE_SIZE 16777216

/* 16MB memory size */
size_t MEM_SIZE = 16777216;

/* Pressure percentage */
float MEM_PRESSURE = 0.99;

bool RELEASE_FLAG = true;
bool FLAG = false;


float get_total_mem_bytes()
{
	unsigned long memsize = 0;
	char buff[256];
	FILE *meminfo = fopen("/proc/meminfo", "r");
	if(meminfo == NULL)
	{
		printf("No Meminfo Information\n");
		exit(-1);
	}
	while(fgets(buff, sizeof(buff), meminfo))
	{
		if(sscanf(buff, "MemFree: %lu kB", &memsize) == 1)
		{
			memsize *= 1024.0;
		}
	}
	if(fclose(meminfo) != 0)
	{
		exit(-1);
	}
	return memsize;
}

void *memory_pressure(void *memory_usage)
{
	char *paddr;
	size_t mem_size;
	int random_num;
	float *mem_percent = (float *)memory_usage;
	mem_size = (unsigned long) (*mem_percent * get_total_mem_bytes());
	printf("Allocating %f percent of memory: %zu in thread\n",
               MEM_PRESSURE*100, mem_size);
	paddr = mmap(NULL, mem_size, PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
	if (paddr == MAP_FAILED)
	{
		printf("Allocation of %zu failed using normal page size\n", mem_size);
		exit(1);
	}
	printf("Allocated successfully ~> %zu from %p to %p\n", mem_size, paddr, paddr);
	printf("Touching %f percent of allocated memory in thread\n",
               MEM_PRESSURE*100);
	memset(paddr, 1, mem_size);
	FLAG = true;
	/* to have constant memory pressure */
	printf("Touching memory with offset 64 continuously in thread\n");
	while(RELEASE_FLAG)
	{
	        /* Touching the memory randomly */
        	for(int i=0; i<mem_size; i+=PAGE_SIZE)
		{
			random_num = rand() % PAGE_SIZE;
			paddr[i+random_num] = 1;
        	}
	}
	printf("Exiting the memory pressure thread\n");
}

/*
 * enable perf_event_open()
 * perform Hugetlb/Normal/THP allocation using mmap
 * prepare the perf events and touch the allocated memory
 * display the perf events and cleanup events
 */

int main()
{
	char *addr;
	int random_num;
	pthread_t stress_thread;
	if (pthread_create(&stress_thread, NULL, memory_pressure, &MEM_PRESSURE))
	{
		printf("creating of thread failed\n");
		exit(-1);
	}
	while(true)
	{
		if(FLAG)
		{
			/* Normal page performance under pressure */
			printf("Performing test with Normal page\n");
			setup_events();
			open_events();
			addr = mmap(NULL, MEM_SIZE, PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
			if (addr == MAP_FAILED)
			{
				printf("Allocation of %zu failed using normal page size\n", MEM_SIZE);
				exit(1);
			}
			printf("Allocated successfully ~> %zu from %p to %p\n", MEM_SIZE, addr, addr);
			prepare_events();
			/* Touching the memory randomly */
			for(int i=0; i<MEM_SIZE; i+=PAGE_SIZE)
			{
				random_num = rand() % PAGE_SIZE;
				addr[i+random_num] = 1;
			}
			display_events();
			close_events();
			printf("Touched normal page allocated memory continuously using memset\n");

			/* Huge page performance under pressure */
			printf("Performing test with Huge page\n");
			setup_events();
			open_events();
			addr = mmap(NULL, MEM_SIZE, PROT_WRITE, MAP_HUGETLB|MAP_ANONYMOUS|MAP_SHARED, 0, 0);
			if (addr == MAP_FAILED)
			{
				printf("Allocation of %zu failed using normal page size\n", MEM_SIZE);
				exit(1);
			}
			printf("Allocated successfully ~> %zu from %p to %p\n", MEM_SIZE, addr, addr);
			prepare_events();
			/* Touching the memory randomly */
			for(int i=0; i<MEM_SIZE; i+=HPAGE_SIZE)
			{
				random_num = rand() % HPAGE_SIZE;
				addr[i+random_num] = 1;
			}
			display_events();
			close_events();
			printf("Touched it Hugepage allocated memory continuously using memset\n");

			/* Transparent huge page performance under pressure */
			printf("Performing test with Transparent Huge page\n");
			setup_events();
			open_events();
			addr = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			if (addr == MAP_FAILED)
			{
				printf("Allocation of %zu failed using normal page size\n", MEM_SIZE);
				exit(1);
			}
			printf("Allocated successfully ~> %zu from %p to %p\n", MEM_SIZE, addr, addr);
			int check_THP = madvise(addr, MEM_SIZE, MADV_HUGEPAGE);
			if(check_THP != 0)
			{
				printf("madvise failed to set MADV_HUGEPAGE the allocated region for THP\n");
				exit(1);
			}
			printf("madvise successfully set MADV_HUGEPAGE for allocated region for THP\n");
			prepare_events();
			/* Touching the memory randomly */
			for(int i=0; i<MEM_SIZE; i+=HPAGE_SIZE)
			{
				random_num = rand() % HPAGE_SIZE;
				addr[i+random_num] = 1;
			}
			display_events();
			close_events();
			printf("Touched it Transparent Hugepage allocated memory continuously using memset\n");
			RELEASE_FLAG = false;
			break;
		}
	}
	if (pthread_join(stress_thread, NULL))
	{
		printf("joining of thread failed\n");
		exit(-1);
	}
	return 0;
}
