
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "common.h"
#include <dlog.h>
#include <glib.h>
#include <bluetooth.h>

#undef LOG_TAG
#define LOG_TAG "REMOTE_KEY_FW"

static GMainLoop* gMainLoop = NULL;
static bt_adapter_visibility_mode_e gVisibilityMode = BT_ADAPTER_VISIBILITY_MODE_NON_DISCOVERABLE;
static int gSocketFd = -1;
static bt_adapter_state_e gBtState = BT_ADAPTER_DISABLED;
static const char uuid[] = "00001101-0000-1000-8000-00805F9B34FB";

// Lifecycle of this framework
int rkf_initialize_bluetooth(void);
int rkf_finalize_bluetooth_socket(void);
int rkf_finalize_bluetooth(void);
int rkf_listen_connection(void);
int rkf_send_data(const char *, int);
void rkf_main_loop(void);

// Callbacks
void rkf_received_data_cb(bt_socket_received_data_s *, void *);
void rkf_socket_connection_state_changed_cb(int, bt_socket_connection_state_e, bt_socket_connection_s *, void *);
void rkf_state_changed_cb(int, bt_adapter_state_e, void *);
gboolean timeout_func_cb(gpointer);

int rkf_initialize_bluetooth(const char *device_name) {
	// Initialize bluetooth and get adapter state
	int ret;
	ret = bt_initialize();
	if(ret != BT_ERROR_NONE) {
		ALOGD("Unknown exception is occured in bt_initialize(): %x", ret);
		return -1;
	}

	ret = bt_adapter_get_state(&gBtState);
	if(ret != BT_ERROR_NONE) {
		ALOGD("Unknown exception is occured in bt_adapter_get_state(): %x", ret);
		return -2;
	}

	// Enable bluetooth device manually
	if(gBtState == BT_ADAPTER_DISABLED)
	{
		if(bt_adapter_set_state_changed_cb(rkf_state_changed_cb, NULL) != BT_ERROR_NONE)
		{
			ALOGE("[%s] bt_adapter_set_state_changed_cb() failed.", __FUNCTION__);
			return -3;
		}

		if(bt_adapter_enable() == BT_ERROR_NONE)
		{
			int timeout_id = -1;
			ALOGE("[%s] bt_adapter_state_changed_cb will be called.", __FUNCTION__);
			timeout_id = g_timeout_add(60000, timeout_func_cb, gMainLoop);
			g_main_loop_run(gMainLoop);
			g_source_remove(timeout_id);
		}
		else
		{
			ALOGE("[%s] bt_adapter_enable() failed.", __FUNCTION__);
			return -4;
		}
	}
	else
	{
		ALOGI("[%s] BT was already enabled.", __FUNCTION__);
	}

	// Set adapter's name
	if(gBtState == BT_ADAPTER_ENABLED) {
		char *name = NULL;
		ret = bt_adapter_get_name(&name);
		if(name == NULL) {
			ALOGD("NULL name exception is occured in bt_adapter_get_name(): %x", ret);
			return -5;
		}

		if(strncmp(name, device_name, strlen(name)) != 0) {
			if(bt_adapter_set_name(device_name) != BT_ERROR_NONE)
			{   
				if (NULL != name)
					free(name);
				ALOGD("Unknown exception is occured in bt_adapter_set_name : %x", ret);
				return -6;
			}   
		}
		free(name);
	} else {
		ALOGD("Bluetooth is not enabled");
		return -7;
	}

	//  Set visibility as BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE
	if(bt_adapter_get_visibility(&gVisibilityMode, NULL) != BT_ERROR_NONE)
	{
		LOGE("[%s] bt_adapter_get_visibility() failed.", __FUNCTION__);
		return -11; 
	}

	if(gVisibilityMode != BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE)
	{
		if(bt_adapter_set_visibility(BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE, 0) != BT_ERROR_NONE)
		{   
			LOGE("[%s] bt_adapter_set_visibility() failed.", __FUNCTION__);
			return -12; 
		}   
		gVisibilityMode = BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE;
	}
	else
	{
		LOGI("[%s] Visibility mode was already set as BT_ADAPTER_VISIBILITY_MODE_GENERAL_DISCOVERABLE.", __FUNCTION__);
	}

	// Connecting socket as a server
	ret = bt_socket_create_rfcomm(uuid, &gSocketFd);
	if(ret != BT_ERROR_NONE) {
		ALOGD("Unknown exception is occured in bt_socket_create_rfcomm(): %x", ret);
		return -8;
	}

	ret = bt_socket_set_connection_state_changed_cb(rkf_socket_connection_state_changed_cb, NULL);
	if(ret != BT_ERROR_NONE) {
		ALOGD("Unknown exception is occured in bt_socket_set_connection_state_changed_cb(): %x", ret);
		return -9;
	}

	ret = bt_socket_set_data_received_cb(rkf_received_data_cb, NULL);
	if(ret != BT_ERROR_NONE) {
		ALOGD("Unknown exception is occured in bt_socket_set_data_received_cb(): %x", ret);
		return -10;
	}

	
	return 0;
}

int rkf_finalize_bluetooth_socket(void) {
	int ret;
	sleep(5); // Wait for completing delivery
	ret = bt_socket_destroy_rfcomm(gSocketFd);
	if(ret != BT_ERROR_NONE)
	{
		ALOGD("Unknown exception is occured in bt_socket_destory_rfcomm(): %x", ret);
		return -1;
	}

	bt_deinitialize();
	return 0;
}

int rkf_finalize_bluetooth(void) {
	bt_deinitialize();
	return 0;
}

int rkf_listen_connection(void) {
	// Success to get a socket
	int ret = bt_socket_listen_and_accept_rfcomm(gSocketFd, 5);
	switch(ret) {
		case BT_ERROR_NONE:
			{
				// Success to listen and accept a connection from client
				ALOGD("listen successful");
				return 0;
			}
			break;
		case BT_ERROR_INVALID_PARAMETER:
			{
				// Invalid parameter exception
				ALOGD("Invalid parameter exception is occured in bt_socket_listen_and_accept_rfcomm()");
				return -1;
			}
			break;
		default:
			{
				// Unknown exception
				ALOGD("Unknown exception is occured in bt_socket_listen_and_accept_rfcomm(): %x", ret);
				return -2;
			}
	}
}

int rkf_send_data(const char *data, int length) {
	int ret = bt_socket_send_data(gSocketFd, data, length);
	if(ret != BT_ERROR_NONE) {
		ALOGD("RemoteKeyFW: unknown error is occured in rkf_serror_end_data()");
		return -1;
	} else {
		return 0;
	}
}

void rkf_main_loop(void) {
	g_main_loop_run(gMainLoop);
}

int gReceiveCount = 0;

// bt_socket_data_received_cb
void rkf_received_data_cb(bt_socket_received_data_s *data, void *user_data) {
	static char buffer[1024];
	char menu_string[]="menu";
	char home_string[]="home";
	char back_string[]="back";

	strncpy(buffer, data->data, 1024);
	buffer[data->data_size] = '\0';
	ALOGD("RemoteKeyFW: received a data!(%d) %s", ++gReceiveCount, buffer);

	// ACTION!
	if(strncmp(buffer, menu_string, strlen(home_string)) == 0) {
		system("/bin/echo 1 > /sys/bus/platform/devices/homekey/coordinates");
	} else if(strncmp(buffer, home_string, strlen(home_string)) == 0) {
		system("/bin/echo 11 > /sys/bus/platform/devices/homekey/coordinates");
	} else if(strncmp(buffer, back_string, strlen(home_string)) == 0) {
		system("/bin/echo 111 > /sys/bus/platform/devices/homekey/coordinates");
	}

	// Sending ack is optional
	//char ack_string[]="ack rkf ";
	//ack_string[strlen(ack_string)-1] = '0' + (gReceiveCount % 10);
	//rkf_send_data(ack_string, strlen(ack_string)+1);
}

// bt_socket_connection_state_changed_cb
void rkf_socket_connection_state_changed_cb(int result, bt_socket_connection_state_e connection_state_event, bt_socket_connection_s *connection, void *user_data) {
	if(result == BT_ERROR_NONE) {
		ALOGD("RemoteKeyFW: connection state changed (BT_ERROR_NONE)");
	} else {
		ALOGD("RemoteKeyFW: connection state changed (not BT_ERROR_NONE)");
	}

	if(connection_state_event == BT_SOCKET_CONNECTED) {
		ALOGD("RemoteKeyFW: connected");
//		if(connection != NULL) {
//			ALOGD("RemoteKeyFW: connected (%d,%s)", connection->local_role, connection->remote_address);
//		}
	} else if(connection_state_event == BT_SOCKET_DISCONNECTED) {
		ALOGD("RemoteKeyFW: disconnected");
//		if(connection != NULL) {
//			ALOGD("RemoteKeyFW: disconnected (%d,%s)", connection->local_role, connection->remote_address);
//		}
		g_main_loop_quit(gMainLoop);
	}
}

void rkf_state_changed_cb(int result, bt_adapter_state_e adapter_state, void *user_data) {
	if(adapter_state == BT_ADAPTER_ENABLED) {
		if(result == BT_ERROR_NONE) {
			ALOGD("RemoteKeyFW: bluetooth was enabled successfully.");
			gBtState = BT_ADAPTER_ENABLED;
		} else {
			ALOGD("RemoteKeyFW: failed to enable BT.: %x", result);
			gBtState = BT_ADAPTER_DISABLED;
		}
	}
	if(gMainLoop) {
		ALOGD("It will terminate gMainLoop.", result);
		g_main_loop_quit(gMainLoop);
	}
}

gboolean timeout_func_cb(gpointer data)
{
	ALOGE("timeout_func_cb");
	if(gMainLoop)
	{
		g_main_loop_quit((GMainLoop*)data);
	}
	return FALSE;
}

int main(int argc, char *argv[])
{
	int error, ret = 0;
	const char default_device_name[] = "Tizen-RK";
	const char *device_name = NULL;
	gMainLoop = g_main_loop_new(NULL, FALSE);
	ALOGD("Sever started\n");

	if(argc < 2) {
		char errMsg[] = "No bluetooth device name, so its name is set as default.";
		printf("%s\n", errMsg);
		ALOGW("%s\n", errMsg);
		device_name = default_device_name;
	} else {
		device_name = argv[1];
	}

	// Initialize bluetooth
	error = rkf_initialize_bluetooth(device_name);
	if(error != 0) {
		ret = -2;
		goto error_end_without_socket;
	}
	ALOGD("succeed in rkf_initialize_bluetooth()\n");

	// Listen connection
	error = rkf_listen_connection();
	if(error != 0) {
		ret = -3;
		goto error_end_with_socket;
	}

	// If succeed to accept a connection, start a main loop.
	rkf_main_loop();

	ALOGI("Server is terminated successfully\n");

error_end_with_socket:
	// Finalized bluetooth
	rkf_finalize_bluetooth_socket();

error_end_without_socket:
	rkf_finalize_bluetooth();
	return ret;
}

//! End of a file
