#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <arph/inet.h>
#include <netinet/in.h>

//#include <glib.h> Already included at "prefetcher.h"
#include <dlog.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "common.h"

#define RC_KB 1024
#define RC_MB (1024*RC_KB)
#define FRAME_BIT 0x01
#define FRAME_SIZ (10*RC_MB)
#define FRAME_BUF_PATH "/dev/rc_mem"
#define FRAME_NUM 2
#define PORT_NUM 9488
#define PORT_BACK_LOG 5
#define MINI(x,y) ((x<y)? x:y)

static char *frame[FRAME_NUM] = {NULL, NULL};
static int frame_num = 0;

static GMainLoop *gMainLoop = NULL;
static DBusConnection *connection;
static DBusError error;

static int servSock = 0;
static int rcvdStat = 0;
static GMutex rcvdStat_mutex;

static int fd_rcMem = 0;


int initializeSock()
{
	int i, res;
	int servPort = PORT_NUM;
	res = 0;

	for(i = 0; i < FRAME_NUM; i++){
		frame[i] = (char *)malloc(sizeof(FRAME_SIZ));
		if(NULL == frame[i]){
			int j;
			for(j=0; j<i; j++){
				ERR("Frame is not allocated");
				free(frame[j]);
				return -1;
			}
		}
	}
	servSock = socket(PF_INET, SOCK_STREAM, 0);
	if(servSock == -1){
		ERR("Socket has not been created");
		return -1;
	}

	struct sockaddr_in serv_addr;
	memset (&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr,sin_addr.s_addr = htonl (INADDR_ANY);
	serv_addr.sin_port = htons(servPort);
	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
		ERR("Binding socket failed");
		close(servSock);
		servSock = 0;
		return -1;
	}

	return 0;
}
int deinitializeSock()
{
	close(servSock);
	servSock = 0;
	return 0;
}
gpointer rcvSockThread(gpointer data)
{
	int ret = 0;

	while(1){
		if(rcvdStat){
			if(servSock <= 0){
				ERR("Server socket tries to be re-initailized");
				if(0 != initializeSock()){
					ERR("Server socket initailization failed");
					servSock = 0;
					sleep(1);
					continue;
				}
			}else{
				if(listen(servSock, PORT_BACK_LOG) == -1){
					ERR("listen error");
					deinitializeSock();
					sleep(1);
					continue;
				}
				struct sockaddr_in clnt_addr;
				socklen_t clnt_addr_len = sizeof(clnt_addr);
				int clnt_sock = accept (serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_len);
				if (clnt_sock == -1){
					ERR("Accept() error");
					deinitializeSock();
					sleep(1);
					continue;
				}

				ERR("Connnected from %s:%d\n", inet_ntoa(clnt_addr.sin_adrr), ntohs(clnt_addr.sin_port));

				while(rcvdStat){
					ssize_t numBytesRcvd = 0;
					uint32_t netFileSize;
					uint32_t fileSize;
					
					numBytesRcvd = recv(clnt_sock, &netFileSize, sizeof(netFileSize), MSG_WAITALL);
					if(numBytesRcvd == -1){
						ERR("Receiving error");
					}else if(numBytesRcvd == 0){
						ERR("Peer connection closed");
						close(clnt_sock);
						deinitializeSock();
					}else if(
					ERR("File Size %d \n", rcvdFileSize);
					
					while(rcvdFileSize < fileSize){
						numBytesRcvd = recv(clnt_sock, frame[frame_num] + rcvdFileSize, fileSize-rcvdFileSize, MSG_WAITALL);
						if (numBytesRcvd == -1){
						ERR("Receving error");
						}else if(numBytesRcvd == 0){
						ERR("Peer connection closed");
						}

						rcvdFileSize += numBytesRcvd;
					}

					write(fd_rcMem, frame[frame_num], fileSize);

				}
				
				close(clnt_sock);
				deinitializeSock();
			}
		}else{
			sleep(1);
		}
	}
}
static DBusHandlerResult dbus_filter(DBusConnection *connection,
		DBusMessage *message,
		void *user_data){
	DBusError error = {0};
	dbus_error_init(&error);
	
	ERR("Dbus got signal");
//	if(dbus_message_is_signal(message, "User.Prefetcher.API", "App_Start")){
//		int pid = 0;
//		char *appid = NULL;//(char*)malloc(256);
//		mg_app_desc *app_desc = NULL;
//
//		if(dbus_message_get_args(message, &error,
//				DBUS_TYPE_INT32, &pid,
//				DBUS_TYPE_STRING, &appid,
//				DBUS_TYPE_INVALID) == FALSE){
//			ERR("Failed to get arguments");
//		}
//		
	if(dbus_message_is_signal(message, "User.RC.API", "Rcvd_Start")){
		g_mutex_lock(&rcvdStat_mutex);
		rcvdStat_mutex = 1;
		g_mutex_unlock(&rcvdStat_mutex);
		ERR("Receiving Started");
		goto out;
	}
	else if(dbus_message_is_signal(message, "User.RC.API", "Rcvd_Stop")){
		g_mutex_lock(&rcvdStat_mutex);
		rcvdStat_mutex = 0;
		g_mutex_unlock(&rcvdStat_mutex);
		ERR("Receiving Stopped");
		goto out;
	}

	out:
		ERR("Signal has been processed");
 		return DBUS_HANDLER_RESULT_HANDLED;
}

gpointer rc_dbus_listener(gpointer data){
	while(dbus_connection_read_write(connection, -1)){
		dbus_connection_read_write_dispatch(connection, -1);
	}
	ERR("Connection has been closed");
	return NULL;
}
GThread *start_dbus_listener(){
	GThread *dbus_thread = NULL;
	
	dbus_thread = g_thread_new("dbus listener Thread", rc_dbus_listener, NULL);

	if(NULL == dbus_thread){
		ERR("Thread has not been created");
		return NULL;
	}
	
	return dbus_thread;
}

int dbus_connection_initialize(){
	

	dbus_error_init(&error);

	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

	if(dbus_error_is_set(&error)){
		ERR("Error connecting to the daemon bus : %s", error.message);
		dbus_error_free(&error);
		return RC_ERROR_ETC;
	}

	dbus_bus_add_match(connection, "path='/User/RemoteCamera/API',type='signal',interface='User.RemoteCamera.API'", NULL);
	dbus_connection_add_filter(connection, dbus_filter, gMainLoop, NULL);

	dbus_connection_setup_with_g_main(connection,NULL);

	return RC_ERROR_NONE;
}
////////////////////////////////////////////////////////////////////////////////////////////
/* Main Code */
int main(int argc, char *argv[])
{
	int err=0;
	GThread *rcvThread = NULL;
	ERR("[%d]REMOTE CAMERA Start", getpid());
	fprintf(stderr, "REMOTE CAMERA start");
	g_mutex_init(&rcvdStat);

	fd_rcMem = open(FRAME_BUF_PATH, O_WRONLY | O_NONBLOCK);
	if(-1 == fd_rcMem){
		ERR("RC Meme open failed");
		return -1;
	}

	if(initializeSock()){
		ERR("Socket initialization failed");
	}

	
	gMainLoop = g_main_loop_new(NULL,FALSE);
	err = dbus_connection_initialize();
	if(err){
		ERR("dbus listen connection failed");
		return -1;
	}

	rcvThread = g_thread_new("RC Frame-receving Thread", rcvSockThread, NULL);
	if(NULL == rcvThread){
		ERR("Thread creation failed");
		return -1;
	}
	
	//dbus_thread = start_dbus_listener();
	g_main_loop_run(gMainLoop);
	g_thread_join(rcvThread);
	//g_thread_join(dbus_thread);
	ERR("RC Camera End");
	return 0;
}
