#include "SyncQueue.h"

SyncQueue::SyncQueue() {
#ifdef WIN32
	semAvailQ = CreateSemaphore(NULL, 0, 128, NULL); 
	InitializeCriticalSection(&critSecQ);
#else
	pthread_cond_init(&condvarAvailQ, NULL);
	pthread_mutex_init(&mutQ, NULL);
	waiting = 0;
#endif
}

SyncQueue::~SyncQueue() {
#ifdef WIN32
	CloseHandle(semAvailQ);
	DeleteCriticalSection(&critSecQ);
#else
	pthread_mutex_destroy(&mutQ);
	pthread_cond_destroy(&condvarAvailQ);
#endif
}

void SyncQueue::enQ(void* data) {
#ifdef WIN32
	EnterCriticalSection(&critSecQ);
	dataQ.push_back(data);
	LeaveCriticalSection(&critSecQ);
#else
	pthread_mutex_lock(&mutQ);
	dataQ.push_back(data);
	pthread_mutex_unlock(&mutQ);
#endif
}

void* SyncQueue::deQ(void) {
	void *data = NULL;
#ifdef WIN32
	EnterCriticalSection(&critSecQ);
	if(dataQ.size() > 0) {
		data = dataQ.front();
		dataQ.pop_front();
	}
	LeaveCriticalSection(&critSecQ);
#else
	pthread_mutex_lock(&mutQ);
	if(dataQ.size() > 0) {
		data = dataQ.front();
		dataQ.pop_front();
	}
	pthread_mutex_unlock(&mutQ);
#endif
	return data;
}

int SyncQueue::count(void) {
	int count=0;
#ifdef WIN32
	EnterCriticalSection(&critSecQ);
	count = dataQ.size();
	LeaveCriticalSection(&critSecQ);
#else
	pthread_mutex_lock(&mutQ);
	count = dataQ.size();
	pthread_mutex_unlock(&mutQ);
#endif
	return count;
}

void* SyncQueue::waitForData(long time) {
	void *data = NULL;
#ifdef WIN32
	if(time > 0) {
		WaitForSingleObject(semAvailQ, time);
	}
	else {
		WaitForSingleObject(semAvailQ, INFINITE);
	}
	data = deQ();
#else
	pthread_mutex_lock(&mutQ);
	if(time > 0) {		if(dataQ.size() < 1) {			waiting++;			long sec = (time/1000);
			long nsec = (time - (sec*1000))*1000000;
			struct timespec cond_var_timeout;
			struct timeval 	now;
			gettimeofday(&now, NULL);
			cond_var_timeout.tv_sec = now.tv_sec + sec;
			cond_var_timeout.tv_nsec = (now.tv_usec * 1000) + nsec;
			pthread_cond_timedwait(&condvarAvailQ, &mutQ, &cond_var_timeout);
			waiting--;		}		if(dataQ.size() > 0) {
			data = dataQ.front();
			dataQ.pop_front();
		}	}
	else {
		while(dataQ.size() < 1) {
			waiting++;
			pthread_cond_wait(&condvarAvailQ, &mutQ);
			waiting--;
		}
		data = dataQ.front();
		dataQ.pop_front();
	}
	pthread_mutex_unlock(&mutQ);
#endif
	return data;
}

void SyncQueue::signalData(void* data) {
	//enQ(data);
	//event->Signal();
#ifdef WIN32
	EnterCriticalSection(&critSecQ);
	dataQ.push_back(data);
	LeaveCriticalSection(&critSecQ);
	ReleaseSemaphore(semAvailQ, 1, NULL);
#else
	pthread_mutex_lock(&mutQ);
	dataQ.push_back(data);
	if(waiting) {
		pthread_cond_signal(&condvarAvailQ);
	}
	pthread_mutex_unlock(&mutQ);
#endif
}

void SyncQueue::signal(void) {
#ifdef WIN32
	ReleaseSemaphore(semAvailQ, 1, NULL);#else
	pthread_mutex_lock(&mutQ);
	pthread_cond_signal(&condvarAvailQ);
	pthread_mutex_unlock(&mutQ);
#endif
}
