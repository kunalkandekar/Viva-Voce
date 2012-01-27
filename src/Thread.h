#ifndef THREAD_H__
#define THREAD_H__

#include <stddef.h>
#include <stdlib.h>

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

static const int THR_STATUS_STARTED = 1;
static const int THR_STATUS_WORKING = 2;
static const int THR_STATUS_WAITING = 0;
static const int THR_STATUS_STOPPED = 3;
static const int THR_STATUS_CRASHED = -1;

#ifndef WIN32
#define INFINITE -1;
#endif

#define EVT_SUCCESS		0
#define EVT_TIMEDOUT	21

class MyEvent
{
private:
#ifdef WIN32
	HANDLE event;
#else
	int status;
	//pthread_timestruct_t
	struct timespec 	cond_var_timeout;
	struct timeval 		now;
	pthread_cond_t 		cond_var;
	pthread_condattr_t 	cond_var_attr;
	pthread_mutex_t 	cond_var_mutex;// = PTHREAD_MUTEX_INITIALIZER;
#endif

public:
	MyEvent();
	~MyEvent();

	void Signal(void);
	int Wait(long timeout);
	int Wait();
};

/*class MyLock
{
private:
#ifdef WIN32
	HANDLE mutex;
#else
	int status;
	//pthread_timestruct_t
	struct timespec cond_var_timeout;
	pthread_cond_t 		cond_var;
	pthread_condattr_t 	cond_var_attr;
	pthread_mutex_t 	cond_var_mutex;// = PTHREAD_MUTEX_INITIALIZER;
#endif

public:
	MyEvent();
	~MyEvent();

	int lock(void);
	int lock(long timeout);
	int unlock(void);
};
*/


/*class XDBClient
{
public:
	XDBClient();

	socket conn;
}*/


class ThreadParam
{
private:
public:
	ThreadParam(void*, void *);
	void* thread;
	void* arg;

	//socket conn;
};

class Thread
{
private:
	//Prototype:
	//int pthread_cond_init(pthread_cond_t *cv,	const pthread_condattr_t *cattr);
	//int condition;
	MyEvent event;
	int exitCode;

#ifdef WIN32
	unsigned int	thread_id;
	HANDLE 			hThread;
#else
	pthread_attr_t 	attr;
	pthread_t		thread_id;
	pthread_key_t 	key;
	pthread_mutex_t	keyMutex;

	struct sched_param schedParam;
#endif
	//pthread_once_t once_control = PTHREAD_ONCE_INIT;
	//pthread_once(&once_control, init);

	/* initialize a condition variable to its default value */
	//ret = pthread_cond_init(&cv, NULL);
	/* initialize a condition variable */
	//ret = pthread_cond_init(&cv, &cattr);
protected:
	int 	ID;

	int wait(void);
	int wait(long);

	int 	isStop;
	bool    running;

public:
	Thread();
	Thread(int, int);
	Thread(int, int, void*);
	virtual ~Thread();

	//static pthread_mutex_t	runMutex = PTHREAD_MUTEX_INITIALIZER;;


	int 	status;
	long 	timeout;

	int		priority;
	int 	policy;

	void* 	param;

	ThreadParam* thrParam;

	int 	start(void);
	int		startDetached(void);
	int 	join(int*);
	int 	detach(void);
	int 	stop(void);
	bool    isRunning();

	int		kill(int);

	//int		pause(void);
	//int		init(void);

	void  	signal(void);

	void  	setParam(void*);
	void* 	getParam(void);

	int 	setKeyData(void*);
	void* 	getKeyData(void);

	int 	setPriority(int);
	int 	getPriority();

	virtual void* run(void*);
#ifdef WIN32
	static unsigned int __stdcall starter(void*);
#else
	static void* starter(void*);
#endif
};



#endif