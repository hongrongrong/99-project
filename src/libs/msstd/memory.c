#include "msstd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>

//#define MS_LEAK_DETECT

#ifdef MS_LEAK_DETECT

#define MAX_LEAK_FILE_SIZE (2*1024*1024)
struct mem_info
{
	void *address;
	unsigned int size;
	unsigned int line;
	char file[32];
	char time[32];
};

struct mem_leak
{
	struct mem_info info;
	struct mem_leak *next;
};

static struct mem_leak *ptr_start = 0;
static struct mem_leak *ptr_next = 0;
static MUTEX_OBJECT global_mutex;

static void add(struct mem_info *info)
{
	struct mem_leak *leak = (struct mem_leak *)malloc(sizeof(struct mem_leak));
	if(!leak)
		return;
	leak->info.address = info->address;
	leak->info.size = info->size;
	leak->info.line = info->line;
	snprintf(leak->info.file, sizeof(leak->info.file), "%s", info->file);
	time_to_string_ms(leak->info.time);
	leak->next = 0;

	if (ptr_start == NULL)	
	{
		ptr_start = leak;
		ptr_next = ptr_start;
	}
	else 
	{
		ptr_next->next = leak;
		ptr_next = ptr_next->next;		
	}	
}

static void erase(unsigned int pos)
{
	unsigned int i = 0;
	struct mem_leak *leak;
	if(pos == 0)
	{
		leak = ptr_start;
		ptr_start = ptr_start->next;
		free(leak);
		return;
	}
	for(i = 0, leak = ptr_start; i < pos; leak = leak->next, ++i)
	{
		if(pos == i + 1)
		{
			struct mem_leak *temp = leak->next;
			leak->next = temp->next;
			free(temp);
			if(leak->next == 0)
				ptr_next = leak;
			break;
		}
	}
}

static void __ms_clear()
{
	struct mem_leak *temp = ptr_start;
	struct mem_leak *leak = ptr_start;

	while(leak != NULL) 
	{
		leak = leak->next;
		free(temp);
		temp = leak;
	}
	ptr_start = 0;	
}

static void add_mem_info (void *mem_ref, unsigned int size,  const char * file, unsigned int line)
{
	struct mem_info info;
	memset(&info, 0, sizeof(info));
	info.address = mem_ref;
	info.size = size;
	snprintf(info.file, sizeof(info.file), "%s", file);
	info.line = line;
	ms_mutex_lock(&global_mutex);
	add(&info);
	ms_mutex_unlock(&global_mutex);	
}

static void remove_mem_info (void * mem_ref, const char* file, int lineno)
{
	unsigned int index;
	ms_mutex_lock(&global_mutex);
	struct mem_leak *leak_info = ptr_start;
	//printf("=====remove_mem_info=========0000=====%p====\n", mem_ref);
	for(index = 0; leak_info != NULL; ++index, leak_info = leak_info->next)
	{
		if(leak_info->info.address == mem_ref)
		{
			erase(index);
			break;
		}
	}
	ms_mutex_unlock(&global_mutex); 
}

static void replace_mem_info (void * mem_old, void *mem_new, const char* file, int lineno)
{
	unsigned int index;
	ms_mutex_lock(&global_mutex);
	struct mem_leak *leak_info = ptr_start;
	for(index = 0; leak_info != NULL; ++index, leak_info = leak_info->next)
	{
		if(leak_info->info.address == mem_old)
		{
			leak_info->info.address = mem_new;
			snprintf(leak_info->info.file, sizeof(leak_info->info.file), "%s", file);
			leak_info->info.line = lineno;
			break;
		}
	}
	ms_mutex_unlock(&global_mutex); 
}
void* __ms_realloc(void *ptr, unsigned int size, const char* file, int lineno)
{
	void* p = realloc(ptr, size);
	if(!p)
	{
		system("cat /proc/buddyinfo");
		printf("[ERROR]file:%s line:%d realloc size:%d fail.\n", file, lineno, size);
		system("cat /proc/buddyinfo");
		sleep(2);
		assert(p);
	}
	else
	{
		if(p != ptr)
		{
			replace_mem_info(ptr, p, file, lineno);
		}
	}
	return p;		
}
void* __ms_malloc(unsigned int size, const char* file, int lineno)
{
	if(size <= 0) 
		return NULL;
	void* p = malloc(size);
	if(!p)
	{
		system("cat /proc/buddyinfo");
		printf("[ERROR]file:%s line:%d malloc size:%d fail.\n", file, lineno, size);
		system("cat /proc/buddyinfo");
		sleep(2);
		assert(p);
	}
	else
	{
		//memset(p, 0, size);
		add_mem_info(p, size, file, lineno);
	}
	
	return p;	
}

void  __ms_free(void* p, const char* file, int lineno)
{
	if (!p) 
		return;
	
	remove_mem_info(p, file, lineno);
	free(p);
	p = 0;
}

void* __ms_calloc(unsigned int n, unsigned int size, const char* file, int lineno)
{
	if(n <= 0 || size <= 0)
		return NULL;
	void *p = calloc(n, size);
	if(!p)
	{
		printf("[ERROR]file:%s line:%d calloc (%d)size:%d fail.\n", file, lineno, n, size);
		sleep(2);
		assert(p);
	}

	add_mem_info(p, size*n, file, lineno);
	return p;
}

void* __ms_valloc(unsigned int size, const char* file, int lineno)
{
	if(size <= 0) 
		return NULL;
	void* p = valloc(size);
	if(!p)
	{
		system("cat /proc/buddyinfo");
		printf("[ERROR]file:%s line:%d valloc size:%d fail.\n", file, lineno, size);
		system("cat /proc/buddyinfo");
		sleep(2);
		assert(p);
	}
	else
	{
		//memset(p, 0, size);
		add_mem_info(p, size, file, lineno);
	}
	return p;	
}

void mem_leak_init()
{
	ms_mutex_init(&global_mutex);
}

unsigned int mem_get_cur_list()
{
	unsigned int count = 0;
	ms_mutex_lock(&global_mutex);
	struct mem_leak *leak_info = ptr_start;
	while(leak_info)
	{
		count++;
		leak_info = leak_info->next;
	}
	ms_mutex_unlock(&global_mutex); 
	return count;
}

void mem_leak_report()
{
	struct mem_leak *leak_info;
	unsigned int total_size = 0, count = 0;

	FILE *fp_write = fopen (MEM_LEAK_OUTPUT_FILE, "wt");

	if(fp_write != NULL)
	{
		char *info = (char *)malloc(MAX_LEAK_FILE_SIZE);
		memset(info, 0, MAX_LEAK_FILE_SIZE);
	
		sprintf(info, "%s\n", "Memory Leak Summary");
		fwrite(info, (strlen(info) + 1) , 1, fp_write);
		sprintf(info, "%s\n", "-----------------------------------");	
		fwrite(info, (strlen(info) + 1) , 1, fp_write);
		
		ms_mutex_lock(&global_mutex);
		for(leak_info = ptr_start; leak_info != NULL; leak_info = leak_info->next)
		{
			sprintf(info, "address : %p\n", (unsigned int *)leak_info->info.address);
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			sprintf(info, "size    : %u bytes\n", leak_info->info.size);	
			total_size += leak_info->info.size;
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			sprintf(info, "file    : %s\n", leak_info->info.file);
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			sprintf(info, "line    : %u\n", leak_info->info.line);
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			sprintf(info, "%s==index:%d\n", "-----------------------------------", count);	
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			count++;
		}
		ms_mutex_unlock(&global_mutex);
		
		sprintf(info, "\ntotal count : %u\n", count);
		fwrite(info, (strlen(info) + 1) , 1, fp_write);
		sprintf(info, "\ntotal size : %u\n", total_size);
		fwrite(info, (strlen(info) + 1) , 1, fp_write);
		
		free(info);
		fclose(fp_write);
	}	

	ms_mutex_lock(&global_mutex);
	__ms_clear();
	ms_mutex_unlock(&global_mutex);
	ms_mutex_uninit(&global_mutex);
}

void mem_leak_show()
{
	struct mem_leak *leak_info;
	unsigned int total_size = 0, count = 0;
    char fileName[64];
    char time[32];

	time_to_string_ms(time);
    sprintf(fileName, "/mnt/nand3/mem_%s", time);
    
	FILE *fp_write = fopen (fileName, "wt");

	if(fp_write != NULL)
	{
		char *info = (char *)malloc(MAX_LEAK_FILE_SIZE);
		memset(info, 0, MAX_LEAK_FILE_SIZE);
	
		sprintf(info, "%s\n", "Memory Leak Summary");
		fwrite(info, (strlen(info) + 1) , 1, fp_write);
		sprintf(info, "%s\n", "-----------------------------------");	
		fwrite(info, (strlen(info) + 1) , 1, fp_write);
		
		ms_mutex_lock(&global_mutex);
		for(leak_info = ptr_start; leak_info != NULL; leak_info = leak_info->next)
		{
			sprintf(info, "address : %p\n", (unsigned int *)leak_info->info.address);
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			sprintf(info, "size    : %u bytes\n", leak_info->info.size);	
			total_size += leak_info->info.size;
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			sprintf(info, "file    : %s\n", leak_info->info.file);
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			sprintf(info, "line    : %u\n", leak_info->info.line);
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			sprintf(info, "time    : %s\n", leak_info->info.time);
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			sprintf(info, "%s==index:%d\n", "-----------------------------------", count);	
			fwrite(info, (strlen(info) + 1) , 1, fp_write);
			count++;
		}
		ms_mutex_unlock(&global_mutex);
		
		sprintf(info, "\ntotal count : %u\n", count);
		fwrite(info, (strlen(info) + 1) , 1, fp_write);
		sprintf(info, "\ntotal size : %u\n", total_size);
		fwrite(info, (strlen(info) + 1) , 1, fp_write);
		
		free(info);
		fclose(fp_write);
	}	
}

#else
void* __ms_realloc(void *ptr, unsigned int size, const char* file, int lineno)
{
	void* p = realloc(ptr, size);
	if(!p)
	{
		system("cat /proc/buddyinfo");
		printf("[ERROR]file:%s line:%d malloc size:%d fail.\n", file, lineno, size);
		system("cat /proc/buddyinfo");
		sleep(2);
		assert(p);
	}
	else
	{
	}
	return p;		
}

void* __ms_malloc(unsigned int size, const char* file, int lineno)
{
	if(size <= 0) 
		return NULL;
	void* p = malloc(size);
	if(!p)
	{
		system("cat /proc/buddyinfo");
		printf("[ERROR]file:%s line:%d malloc size:%d fail.\n", file, lineno, size);
		system("cat /proc/buddyinfo");
		sleep(2);
		assert(p);
	}
	else
	{
		//memset(p, 0, size);
	}
	
	return p;	
}

void  __ms_free(void* p, const char* file, int lineno)
{
	if (!p) 
		return;
	free(p);
	p = 0;
}

void* __ms_calloc(unsigned int n, unsigned int size, const char* file, int lineno)
{
	if(n <= 0 || size <= 0)
		return NULL;
	void *p = calloc(n, size);
	if(!p)
	{
		printf("[ERROR]file:%s line:%d calloc (%d)size:%d fail.\n", file, lineno, n, size);
		sleep(2);
		assert(p);
	}

	return p;
}

void* __ms_valloc(unsigned int size, const char* file, int lineno)
{
	if(size <= 0) 
		return NULL;
	void* p = valloc(size);
	if(!p)
	{
		system("cat /proc/buddyinfo");
		printf("[ERROR]file:%s line:%d valloc size:%d fail.\n", file, lineno, size);
		system("cat /proc/buddyinfo");
		sleep(2);
		assert(p);
	}
	else
	{
		//memset(p, 0, size);
	}
	
	return p;	
}

unsigned int mem_get_cur_list()
{
}
void mem_leak_init()
{
}
void mem_leak_report()
{
}
void mem_leak_show()
{
}
#endif
