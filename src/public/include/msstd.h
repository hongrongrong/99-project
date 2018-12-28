#ifndef __MS_STD_H__
#define __MS_STD_H__
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define RECORD_PATH_MMAP	"/tmp/rdmmap"

//memory manager
#define MEM_LEAK_OUTPUT_FILE "/tmp/mem_leak"
void* __ms_malloc(unsigned int size, const char* file, int lineno);
void  __ms_free(void* p, const char* file, int lineno);
void* __ms_calloc(unsigned int n, unsigned int size, const char* file, int lineno);
void* __ms_valloc(unsigned int size, const char* file, int lineno);
void mem_leak_init();
unsigned int mem_get_cur_list();
void mem_leak_report();
void mem_leak_show();
#define ms_realloc(p, size) __ms_realloc(p, size, __FILE__, __LINE__)
#define ms_malloc(len)	__ms_malloc((len), __FILE__, __LINE__)
#define ms_free(p)		__ms_free(p, __FILE__, __LINE__)
#define ms_calloc(n, size) 	__ms_calloc(n, size, __FILE__, __LINE__)
#define ms_valloc(len) __ms_valloc((len), __FILE__, __LINE__)

//bit operation
int  ms_get_bit(unsigned long long value, int p);	//get bit No.p (from right) of value 
int  ms_set_bit(unsigned long long* value, int p, int on); //set bit No.p (from right) of value on/off
unsigned long long ms_get_bits(unsigned long long value, int p, int n); //get n bits of value at position p (from right) 
void ms_printf_bits(unsigned long long value); //printf bits

//string and time
int ms_string_strip(char s[], char c);//delete char 'c' from string s
void time_to_string(int ntime, char stime[32]);	//time change to string(fmt:year-month-day hour:minute:second)
void time_to_string_ms(char stime[32]);


//file manager
int ms_create_dir(const char* dir);
int ms_remove_dir(const char* dir);
int ms_file_existed(const char* file);
enum MEM_TYPE
{
    MEM_TYPE_TOTAL,
    MEM_TYPE_FREE,
    MEM_TYPE_PERCENT,
};
unsigned int ms_get_mem(int type);
unsigned long ms_get_file_size(const char *path);


//system
int ms_system(const char* cmd);
int ms_vsystem(const char * cmd);
FILE * ms_vpopen(const char *program, const char *type);
int ms_vpclose(FILE *iop);

//lock mutex
#define MUTEX_OBJECT pthread_mutex_t
int ms_mutex_init(MUTEX_OBJECT *mutex);
void ms_mutex_lock(MUTEX_OBJECT *mutex);
void ms_mutex_unlock(MUTEX_OBJECT *mutex);
void ms_mutex_uninit(MUTEX_OBJECT *mutex);

//rwlock
#define RWLOCK_BOJECT pthread_rwlock_t
void ms_rwlock_init(RWLOCK_BOJECT *rwlock);
void ms_rwlock_uninit(RWLOCK_BOJECT *rwlock);
int ms_rwlock_rdlock(RWLOCK_BOJECT *rwlock);
int ms_rwlock_wrlock(RWLOCK_BOJECT *rwlock);
int ms_rwlock_tryrdlock(RWLOCK_BOJECT *rwlock);
int ms_rwlock_trywrlock(RWLOCK_BOJECT *rwlock);
int ms_rwlock_unlock(RWLOCK_BOJECT *rwlock);

#define BARRIER_OBJECT pthread_barrier_t
void ms_barrier_init(BARRIER_OBJECT *barrier, unsigned int count);
void ms_barrier_uninit(BARRIER_OBJECT *barrier);
int ms_barrier_wait(BARRIER_OBJECT * barrier);



//thread
#define TASK_OK 0
#define TASK_ERROR -1
int ms_task_set_name(const char* name);
#define TASK_HANDLE pthread_t
int ms_task_create(TASK_HANDLE *handle, void *(*func)(void *), void *arg);//detached
int ms_task_create_join(TASK_HANDLE *handle, void *(*func)(void *), void *arg);//join
int ms_task_create_join_stack_size(TASK_HANDLE *handle, int size, void *(*func)(void *), void *arg);

int ms_task_join(TASK_HANDLE *handle);
int ms_task_detach(TASK_HANDLE *handle);
int ms_task_cancel(TASK_HANDLE *handle);
#define ms_task_quit(ptr) pthread_exit(ptr)


typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
#define MAX_PATH	256
#define ERR_LOG_PATH	"/mnt/nand/err_log.txt"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


/* Word color define */
#define YELLOW			"\033[0;33m"	// yellow
#define GRAY 			"\033[2;37m"
#define DGREEN 			"\033[1;32m"
#define GREEN 			"\033[0;32m"
#define DARKGRAY 		"\033[0;30m"
//#define BLACK 		"\033[0;39m"
#define BLACK 			"\033[2;30m" 	// not faint black
#define NOCOLOR 		"\033[0;39m \n"
#define CLRCOLOR		"\033[0;39m "
//#define NOCOLOR 		"\033[0m \n"
#define BLUE 			"\033[1;34m"
#define DBLUE 			"\033[2;34m"	
#define RED 			"\033[0;31m"
#define BOLD 			"\033[1m"
#define UNDERLINE 		"\033[4m"
#define CLS 			"\014"
#define NEWLINE 		"\r\n"
//#define CR 				"\r\0"
#define MAX_THREAD_POOL_NUM 10
#define MAX_THREAD_POOL_TASK 200

#define MS_SERIAL_POE_BIT (15) // is poe ?
#define MS_SERIAL_HDD_BIT (7)  // a number of hdd
#define MS_SERIAL_HDD_LEN (2)  // a number of hdd

//debug print
enum DEBUG_LEVEL
{
	DEBUG_ERR = 0x01,
	DEBUG_WRN = 0x02,
	DEBUG_INF = 0x04,
	DEBUG_GUI = 0x08,
	DEBUG_POE = 0X10,
};
#define DEBUG_LEVEL_DEFAULT 0xffffffff
extern unsigned int global_debug_mask;
typedef void (*hook_print)(char* format, ...);
extern hook_print global_debug_hook;

//mslogerr add by mjx.milesight 2014.12.17 for Stability Test start...
#define _log_err(level, prefix, format, ...) \
	do{\
		if(global_debug_mask & level){\
			char stime[32] = {0};\
			char log[1024] = {0};\
			char cmd[128] = {0};\
			int filesize = -1;\
			FILE * hfile = NULL;\
			hfile = fopen(ERR_LOG_PATH, "a");\
			if(hfile){\
				fseek(hfile, 0L, SEEK_END);\
    			filesize = ftell(hfile);\
				time_to_string(time(0), stime);\
				snprintf(log, sizeof(log), "[%s%s %d==func:%s file:%s line:%d]==" format "\n", \
				prefix, stime, getpid(), __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
				fwrite(log, 1, strlen(log), hfile);\
				fflush(hfile);\
				fclose(hfile);\
				snprintf(cmd, sizeof(cmd), "rm -rf %s", ERR_LOG_PATH);\
				if(filesize > 1024*1024)\
					system(cmd);\
			}\
		}\
    }while(0);

#define mslogerr(level, format, ...) \
	if(level == DEBUG_ERR){\
		_debug(level,  "ERR:",RED format CLRCOLOR, ##__VA_ARGS__)\
		_log_err(level, "ERR:", format, ##__VA_ARGS__)\
	}else if(level == DEBUG_WRN)\
		_log_err(level, "WRN:", format, ##__VA_ARGS__)\
	else\
		_log_err(level, "INF:", format, ##__VA_ARGS__)
//end

#define _debug(level, prefix, format, ...) \
	do{\
		if(global_debug_mask & level){\
			char stime[32] = {0};\
			time_to_string_ms(stime);\
			printf("[%s%s %d==func:%12s file:%12s line:%6d]==" format "\n", \
			prefix, stime, getpid(), __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
			if(global_debug_hook)\
				global_debug_hook("[%s%s %d==func:%12s file:%12s line:%6d]==" format "\n",\
				prefix, stime, getpid(), __func__, __FILE__, __LINE__, ##__VA_ARGS__);\
		}\
    }while(0);		

#define msdebug(level, format, ...) \
	do { \
		switch(level) \
		{ \
		case DEBUG_ERR: \
			_debug(level,  "ERR:",RED format CLRCOLOR, ##__VA_ARGS__); \
			_log_err(level, "ERR:", format, ##__VA_ARGS__); \
			break; \
		case DEBUG_WRN: \
			_debug(level, "WRN:",YELLOW format CLRCOLOR, ##__VA_ARGS__); \
			break; \
		case DEBUG_GUI: \
			_debug(level, "GUI:",GREEN format CLRCOLOR, ##__VA_ARGS__); \
			break; \
		default: \
			_debug(level, "INF:", format, ##__VA_ARGS__); \
			break; \
		} \
	}while(0)		
		
#define msprintf(format, ...) \
    do{\
		char stime[32] = {0};\
		time_to_string(time(0), stime);\
		printf("[%s %d==func:%s file:%s line:%d]==" format "\n", \
		stime, getpid(), __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
    }while(0)
    
#define msprintf_ms(format, ...) \
	do{\
		char stime[32] = {0};\
		time_to_string_ms(stime);\
		printf("[%s %d==func:%s file:%s line:%d]==" format "\n", \
		stime, getpid(), __func__, __FILE__, __LINE__, ##__VA_ARGS__); \
	}while(0)

//link table
typedef struct ms_node_t
{
    void *next;                 /**< next __node_t containing element */
    void *element;              /**< element in Current node */
} ms_node_t;

typedef struct ms_list_t
{
    int nb_elt;         /**< Number of element in the list */
    ms_node_t *node;     /**< Next node containing element  */
} ms_list_t;

typedef struct
{
	ms_node_t *actual;
	ms_node_t **prev;
	ms_list_t *li;
	int pos;
} ms_list_iterator_t;

typedef struct thread_pool_task
{
	void *(*do_pool_task)(void *arg);
	void *arg;
	struct thread_pool_task *next;
}ms_thread_pool_task;

typedef struct
{
	pthread_mutex_t queue_lock;
	pthread_cond_t queue_ready;

	ms_thread_pool_task *queue_head;
	
	pthread_t *pthreadId;
	int shutdown;
	int max_thread_num;
	int cur_queue_size;
}ms_thread_pool;

int ms_list_init (ms_list_t * li);
void ms_list_special_free (ms_list_t * li, void *(*free_func) (void *));
void ms_list_ofchar_free (ms_list_t * li);
int ms_list_size (const ms_list_t * li);
int ms_list_eol (const ms_list_t * li, int pos);
int ms_list_add (ms_list_t * li, void *element, int pos);
void *ms_list_get (const ms_list_t * li, int pos);
int ms_list_remove (ms_list_t * li, int pos);

#define ms_list_iterator_has_elem( it ) ( 0 != (it).actual && (it).pos < (it).li->nb_elt )
void *ms_list_get_first (ms_list_t * li, ms_list_iterator_t * it);
void *ms_list_get_next (ms_list_iterator_t * it);
void *ms_list_iterator_remove (ms_list_iterator_t * it);
int  ms_list_iterator_add(ms_list_iterator_t * it, void* element, int pos);

//net config
int net_get_ifaddr(const char *ifname, char *addr, int length);
int net_get_netmask(const char *ifname, char *addr, int length);
int net_get_gateway(const char *ifname, char *addr, int length);

int write_mac_conf(char *filename);
int read_mac_conf(char *mac);

int ms_sys_get_chipinfo(char* chip_info, int size);

int net_get_ipv6_ifaddr(char type, const char *ifname, char *addr, int length, char *prefix, int length2);//hrz.milesight
int net_get_ipv6_gateway(char type, const char *ifname, char *gateway, int length);//cgi popen failed in hi3798
int net_get_ipv6_gateway2(char type, const char *ifname, char *gateway, int length);//fopen
int net_is_validipv6(const char *hostname);
int net_is_haveipv6(void);
int net_is_dhcp6c_work(const char *ifname);
int net_get_dhcp6c_pid(const char *ifname, int *pid);
int net_ipv4_trans_ipv6(const char *source, char *dest, int length);
int net_get_dhcpv6gateway(const char *dhcpAddr, const char *dhcpPrefix, char *gateway, int length);//for dhcpv6 set gateway

//pool thread
void ms_pool_thread_init(ms_thread_pool *pool_task, int thread_num, void *(*pthread_main)(void *arg));
int ms_pool_add_task(ms_thread_pool *pool_task, ms_thread_pool_task *new_task);
int ms_pool_thread_uninit(ms_thread_pool *pool_task);

void ms_base64_encode(unsigned char *from, char *to, int len);
int ms_base64_decode( const unsigned char * base64, unsigned char * bindata, int bindata_len);
int Base64to16( char *s,char *ret);
char* hex_2_base64(char *_hex);
int url_decode(char *dst, const char *src, int len);
int url_encode(char *dst, const char *src, int len);
int sqa_encode(char *dst, const char *src, int len);

#ifdef __cplusplus
}
#endif


#endif 
