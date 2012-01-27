#include "SyncQueue.h"

SyncQueue::SyncQueue() {
	event = new MyEvent();
	pthread_mutex_init(&mutQ, NULL);
}

SyncQueue::~SyncQueue() {
	pthread_mutex_destroy(&mutQ);
	delete event;
}

void SyncQueue::enQ(void* data) {
	pthread_mutex_lock(&mutQ);
	dataQ.push_back(data);
	pthread_mutex_unlock(&mutQ);
}

void* SyncQueue::deQ(void) {
	void* data = NULL;
	pthread_mutex_lock(&mutQ);
	if(dataQ.size() > 0) {
		data = dataQ.front();
		dataQ.pop_front();
	}
	pthread_mutex_unlock(&mutQ);
	return data;
}

int SyncQueue::count(void) {
	int count=0;
	pthread_mutex_lock(&mutQ);
	count = dataQ.size();
	pthread_mutex_unlock(&mutQ);
	return count;
}

void* SyncQueue::waitForData(long time) {
	if(count()<1) {
		event->Wait(time);
	}
	return deQ();
}

void SyncQueue::signalData(void* data) {
	enQ(data);
	event->Signal();
}

void SyncQueue::signal(void) {
	event->Signal();
}