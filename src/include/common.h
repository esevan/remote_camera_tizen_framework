
#if !defined(_COMMON_H_)
#define _COMMON_H_


#ifdef __cplusplus
extern "C"
{
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
	
enum tp_error_type {
	TP_ERROR_NONE = 0,
	TP_ERROR_INVALID_ARGUMENT = 1,
	TP_ERROR_FILE_OPEN = 2,
	TP_ERROR_ETC = 3,
};
enum tp_log_type {
	TP_LOG_PRINT_FILE		= 1,
	TP_LOG_SYSLOG			= 2,
	TP_LOG_DLOG			= 3,
};

enum tp_priority_type {
	TP_LOG_ERR			= 1,
	TP_LOG_DBG			= 2,
	TP_LOG_INFO		= 3,
};

void tp_log(int type, int priority, const char *tag, const char *fmt , ...);

#define MICROSECONDS(tv)	((tv.tv_sec * 1000000ll) + tv.tv_usec)

#ifndef __MODULE__
#include <string.h>
#define __MODULE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifdef LOG_TAG
	#undef LOG_TAG
#endif
#define LOG_TAG "APP_PF"

#if defined(_DEBUG) ||defined(USE_FILE_DEBUG)

#define DbgPrint(fmt, arg...) do { tp_log(TP_LOG_PRINT_FILE, 0, LOG_TAG, "[TP_MSG_PRINT][%s:%d] "fmt"\n",__MODULE__, __LINE__, ##arg);}while(0)

#endif

#if defined(USE_SYSLOG_DEBUG)

#define ERR(fmt, arg...) do { tp_log(TP_LOG_SYSLOG, TP_LOG_ERR, LOG_TAG, "%s:%s(%d)> "fmt, __MODULE__, __func__, __LINE__, ##arg); } while(0)
#define INFO(fmt, arg...) do { tp_log(TP_LOG_SYSLOG, TP_LOG_INFO, LOG_TAG, "%s:%s(%d)> "fmt, __MODULE__, __func__, __LINE__, ##arg); } while(0)
#define DBG(fmt, arg...) do { tp_log(TP_LOG_SYSLOG, TP_LOG_DBG, LOG_TAG, "%s:%s(%d)> "fmt, __MODULE__, __func__, __LINE__, ##arg); } while(0)


#elif defined(_DEBUG) || defined(USE_DLOG_DEBUG)

#define ERR(fmt, arg...) do { tp_log(TP_LOG_DLOG, TP_LOG_ERR, LOG_TAG, "%s:%s(%d)> "fmt, __MODULE__, __func__, __LINE__, ##arg); } while(0)
#define INFO(fmt, arg...) do { tp_log(TP_LOG_DLOG, TP_LOG_INFO, LOG_TAG, "%s:%s(%d)> "fmt, __MODULE__, __func__, __LINE__, ##arg); } while(0)
#define DBG(fmt, arg...) do { tp_log(TP_LOG_DLOG, TP_LOG_DBG, LOG_TAG, "%s:%s(%d)> "fmt, __MODULE__, __func__, __LINE__, ##arg); } while(0)

#elif defined(USE_FILE_DEBUG) 

#define ERR(fmt, arg...)	do { tp_log(TP_LOG_PRINT_FILE, 0, LOG_TAG ,"[TP_MSG_ERR][%s:%d] "fmt"\n",__MODULE__, __LINE__, ##arg); } while(0)
#define DBG(fmt, arg...)	do { tp_log(TP_LOG_PRINT_FILE, 0, LOG_TAG ,"[TP_MSG_DBG][%s:%d] "fmt"\n",__MODULE__, __LINE__, ##arg); } while(0)
#define INFO(fmt, arg...)	do { tp_log(TP_LOG_PRINT_FILE, 0, LOG_TAG ,"[TP_MSG_INFO][%s:%d] "fmt"\n",__MODULE__, __LINE__, ##arg); } while(0)

#elif defined(USE_DLOG_LOG) 

#define ERR(fmt, arg...) do { tp_log(TP_LOG_DLOG, TP_LOG_ERR, LOG_TAG, "%s:%s(%d)> "fmt, __MODULE__, __func__, __LINE__, ##arg); } while(0)
#define INFO(fmt, arg...) do { tp_log(TP_LOG_DLOG, TP_LOG_INFO, LOG_TAG, "%s:%s(%d)> "fmt, __MODULE__, __func__, __LINE__, ##arg); } while(0)

#define DBG(...)
#define DbgPrint(...)

#else
#define ERR(fmt, arg...) do { tp_log(TP_LOG_DLOG, TP_LOG_ERR, LOG_TAG, fmt , ##arg); } while(0)
						
#define DbgPrint(...)
#define DBG(...)
#define INFO(...)
#endif


#if defined(_DEBUG)
#  define warn_if(expr, fmt, arg...) do { \
		if(expr) { \
			DBG("(%s) -> "fmt, #expr, ##arg); \
		} \
	} while (0)
#  define ret_if(expr) do { \
		if(expr) { \
			DBG("(%s) -> %s() return", #expr, __FUNCTION__); \
			return; \
		} \
	} while (0)
#  define retv_if(expr, val) do { \
		if(expr) { \
			DBG("(%s) -> %s() return", #expr, __FUNCTION__); \
			return (val); \
		} \
	} while (0)
#  define retm_if(expr, fmt, arg...) do { \
		if(expr) { \
			ERR(fmt, ##arg); \
			DBG("(%s) -> %s() return", #expr, __FUNCTION__); \
			return; \
		} \
	} while (0)
#  define retvm_if(expr, val, fmt, arg...) do { \
		if(expr) { \
			ERR(fmt, ##arg); \
			DBG("(%s) -> %s() return", #expr, __FUNCTION__); \
			return (val); \
		} \
	} while (0)

#else
#  define warn_if(expr, fmt, arg...) do { \
		if(expr) { \
			ERR(fmt, ##arg); \
		} \
	} while (0)
#  define ret_if(expr) do { \
		if(expr) { \
			return; \
		} \
	} while (0)
#  define retv_if(expr, val) do { \
		if(expr) { \
			return (val); \
		} \
	} while (0)
#  define retm_if(expr, fmt, arg...) do { \
		if(expr) { \
			ERR(fmt, ##arg); \
			return; \
		} \
	} while (0)
#  define retvm_if(expr, val, fmt, arg...) do { \
		if(expr) { \
			ERR(fmt, ##arg); \
			return (val); \
		} \
	} while (0)

#endif


#ifdef __cplusplus
}
#endif

#endif
//! End of a file
