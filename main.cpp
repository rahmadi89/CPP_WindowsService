#include <Windows.h>
#include <tchar.h>
#include <Dbt.h>

//#include "Logger/plog/Log.h"
//#include "Logger/plog/Initializers/RollingFileInitializer.h"


SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
DWORD WINAPI ServiceCtrlHandler(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);

LPSTR pUserData;

#define SERVICE_NAME	const_cast<LPWSTR>(_T("MyServiceName"))


int _tmain(int argc, TCHAR* argv[])
{
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		//PLOG_ERROR << "Start returned error. Exiting...";
		return GetLastError();
	}
	return NO_ERROR;
}



VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
	DWORD Status = E_FAIL;
	
	//g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
	g_StatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, ServiceCtrlHandler, pUserData);

	if (g_StatusHandle == NULL)
	{
		//PLOG_ERROR << "Register returned error. Exiting...";
		return;
	}

	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		//PLOG_ERROR << "SetStatus returned error";
	}

	/*
	 * Perform tasks neccesary to start the service here
	 */

	// Create stop event to wait on later.
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		//PLOG_ERROR << "CreateEvent returned error. Exiting...";

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			//PLOG_ERROR << "SetStatus returned error. Exiting...";
		}
		return;
	}


	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		//PLOG_ERROR << "SetStatus returned error, Exiting...";
	}

	//Register service for recieve Device Event
	//https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerdevicenotificationa

	HDEVNOTIFY hDeviceNotify;
	//GUID InterfaceClassGuid = { 0x25dbce51, 0x6c8f, 0x4a72, 0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 };
	GUID InterfaceClassGuid = { 0x71a27cdd, 0x812a, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f };
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = InterfaceClassGuid;

	//PLOG_DEBUG << "Registering service for recieve device events...";

	hDeviceNotify = RegisterDeviceNotification(g_StatusHandle, &NotificationFilter, DEVICE_NOTIFY_SERVICE_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

	if (hDeviceNotify != NULL) {
		//PLOG_DEBUG << "Registeraion was successful.";
	}
	else
	{
		//PLOG_DEBUG << "Registeration failed: " << GetLastError();
	}

	//Disable Stopability!
	//g_ServiceStatus.dwControlsAccepted = 0;
	//g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	//g_ServiceStatus.dwWin32ExitCode = 0;
	//g_ServiceStatus.dwCheckPoint = 0;

	//if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	//{
	//	//PLOG_ERROR << "SetStatus returned error, Exiting...";
	//}

	// Start the thread that will perform the main task of the service
	HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	//PLOG_DEBUG << "Waiting for Worker Thread to complete";

	// Wait until our worker thread exits effectively signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);

	//PLOG_DEBUG << "Worker Thread Stop Event signaled";


	/*
	 * Perform any cleanup tasks
	 */
	//PLOG_DEBUG << "Performing Cleanup Operations";

	CloseHandle(g_ServiceStopEvent);

	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		//PLOG_ERROR << "SetStatus returned error";
	}

	//PLOG_DEBUG << "ServiceMain: Exit";

	return;
}

//for RegisterServiceCtrlHandler
//DWORD WINAPI ServiceCtrlHandler(DWORD CtrlCode)
//for RegisterServiceCtrlHandlerEx
DWORD WINAPI ServiceCtrlHandler(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		//PLOG_DEBUG << "SERVICE_CONTROL_STOP Request";
		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		 * Perform tasks neccesary to stop the service here
		 */

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			//PLOG_ERROR << "SetStatus returned error";
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);
		break;

	case SERVICE_CONTROL_DEVICEEVENT:
		// Do whatever need
		break;
	default:
		//PLOG_DEBUG << "UnKnown Event Requested";
		break;
	}

	return NO_ERROR;
}


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		/*
		 * Perform main service function here
		 */
	}

	return NO_ERROR;
}


