#ifndef __SYNCQUEUE_H
#define __SYNCQUEUE_H
#include <deque>
#include "Thread.h"
using namespace std;
//typedef vector<void*> voidVector;

class SyncQueue {
private:
	MyEvent				*event;
	deque <void*> 		dataQ;
	pthread_mutex_t 	mutQ;

public:
	SyncQueue();
	~SyncQueue();

	void enQ(void* memo);

	void* deQ(void);

	int  count(void);

	void signalData(void* data);
	void* waitForData(long time);

	void signal(void);

};

#endif