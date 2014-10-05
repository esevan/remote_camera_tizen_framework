
#include <syslog.h> 
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <common.h>
#include <dlog.h>
#include <stdarg.h>

#ifndef EXTAPI
#define EXTAPI __attribute__((visibility("default")))
#endif

#define RKF_SERVER_MSG_LOG_FILE		"/var/log/messages"
#define FILE_LENGTH 255

static int rkf_debug_file_fd;
static char rkf_debug_file_buf[FILE_LENGTH];

EXTAPI void rkf_log(int type , int priority , const char *tag , const char *fmt , ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	switch (type) {
		case RKF_LOG_PRINT_FILE:
			rkf_debug_file_fd = open(RKF_SERVER_MSG_LOG_FILE, O_WRONLY|O_CREAT|O_APPEND, 0644);
			if (rkf_debug_file_fd != -1) {
				vsnprintf(rkf_debug_file_buf,255, fmt , ap );
				write(rkf_debug_file_fd, rkf_debug_file_buf, strlen(rkf_debug_file_buf));
				close(rkf_debug_file_fd);
			}
			break;

		case RKF_LOG_SYSLOG:
			int syslog_prio;
			switch (priority) {
				case RKF_LOG_ERR:
					syslog_prio = LOG_ERR|LOG_DAEMON;
					break;
					
				case RKF_LOG_DBG:
					syslog_prio = LOG_DEBUG|LOG_DAEMON;
					break;

				case RKF_LOG_INFO:
					syslog_prio = LOG_INFO|LOG_DAEMON;
					break;
					
				default:
					syslog_prio = priority;
					break;
			}
			
			vsyslog(syslog_prio, fmt, ap);
			break;

		case RKF_LOG_DLOG:
			if (tag) {
				switch (priority) {
					case RKF_LOG_ERR:
						SLOG_VA(LOG_ERROR, tag ? tag : "NULL" , fmt ? fmt : "NULL" , ap);
						break;

					case RKF_LOG_DBG:
						SLOG_VA(LOG_DEBUG, tag ? tag : "NULL", fmt ? fmt : "NULL" , ap);
						break;

					case RKF_LOG_INFO:
						SLOG_VA(LOG_INFO, tag ? tag : "NULL" , fmt ? fmt : "NULL" , ap);
						break;
				}
			}
			break;
	}

	va_end(ap);
}
