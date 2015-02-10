#if !defined(__PREFETCHER_H__)
#define __PREFETCHER_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <glib.h>

#define APPID_LEN 256
#define FILENAME_LEN 256
#define PMT_SIZE 256

enum app_prefetching_stat {
	TP_STAT_PROFILING = 0,
	TP_STAT_EXTRACTING = 1,
	TP_STAT_EXTRACTED = 2,
};
enum ex_range_convered {
	EX_RANGE_COVERING = 0,
	EX_RANGE_COVERED = 1,
	EX_RANGE_OVERLAPPED = 2,
	EX_RANGE_NON_OVERLAPPED = 3,
};



/* Manager */
typedef struct _off_len_t{
	int off;
	unsigned int len;
} off_len_t;

typedef struct _name_len_t{
	char filename[FILENAME_LEN]; //key
	GList *off_len_list;
} name_len_t;

typedef struct __attribute__((packed)) _ex_appinfo{
	char appid[APPID_LEN]; //key
	GList *name_len_list;
} ex_app_info_t;

typedef struct __attribute__((packed)) _mg_app_desc{
	char appid[APPID_LEN];
	int stat;
} mg_app_desc;

typedef struct __attribute__((packed)) _profile_brief_t{
	int pid;
	char filename[FILENAME_LEN];
	size_t len;
	int offset;
} profile_brief_t;

/* Common Function */

/* Profiler */
class TP_Profiler{
	private:
		void *manager;
		int enabled;
	public:
		TP_Profiler();
		TP_Profiler(void *);
		~TP_Profiler();

		int is_created(void);
		int is_enabled(void);
		void profiler_enable(int pid);
		void profiler_disable(void);
};

/* Extractor */
typedef struct _ex_table_entry_t{
	int pid;
	char appid[APPID_LEN];
} ex_entry_t;

class TP_Extractor{
	private:
		void *manager;
		GQueue *ex_queue;
		GMutex ex_queue_mutex_lock;
		
		name_len_t *get_name_len(ex_app_info_t *app_info, 
				char *filename);
		
		
		void update_file(name_len_t *p_name_len,
				int offset,
				unsigned int length);
	public:
		TP_Extractor();
		TP_Extractor(void *manager);
		~TP_Extractor();
		
		/* Starting with pthread */
		GThread *start_extractor(void);
		
		//FIFO
		void ex_enqueue(int pid, char *appid);
		void ex_dequeue();
		ex_entry_t *ex_queue_get_curr();
		
		//File operation
		int flush_app_info(ex_app_info_t *app_info);
		
		//App information 
		int update_app_info(ex_app_info_t *app_info, 
				profile_brief_t *profile_brief);
			//get_current_app_info does not allocate new memory
			//for ex_app_info_t, but it just fill out the struct.
		int get_current_app_info(ex_app_info_t *app_info, char *appid);
		void free_app_info_content(ex_app_info_t *app_info);
		//MUTEX
		int ex_queue_lock();
		int ex_queue_unlock();
};

/* Prefetcher */
class TP_Prefetcher{
	private:
	public:
		TP_Prefetcher();
		~TP_Prefetcher();
		
		GThread *start_prefetcher(char *appid);
};

class TP_Manager{
	private:
		GTree *app_desc_table;
		GMutex adt_mutex;
		GMutex adt_dev_mutex;
		GMutex pf_dev_mutex;

	
	public:
		TP_Manager();
		~TP_Manager();
		
		int fd_profiler;
		int fd_adt;

		TP_Profiler *tp_profiler;
		TP_Extractor *tp_extractor;
		TP_Prefetcher *tp_prefetcher;
	
		void load_adt(void);
		void flush_adt(void);//Should be locked adt before use this
		mg_app_desc *get_adt_entry(char *appid);
		void add_adt_entry(char *appid);//
		void adt_dev_mutex_lock();
		void adt_dev_mutex_unlock();
		void adt_mutex_lock();
		void adt_mutex_unlock();
		void pf_dev_mutex_lock();
		void pf_dev_mutex_unlock();
};
/* dbus interface */
int dbus_listen_connection(void);

#ifdef __cplusplus
}
#endif

#endif
//!End of file
