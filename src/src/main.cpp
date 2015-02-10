#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

//#include <glib.h> Already included at "prefetcher.h"
#include <dlog.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "prefetcher.h"
#include "common.h"
static TP_Manager *tp_Manager = NULL;
static GMainLoop *gMainLoop = NULL;
static int priv_pid = 0;
static char priv_appid[APPID_LEN] = {0,};

/* Profiler Code */////////////////////////////////////////////////////////////////////////
//Profiler method implementation
TP_Profiler::TP_Profiler(){
	int fd_trace_event = open("/sys/kernel/debug/tracing/events/prefetcher_info/enable", O_WRONLY | O_NONBLOCK);
	if(-1 == fd_trace_event){
		ERR("Trace event file has failed to be opened");
		return;
	}

	if(0 == write(fd_trace_event, "1\n", strlen("1\n")+1)){
		ERR("Trace event cannot be enabled");
		close(fd_trace_event);
		return;
	}
	
	manager = NULL;
	
	enabled = FALSE;
	close(fd_trace_event);

	return;
}
TP_Profiler::TP_Profiler(void *manager){
	TP_Manager *tp_manager = (TP_Manager *)manager;
	TP_Profiler();
	if(NULL == manager){
		ERR("Manager has not been created");
		return;
	}

	this->manager = manager;
	
	if(-1 == tp_manager->fd_profiler){
		ERR("Profiler dev has not been opened");
		return;
	}
	
	return;
}

TP_Profiler::~TP_Profiler(){
	return;
}
int TP_Profiler::is_created(){
	TP_Manager *tp_manager = (TP_Manager *)manager;
	return (-1 != tp_manager->fd_profiler);
}
int TP_Profiler::is_enabled(){
	return enabled;
}

void TP_Profiler::profiler_enable(int pid){
	char char_pid[256] = {0,};
	TP_Manager *tp_manager = (TP_Manager *)manager;

	if(FALSE == is_created()){
		ERR("Profiler has not been created");
		return;
	}

	char_pid[0] = '1';
	sprintf(char_pid+1, "%d\n", pid);
	
	tp_manager->pf_dev_mutex_lock();
	if(0 == write(tp_manager->fd_profiler, char_pid, sizeof(char_pid))){
		ERR("Profiler has failed to be enabled");
		tp_manager->pf_dev_mutex_unlock();
		return;
	}
	tp_manager->pf_dev_mutex_unlock();
	enabled = TRUE;
	INFO("TP_Profiler enabled");
	return;
}

void TP_Profiler::profiler_disable(){
	TP_Manager *tp_manager = (TP_Manager *)manager;
	if(FALSE == is_created()){
		ERR("Profiler has not been created");
		return;
	}
	
	tp_manager->pf_dev_mutex_lock();
	if(0 == write(tp_manager->fd_profiler, "0\n", strlen("0\n")+1)){
		ERR("Profiler has failed to be disabled");
		return;
	}
	tp_manager->pf_dev_mutex_unlock();
	enabled = FALSE;
	return;
}

//////////////////////////////////////////////////////////////////////////////
/* Extractor */

TP_Extractor::TP_Extractor(){
	manager = NULL;

	ex_queue = g_queue_new();

	if(NULL == ex_queue){
		ERR("Extractor has failed to be initialized");
		return;
	}

	g_mutex_init(&ex_queue_mutex_lock);
	
}
TP_Extractor::TP_Extractor(void *manager){
	TP_Extractor();
	this->manager = manager;
}

void free_ex_entry(gpointer p_ex_entry){
	ex_entry_t *to_free = (ex_entry_t *)p_ex_entry;
	free(to_free);
}
TP_Extractor::~TP_Extractor(){
	ex_queue_lock();
		g_queue_free_full(ex_queue, free_ex_entry);
	ex_queue_unlock();
}

void TP_Extractor::ex_enqueue(int pid, char *appid){
	ex_entry_t *to_insert = NULL;

	to_insert = (ex_entry_t *)malloc(sizeof(ex_entry_t));
	if(NULL == to_insert){
		ERR("Memory allocation has failed;Out of memory");
		return;
	}
	to_insert->pid = pid;
	strncpy(to_insert->appid, appid, APPID_LEN);
	g_queue_push_tail(ex_queue, (gpointer)to_insert);

	return;
	
}

void TP_Extractor::ex_dequeue(){
	ex_entry_t *to_del = NULL;

	to_del = (ex_entry_t *)g_queue_pop_head(ex_queue);
	if(NULL == to_del){
		ERR("There's no entry in the extractor queue");
		return;
	}

	free(to_del);

	return;
}

ex_entry_t *TP_Extractor::ex_queue_get_curr(){	
	ex_entry_t *to_get = NULL;

	to_get = (ex_entry_t *)g_queue_peek_head(ex_queue);

	return to_get;
}

int TP_Extractor::ex_queue_lock(){
	g_mutex_lock(&ex_queue_mutex_lock);
	return TRUE;
}
int TP_Extractor::ex_queue_unlock(){
	g_mutex_lock(&ex_queue_mutex_lock);
	return TRUE;
}
void free_off_len(gpointer p_off_len){
	off_len_t *off_len = (off_len_t *)p_off_len;
	if(NULL == off_len){
		ERR("No off_len entry");
		return;
	}

	free(off_len);
	return;
}
void free_name_len(gpointer p_name_len){
	name_len_t *name_len = (name_len_t *)p_name_len;
	if(NULL == name_len){
		ERR("No name_len entry");
		return;
	}

	if(NULL == name_len->off_len_list){
		ERR("No off_len list");
		return;
	}

	g_list_free_full(name_len->off_len_list, free_off_len);
	free(name_len);

	return;
}
void TP_Extractor::free_app_info_content(ex_app_info_t *app_info){
	
	if(NULL == app_info){
		ERR("No app_info entry");
		return;
	}
	if(NULL == app_info->name_len_list){
		ERR("No name_len list");
		return;
	}
	g_list_free_full(app_info->name_len_list, free_name_len);

	return;
}
gpointer extract(gpointer data){
	//pid_t curr_tid = -1;
	int ret = 0;

	profile_brief_t temp_brief;
	ex_app_info_t curr_app_info;
	char curr_appid[APPID_LEN];

	TP_Manager *tp_manager = tp_Manager;
	ex_entry_t *curr_ex_entry = NULL;
	int curr_pid = -1;
	profile_brief_t *tmp_brief = &temp_brief;
	curr_app_info.name_len_list = NULL;
	
	INFO("Extracting Start");
	/*curr_tid = gettid();
	setpriority(PRIO_PROCESS, curr_tid, 20);//Extractor could be delayed
	*/
	tp_Manager->tp_extractor->ex_queue_lock();
	{	//Critical Section for update currently extracting pid and appid
		curr_ex_entry = tp_Manager->tp_extractor->ex_queue_get_curr();
		if(NULL != curr_ex_entry){
			curr_pid = curr_ex_entry->pid;
			memcpy(curr_appid, curr_ex_entry->appid, APPID_LEN);
		}
	}
	tp_Manager->tp_extractor->ex_queue_unlock();
	
	if(-1 != curr_pid && TP_ERROR_NONE != tp_manager->tp_extractor->get_current_app_info(&curr_app_info, curr_appid)){
		ERR("There's no app information managed");
		return NULL;
	}

retry_fetch:
	INFO("Extracting Retry");
	tp_manager->pf_dev_mutex_lock();
	ret = read(tp_manager->fd_profiler, tmp_brief, sizeof(profile_brief_t));
	tp_manager->pf_dev_mutex_unlock();
	
	while(0 < ret){
		if(curr_pid == tmp_brief->pid){
			if(TP_ERROR_NONE != tp_manager->tp_extractor->update_app_info(&curr_app_info, tmp_brief)){
				ERR("App Information update failed");
				return NULL;
			}
		}else{
			// Flush currently extracting information
			if(0 < curr_pid){
				if(TP_ERROR_NONE != tp_manager->tp_extractor->flush_app_info(&curr_app_info)){
					ERR("Extractor has failed to flush current app information");
					return NULL;
				}
				tp_manager->tp_extractor->free_app_info_content(&curr_app_info);
			}

			tp_manager->tp_extractor->ex_queue_lock();
			{	//Critical Section for update currently extracting pid appid
				tp_manager->tp_extractor->ex_dequeue();
				curr_ex_entry = tp_manager->tp_extractor->ex_queue_get_curr();
				if(NULL == curr_ex_entry){
					ERR("There's no entry, but should be.");
					return NULL;
				}
				curr_pid = curr_ex_entry->pid;
				memcpy(curr_appid, curr_ex_entry->appid, APPID_LEN);
			}
			tp_manager->tp_extractor->ex_queue_unlock();
			//? end of critical section

			if(TP_ERROR_NONE != tp_manager->tp_extractor->get_current_app_info(&curr_app_info, curr_appid)){
				ERR("There's no app information managed");
				return NULL;
			}
			if(TP_ERROR_NONE != tp_manager->tp_extractor->update_app_info(&curr_app_info, tmp_brief)){
				ERR("App information update failed");
				return NULL;
			}
		}

		tp_manager->pf_dev_mutex_lock();
		ret = read(tp_manager->fd_profiler, tmp_brief, sizeof(profile_brief_t));
		tp_manager->pf_dev_mutex_unlock();
	}
	sleep(5);
	goto retry_fetch;	
}

void flush_off_len(gpointer p_off_len_entry, gpointer user_data){
	int *p_fd_app_info = (int *)user_data;
	off_len_t *off_len_entry = (off_len_t *)p_off_len_entry;

	write(*p_fd_app_info, &(off_len_entry->off), sizeof(int));
	write(*p_fd_app_info, &(off_len_entry->len), sizeof(unsigned int));
}
void flush_name_len(gpointer p_name_len_entry, gpointer user_data){
	int *p_fd_app_info = (int *)user_data;
	guint off_len_length = 0;
	name_len_t *name_len_entry = (name_len_t *)p_name_len_entry;

	off_len_length = g_list_length(name_len_entry->off_len_list);
	write(*p_fd_app_info, name_len_entry->filename, FILENAME_LEN);
	write(*p_fd_app_info, &off_len_length, sizeof(guint));

	g_list_foreach(name_len_entry->off_len_list, flush_off_len, user_data);	
}
int TP_Extractor::flush_app_info(ex_app_info_t *app_info){
	int fd_app_info;
	char path_to_store[FILENAME_LEN];
	mg_app_desc *app_desc = NULL;
	struct stat st = {0};
	guint name_len_length = 0;
	TP_Manager *tp_manager = (TP_Manager *)manager;
	
	if(NULL == app_info){
		ERR("App information argument doesn't exist");
		return TP_ERROR_INVALID_ARGUMENT;
	}
	if(NULL == tp_manager){
		ERR("No manager has been created");
		return TP_ERROR_ETC;
	}
	
	tp_manager->adt_mutex_lock();
	{
		app_desc = tp_manager->get_adt_entry(app_info->appid);
		if(NULL == app_desc){
			ERR("No app descriptor has been found");
			return TP_ERROR_INVALID_ARGUMENT;
		}
		app_desc->stat = TP_STAT_EXTRACTING;
	}
	tp_manager->adt_mutex_unlock();
	
	memcpy(path_to_store, "/usr/prefetcher", strlen("/usr/prefetcher")+1);
	if(stat(path_to_store, &st) == -1)
		mkdir(path_to_store, 0700);

	/*---This version is temporary version under the assumption;extracting only once is enough.---
	   If extracting doesn't finish at once,
	   this operation should be modified to the way in that
	   extractor read extracted file and compare with current file sequence.
	   It makes sure that extracting operation is done after confirmation
	   that there's only little change in that comparison.
	   */
	strncat(path_to_store, "/", 1);
	strncat(path_to_store, app_info->appid, FILENAME_LEN - strlen(path_to_store) - 1);
	fd_app_info = open(path_to_store, O_RDWR | O_CREAT | O_TRUNC, 0664);
	if(-1 == fd_app_info){
		ERR("File open failed");
		return TP_ERROR_FILE_OPEN;
	}
	name_len_length = g_list_length(app_info->name_len_list);
	write(fd_app_info, app_info->appid, APPID_LEN);
	write(fd_app_info, &name_len_length, sizeof(guint));
	g_list_foreach(app_info->name_len_list, flush_name_len, (gpointer)&fd_app_info);
	
	close(fd_app_info);

	tp_manager->adt_mutex_lock();
	{
		app_desc = tp_manager->get_adt_entry(app_info->appid);
		if(NULL == app_desc){
			ERR("No app descriptor has been found");
			return TP_ERROR_INVALID_ARGUMENT;
		}
		app_desc->stat = TP_STAT_EXTRACTED;
		tp_manager->flush_adt();
	}
	tp_manager->adt_mutex_unlock();

	return TP_ERROR_NONE;
}

int TP_Extractor::get_current_app_info(ex_app_info_t *app_info, char *appid){	
	//This function does not allocate new app_info, but just fill out the struct.
	TP_Manager *tp_manager = (TP_Manager *)manager;
	if(NULL == tp_manager){
		ERR("No TP_Manager has been initialized");
		return TP_ERROR_ETC;
	}
	
	tp_manager->adt_mutex_lock();
	{
		if(NULL == tp_manager->get_adt_entry(appid)){
			ERR("There's no adt entry corresponding to app_info to update");
			tp_manager->adt_mutex_unlock();
			return TP_ERROR_ETC;
		}
	}
	tp_manager->adt_mutex_unlock();

	memcpy(app_info->appid, appid, APPID_LEN);
	
	app_info->name_len_list = NULL;
	app_info->name_len_list = g_list_alloc();
	if(NULL == app_info->name_len_list){
		ERR("Name list is not able to be allocated");
		return TP_ERROR_ETC;
	}

	return TP_ERROR_NONE;
}
gint compare_filename(gconstpointer name_len1,
		gconstpointer name_len2){
	const char *fname1 = ((const name_len_t*)name_len1)->filename;
	const char *fname2 = ((const name_len_t*)name_len2)->filename;

	return strcmp(fname1, fname2);
}
name_len_t *TP_Extractor::get_name_len(ex_app_info_t *app_info,
		char *filename){
	GList *tmp_list = NULL;
	name_len_t tmp_name_len;
	
	if(NULL == app_info || NULL == app_info->name_len_list){
		return NULL;
	}
	memcpy((&tmp_name_len)->filename, filename, FILENAME_LEN);
	tmp_list = g_list_find_custom(app_info->name_len_list, &tmp_name_len, compare_filename);
	
	if(NULL == tmp_list)
		return NULL;
	
	return (name_len_t *)tmp_list->data;
}
/*-----------------------------------------------
 * File offset length optimization methods
 * For convenience, assume that off2 is parameter and every off_len element is ordered by offset.
 * All elements which are in the area surrounded with parameter should be removed in advance.
 * Case 0-1. off2 < off1(head)
 *      |---------| |-----------|
 *      off2        off1
 * 		new head should be added.
 * 
 * Case 0-2.
 *			 Off1
 *            |---------|
 *      |--------|
 *     Off2  
 *     head offset : off2
 *     head length : off1+len1-off2
 * 
 * Case 1. off1+len1 >= off2+len2
 *               len1
 * 		|---------------------|
 * 		off1  |---------|
 * 		     off2 len2
 *     offset : off1
 *     length : len1
 *
 * Case 2-1 . off1+len1 >= off2	 
 * 				len1
 * 		|--------------|
 * 		off1	  |----------------------|
 *				off2		len2 
 * 		offset : off1
 * 		length : off2+len2-off1 
 * 
 * Case 2-2. off1+len1 >= off2 && off2+len2 >= off3
 *           len1            off3    len3
 *      |--------------|      |--------------|
 *      off1   |-------------------------|
 *            off2         len2
 *     Offset : off1
 *     Length : off3+len3-off1
 * 		and off3 should be deleted
 *    
 * case 3-1. off2+len2 >= off3
 *     off1   len1          off3  len3
 * 		|------------|       |------------|
 *                      |---------|   
 *                     off2 len2
 *		offset3 : off2
 *      length3 : off3+len3-off2
 * 
 * case 3-2. off1+len1 < off2
 * 		|--------------|	|--------------|
 * 		off1   len1			off2   len2
 * 		
 * 		new off_len should be compared with next.
 * 
 *-------------------------------------------------*/
gint compare_offset(gconstpointer off_len1, gconstpointer off_len2){
	int offset1 = ((off_len_t *)off_len1)->off;
	int offset2 = ((off_len_t *)off_len2)->off;

	return offset2-offset1;
}
void TP_Extractor::update_file(name_len_t *p_name_len, int offset, unsigned int length){
	GList *off_len_list = NULL;
	GList *walker = NULL;
	off_len_t *walker_off_len = NULL;
	off_len_t *new_off_len = NULL;
	int ex_stat = EX_RANGE_NON_OVERLAPPED;

	if(NULL == p_name_len){
		ERR("Name length has not been passed properly");
		return;
	}
	if(NULL == p_name_len->off_len_list){
		ERR("Why off_len_list has not been initailzed?");
		p_name_len->off_len_list = g_list_alloc();
	}

	off_len_list = p_name_len->off_len_list;
	walker = off_len_list;
	while(NULL != walker){
		walker_off_len = (off_len_t *)walker->data;
		//case 0-1
		if(offset+(int)length < walker_off_len->off)
			break;
		else if(offset <= walker_off_len->off){
			//Covering
			if(offset+length >= walker_off_len->off+walker_off_len->len){
				GList *next = walker->next;

				free(walker_off_len);
				walker_off_len = NULL;
				off_len_list = g_list_delete_link(off_len_list, walker);
				p_name_len->off_len_list = off_len_list;

				walker = next;
			}
			//case 0-2
			else{
				walker_off_len->len = walker_off_len->len + walker_off_len->off - offset;
				walker_off_len->off = offset;

				offset = walker_off_len->off;
				length = walker_off_len->len;

				ex_stat = EX_RANGE_OVERLAPPED;
				break;
			}
		}
		else if(offset <= walker_off_len->off + (int)walker_off_len->len){
			//Covered
			if(offset+length <= walker_off_len->off + walker_off_len->len){
				ex_stat = EX_RANGE_COVERED;
				break;
			}
			else{
				length = offset+length-walker_off_len->off;
				offset = walker_off_len->off;
				
				GList *next = walker->next;

				free(walker_off_len);
				walker_off_len = NULL;
				off_len_list = g_list_delete_link(off_len_list, walker);
				p_name_len->off_len_list = off_len_list;

				walker = next;
			}
		}
	}

	if(EX_RANGE_NON_OVERLAPPED == ex_stat){
		new_off_len = (off_len_t *)malloc(sizeof(off_len_t));
		new_off_len->off = offset;
		new_off_len->len = length;
		off_len_list= g_list_insert_sorted(off_len_list, (gpointer)new_off_len, compare_offset);
		p_name_len->off_len_list = off_len_list;
	}

}
int TP_Extractor::update_app_info(ex_app_info_t *app_info,
		profile_brief_t *profile_brief){
	name_len_t *name_len = NULL;

	if(NULL == app_info || NULL == profile_brief){
		ERR("App information or profile brief argument doesn't exist");
		return TP_ERROR_INVALID_ARGUMENT;
	}

	if(NULL == app_info->name_len_list){
		ERR("App information has not been initialized by get_current_app_info");
		return TP_ERROR_INVALID_ARGUMENT;
	}
	
	name_len = get_name_len(app_info, profile_brief->filename);
	if(NULL == name_len){
		name_len = (name_len_t *)malloc(sizeof(name_len_t));
		name_len->off_len_list = g_list_alloc();

		app_info->name_len_list = g_list_prepend(app_info->name_len_list, (gpointer)name_len);
	}

	update_file(name_len, (int)profile_brief->offset, (unsigned int)profile_brief->len);
}

GThread *TP_Extractor::start_extractor(){
	GThread *extracting_thread = NULL;
	
	extracting_thread = g_thread_new("Extracting Thread", extract, NULL);

	if(NULL == extracting_thread){
		ERR("Thread has not been created");
		return NULL;
	}
	
	return extracting_thread;
}

/* Manager Code */
gint compare_adt_key(gconstpointer mg_app_id1, gconstpointer mg_app_id2, gpointer data){
	const char *appid1 = (const char *)mg_app_id1;
	const char *appid2 = (const char *)mg_app_id2;

	return (gint)strcmp(appid1, appid2);
}
void destroy_adt_key(gpointer app_id){
	return;
}
void destroy_adt_val(gpointer app_desc){
	mg_app_desc *desc = (mg_app_desc *)app_desc;
	
	free(desc);
	return;
}

gboolean flush_adt_entry(gpointer key, gpointer adt_entry, gpointer p_fd){
	int fd = *(int*)p_fd;
	mg_app_desc *app_desc = (mg_app_desc *)adt_entry;

	if(NULL == key || NULL == adt_entry || NULL == p_fd){
		ERR("Invalid Argument");
		return TRUE;
	}

	write(fd, app_desc, sizeof(mg_app_desc));
	return FALSE;
}

TP_Manager::TP_Manager(){
	fd_profiler = -1;
	app_desc_table = g_tree_new_full(compare_adt_key, NULL, destroy_adt_key, destroy_adt_val);

	g_mutex_init(&adt_mutex);
	g_mutex_init(&pf_dev_mutex);
	g_mutex_init(&adt_dev_mutex);
	
	tp_profiler = new TP_Profiler(this);
	tp_extractor = new TP_Extractor(this);
	tp_prefetcher = new TP_Prefetcher();

	fd_adt = -1;

	fd_profiler = open("/dev/profiling_info", O_RDWR | O_NONBLOCK);
	if(fd_profiler <= 0){
		ERR("Profiler Device has not been openend");
		return;
	}

	load_adt();

	INFO("TP_Manager has been created");
}
TP_Manager::~TP_Manager(){
	g_tree_destroy(app_desc_table);
	delete tp_profiler;
	delete tp_extractor;
	delete tp_prefetcher;
	tp_profiler = NULL;
	tp_extractor = NULL;
	tp_prefetcher = NULL;
	
	sync();

	g_tree_destroy(app_desc_table);
	close(fd_profiler);
	close(fd_adt);
}

void TP_Manager::load_adt(void){
	char path_to_load[FILENAME_LEN];
	mg_app_desc *p_app_desc;
	int ret;
	struct stat st = {0};
	
	if(fd_adt <= 0){
		memcpy(path_to_load, "/usr/prefetcher", strlen("/usr/prefetcher")+1);
		if(stat(path_to_load, &st) == -1)
			mkdir(path_to_load, 0700);

		strncat(path_to_load, "/", 1);
		strncat(path_to_load, "adt_table", FILENAME_LEN - strlen(path_to_load) - 1);

		fd_adt = open(path_to_load, O_RDWR);
		if(fd_adt <= 0){
			ERR("App descriptor storage has not been created");
			return;
		}
	}

	ret = 0;
	p_app_desc = NULL;
	p_app_desc = (mg_app_desc *)calloc(1, sizeof(mg_app_desc));
	
	adt_dev_mutex_lock();
	ret = read(fd_adt, p_app_desc, sizeof(mg_app_desc));
	adt_mutex_lock();
	while(ret > 0){
		if(p_app_desc->appid[0] == '\0')
			break;
		g_tree_insert(app_desc_table,(gpointer)p_app_desc->appid, (gpointer)p_app_desc);
		p_app_desc = (mg_app_desc *)calloc(1, sizeof(mg_app_desc));
		ret = read(fd_adt, p_app_desc, sizeof(mg_app_desc));
	}
	adt_mutex_unlock();
	adt_dev_mutex_unlock();

	free(p_app_desc);
	p_app_desc = NULL;
		
}

void TP_Manager::flush_adt(){
	char path_to_flush[FILENAME_LEN];
	struct stat st = {0};

	memcpy(path_to_flush, "/usr/prefetcher", strlen("/usr/prefetcher")+1);
	if(stat(path_to_flush, &st) == -1)
		mkdir(path_to_flush, 0700);

	strncat(path_to_flush, "/", 1);
	strncat(path_to_flush, "adt_table", FILENAME_LEN - strlen(path_to_flush) - 1);


	fd_adt = open(path_to_flush, O_RDWR | O_CREAT | O_TRUNC, 0664);
	if(fd_adt <= 0){
		ERR("App descriptor storage has not been created");
		return;
	}

	if(NULL == app_desc_table){
		ERR("App descriptor table has not been initialized");
		return;
	}
	adt_dev_mutex_lock();
	g_tree_foreach(app_desc_table, flush_adt_entry, &fd_adt);
	fsync(fd_adt);
	adt_dev_mutex_unlock();
}

mg_app_desc *TP_Manager::get_adt_entry(char *appid){
	if(NULL == app_desc_table){
		ERR("App dscriptor table has not been initialized");
		return NULL;
	}

	return (mg_app_desc *)g_tree_lookup(app_desc_table, (gconstpointer)appid);
}

void TP_Manager::add_adt_entry(char *appid){
	mg_app_desc *p_app_desc = (mg_app_desc *)malloc(sizeof(mg_app_desc));

	p_app_desc->stat = TP_STAT_PROFILING;
	strncpy(p_app_desc->appid, appid, APPID_LEN);

	g_tree_insert(app_desc_table, (gpointer)p_app_desc->appid, (gpointer)p_app_desc);
}

void TP_Manager::adt_dev_mutex_lock(){
	g_mutex_lock(&adt_dev_mutex);
}
void TP_Manager::adt_dev_mutex_unlock(){
	g_mutex_unlock(&adt_dev_mutex);
}
void TP_Manager::adt_mutex_lock(){
	g_mutex_lock(&adt_mutex);
}
void TP_Manager::adt_mutex_unlock(){
	g_mutex_unlock(&adt_mutex);
}
void TP_Manager::pf_dev_mutex_lock(){
	g_mutex_lock(&pf_dev_mutex);
}
void TP_Manager::pf_dev_mutex_unlock(){
	g_mutex_unlock(&pf_dev_mutex);
}




//////////////////////////////////////////////////////////////////////////////////
/* Prefetcher Code */
TP_Prefetcher::TP_Prefetcher(){
}
TP_Prefetcher::~TP_Prefetcher(){
}
gpointer prefetch(gpointer g_appid){
	char app_path[FILENAME_LEN];
	char read_appid[APPID_LEN];
	char read_filename[FILENAME_LEN];
	int read_off;
	unsigned int read_len;

	int no_files = 0;
	int no_offs = 0;
	int fd_app_info = -1;
	int fd_read_file = -1;
	int ret = 0;
	char *appid = (char *)g_appid;
	//pid_t curr_tid = -1;
	
	/*curr_tid = gettid();
	setpriority(PRIO_PROCESS, curr_tid, -20);//Prefetcher should be scheduled at high prio.
	*/

	if(NULL == appid || '\0' == appid[0]){
		ERR("Invalid argument");
		return NULL;
	}
	strncpy(app_path, "/usr/prefetcher/", strlen("/usr/prefetcher/"));
	strncpy(app_path, appid, strlen(appid));
	
	fd_app_info = open(app_path, O_RDONLY);
	
	if(fd_app_info <= 0){
		ERR("File open error");
		return NULL;
	}
	/*	App Info file strucutre
	   APPID | Num of Files | File Name | Num of Offsets | Offset | Length | ...
	   */
	
	ret = read(fd_app_info, read_appid, APPID_LEN);
	if(ret <= 0){
		ERR("File read error");
		close(fd_app_info);
		return NULL;
	}
	ret = read(fd_app_info, &no_files, sizeof(gint));
	if(ret <= 0){
		ERR("File read error");
		close(fd_app_info);
		return NULL;
	}
	while(no_files > 0){
		ret = read(fd_app_info, read_filename, FILENAME_LEN);
		if(ret <= 0){
			ERR("File read error");
			close(fd_app_info);
			return NULL;
		}
		fd_read_file = open(read_filename, O_RDONLY);
		ret = read(fd_app_info, &no_offs, sizeof(gint));
		if(ret <= 0){
			ERR("File read error");
			close(fd_app_info);
			return NULL;
		}
		while(no_offs > 0){
			ret = read(fd_app_info, &read_off, sizeof(int));
			if(ret <= 0){
				ERR("File read error");
				close(fd_app_info);
				return NULL;
			}
			ret = read(fd_app_info, &read_len, sizeof(unsigned int));
			if(ret <= 0){
				ERR("File read error");
				close(fd_app_info);
				return NULL;
			}
			if(fd_read_file > 0)
				posix_fadvise(fd_read_file, read_off, read_len, POSIX_FADV_WILLNEED);
			
			no_offs--;
		}
		if(fd_read_file > 0)
			close(fd_read_file);
		no_files--;
	}

	close(fd_app_info);
}

GThread *TP_Prefetcher::start_prefetcher(char *appid){
	GThread *prefetching_thread = NULL;
	
	prefetching_thread = g_thread_new("Prefetching Thread", prefetch, (gpointer)appid);

	if(NULL == prefetching_thread){
		ERR("Thread has not been created");
		return NULL;
	}
	
	return prefetching_thread;
}
/* dbus Code */
static DBusHandlerResult dbus_filter(DBusConnection *connection, 
		DBusMessage *message, 
		void *user_data);
//dbus implementation
static DBusHandlerResult dbus_filter(DBusConnection *connection,
		DBusMessage *message,
		void *user_data){
	DBusError error = {0};
	dbus_error_init(&error);
	
	INFO("Prefetcher got signal");
	if(dbus_message_is_signal(message, "User.Prefetcher.API", "App_Start")){
		int pid = 0;
		char *appid = NULL;
		mg_app_desc *app_desc = NULL;

		dbus_message_get_args(message, &error,
				DBUS_TYPE_INT32, &pid,
				DBUS_TYPE_STRING, &appid,
				DBUS_TYPE_INVALID);
		
		if(NULL == appid || pid <= 0){
			ERR("DBUS Invalid Argument");
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		if(priv_pid == pid || 0 == strcmp(priv_appid, appid)){
			if(tp_Manager->tp_profiler->is_enabled())
				tp_Manager->tp_profiler->profiler_disable();
		}else{
			priv_pid = pid;
			strncpy(priv_appid, appid, APPID_LEN);
			tp_Manager->adt_mutex_lock();
			app_desc = tp_Manager->get_adt_entry(appid);
			if(NULL == app_desc){//It's first start
				tp_Manager->add_adt_entry(appid);
				tp_Manager->tp_extractor->ex_queue_lock();
				tp_Manager->tp_extractor->ex_enqueue(pid,appid);
				tp_Manager->tp_extractor->ex_queue_unlock();
				tp_Manager->tp_profiler->profiler_enable(pid);
			}else if(app_desc->stat == TP_STAT_PROFILING){
				if(tp_Manager->tp_profiler->is_enabled())
					tp_Manager->tp_profiler->profiler_disable();
			}else if(app_desc->stat == TP_STAT_EXTRACTING){
				if(tp_Manager->tp_profiler->is_enabled())
					tp_Manager->tp_profiler->profiler_disable();
			}else if(app_desc->stat == TP_STAT_EXTRACTED){
				tp_Manager->tp_prefetcher->start_prefetcher(appid);
			}else{
				ERR("Invalid TP Stat");
			}
			tp_Manager->adt_mutex_unlock();
		}
		
		dbus_free_string_array(&appid);

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if(dbus_message_is_signal(message, "User.Prefetcher.API", "App_Resume")){
		if(tp_Manager->tp_profiler->is_enabled())
			tp_Manager->tp_profiler->profiler_disable();

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if(dbus_message_is_signal(message, "User.Prefetcher.API", "App_Terminate")){
		if(tp_Manager->tp_profiler->is_enabled())
			tp_Manager->tp_profiler->profiler_disable();

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	INFO("DBus handler signal is not as same as format");
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int dbus_listen_connection(){
	DBusConnection *connection;
	DBusError error;

	dbus_error_init(&error);

	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

	if(dbus_error_is_set(&error)){
		ERR("Error connecting to the daemon bus : %s", error.message);
		dbus_error_free(&error);
		return TP_ERROR_ETC;
	}

	dbus_bus_add_match(connection, "path='/User/Prefetcher/API',type='signal',interface='User.Prefetcher.API'", NULL);
	dbus_connection_add_filter(connection, dbus_filter, gMainLoop, NULL);

	dbus_connection_setup_with_g_main(connection, NULL);

	return TP_ERROR_NONE;
}
////////////////////////////////////////////////////////////////////////////////////////////
/* Main Code */
int main(int argc, char *argv[])
{
	int err=0;
	GThread *extracting_thread = NULL;
	INFO("PREFETCHER Start");
	fprintf(stderr, "PREFETCHER start");

	gMainLoop = g_main_loop_new(NULL, FALSE);
	tp_Manager = new TP_Manager();

	extracting_thread = tp_Manager->tp_extractor->start_extractor();

	err = dbus_listen_connection();
	g_main_loop_run(gMainLoop);
	g_thread_exit(extracting_thread);
	INFO("PREFETCHER End");
	fprintf(stderr, "PREFETCHER End");
	return 0;
}
