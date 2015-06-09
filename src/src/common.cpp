
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

#define REMOTE_CAMERA_MSG_LOG_FILE		"/var/log/messages"
#define FILE_LENGTH 255

static int rc_debug_file_fd;
static char rc_debug_file_buf[FILE_LENGTH];

EXTAPI void rc_log(int type , int priority , const char *tag , const char *fmt , ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	switch (type) {
		case RC_LOG_PRINT_FILE:
			rc_debug_file_fd = open(REMOTE_CAMERA_MSG_LOG_FILE, O_WRONLY|O_CREAT|O_APPEND, 0644);
			if (rc_debug_file_fd != -1) {
				vsnprintf(rc_debug_file_buf,255, fmt , ap );
				write(rc_debug_file_fd, rc_debug_file_buf, strlen(rc_debug_file_buf));
				close(rc_debug_file_fd);
			}
			break;

		case RC_LOG_SYSLOG:
			int syslog_prio;
			switch (priority) {
				case RC_LOG_ERR:
					syslog_prio = LOG_ERR|LOG_DAEMON;
					break;
					
				case RC_LOG_DBG:
					syslog_prio = LOG_DEBUG|LOG_DAEMON;
					break;

				case RC_LOG_INFO:
					syslog_prio = LOG_INFO|LOG_DAEMON;
					break;
					
				default:
					syslog_prio = priority;
					break;
			}
			
			vsyslog(syslog_prio, fmt, ap);
			break;

		case RC_LOG_DLOG:
			if (tag) {
				switch (priority) {
					case RC_LOG_ERR:
						SLOG_VA(LOG_ERROR, tag ? tag : "NULL" , fmt ? fmt : "NULL" , ap);
						break;

					case RC_LOG_DBG:
						SLOG_VA(LOG_DEBUG, tag ? tag : "NULL", fmt ? fmt : "NULL" , ap);
						break;

					case RC_LOG_INFO:
						SLOG_VA(LOG_INFO, tag ? tag : "NULL" , fmt ? fmt : "NULL" , ap);
						break;
				}
			}
			break;
	}

	va_end(ap);
}
