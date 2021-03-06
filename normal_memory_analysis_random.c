#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>

/* APIs required for performance monitoring units */
#include "event_apis.h"

/* Normal page size in Power8 */
#define PAGE_SIZE 65536

/* 16MB memory size */
size_t MEM_SIZE = 16777216;

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

	/* Normal page performance */
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
	printf("Touched normal page allocated memory continuously using random offset\n");

	/* Huge page performance */
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
        for(int i=0; i<MEM_SIZE; i+=PAGE_SIZE)
	{
		random_num = rand() % PAGE_SIZE;
		addr[i+random_num] = 1;
	}
	display_events();
	close_events();
	printf("Touched it Hugepage allocated memory continuously using random offset\n");

	/* Transparent huge page performance */
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
        for(int i=0; i<MEM_SIZE; i+=PAGE_SIZE)
	{
		random_num = rand() % PAGE_SIZE;
		addr[i+random_num] = 1;
	}
	display_events();
	close_events();
	printf("Touched it Transparent Hugepage allocated memory continuously using random offset\n");
	return 0;
}
