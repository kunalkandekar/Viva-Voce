#ifndef __SYNCQUEUE_H
#define __SYNCQUEUE_H
#include <deque>

#ifdef WIN32
#include <conio.h>
#include <process.h>
#include <windows.h>
#include <winbase.h>
#else

#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#endif

using namespace std;
//typedef vector<void*> voidVector;

class SyncQueue {
private:
#ifdef WIN32
	HANDLE				semAvailQ;
	CRITICAL_SECTION	critSecQ;
#else
	pthread_cond_t		condvarAvailQ;
	pthread_mutex_t 	mutQ;
	int					waiting;
#endif
	deque <void*> 		dataQ;

public:
	SyncQueue();
	~SyncQueue();

	void enQ(void* memo);

	void* deQ(void);

	int count(void);

	void signalData(void* data);
	void* waitForData(long time);

	void signal(void);

};

#endif