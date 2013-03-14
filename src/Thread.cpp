#include "Thread.h"

ThreadParam::ThreadParam(void* _thread, void * _arg)
{
	thread =_thread;
	arg = _arg;
};

/************************ THREAD IMPLEMENTATION ****************************************/

/******** CTOR *********/
Thread::Thread()
{
	Thread(0,0,NULL);
}

Thread::Thread(int _id,  int _priority)
{
	Thread(_id, _priority, NULL);
}

Thread::Thread(int _id,  int _priority, void* _param)
{
	exitCode = 0;
	ID       = _id;
	isStop   = 0;
	running  = false;
	priority = _priority;
	status   = THR_STATUS_WAITING;
	param=_param;
	

	thrParam = new ThreadParam(this, param);

//	if(priority>0)
//		setPriority(priority);
#ifdef WIN32
	//
#else
	pthread_attr_init(&attr);
	pthread_mutex_init(&keyMutex, NULL);
	pthread_key_create(&key, NULL);
#endif
}

/******** DTOR *********/

Thread::~Thread()
{
	delete thrParam;
	running  = false;	
#ifdef WIN32
	//
#else
	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&keyMutex);
	pthread_key_delete(key);
	//pthread_exit(&status);   //do not use here, leads to memory leaks when freeing subclasses
#endif
}

/******** START *********/

int 	Thread::start()
{
    if (running)
        return 1;  //already running
#ifdef WIN32
	DWORD ret =0;
	ret =  _beginthreadex( NULL, 0, Thread::starter, thrParam, 0, &thread_id);
	if(ret==0)
		return -1;
	else
		hThread = (HANDLE)ret;
	ret =0;
#else
	int ret = 0;

//  ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);  //this is giving errors on linux and osx

	if(ret==0)
		ret = pthread_create(&thread_id, &attr, starter, thrParam);
#endif

	return ret;
}

int 	Thread::startDetached()
{
	int ret;
#ifdef WIN32
	return 0;
#else
	ret = pthread_attr_setinheritsched(&attr,PTHREAD_INHERIT_SCHED);

	if(ret==0)
		ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);

	if(ret==0)
		ret = pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

	/* start thread */
	if(ret==0)
		ret = pthread_create(&thread_id, &attr, starter, thrParam);
#endif
	return ret;
}

int		Thread::join(int* status)
{
	int ret=-1;
#ifdef WIN32
    WaitForSingleObject(hThread); //??... untested
	return 0;
#else
	if(!pthread_equal(pthread_self(), thread_id))
		ret = pthread_join(thread_id, (void**)status);
	return ret;
#endif
}

int		Thread::detach(void)
{
	int ret=-1;
#ifdef WIN32
	return 0;
#else
	if(!pthread_equal(pthread_self(), thread_id))
		ret = pthread_detach(thread_id);
	return ret;
#endif
}

/******** SIGNALLING *********/

void 	Thread::signal()
{
	event.Signal();
}

int	Thread::wait()
{
	return event.Wait();
}

int	Thread::wait(long msec)
{
	return event.Wait(msec);
}

int		Thread::kill(int sig)
{
	//return pthread_kill(thread_id, sig);
	return 0;
}

int		Thread::stop()
{
	this->isStop = 1;
	return 0;
}


/******** SETTING AND GETTING *********/

int		Thread::setKeyData(void* value)
{
	int ret=0;
#ifdef WIN32
	//
#else
	pthread_mutex_lock(&keyMutex);
	ret = pthread_setspecific(key, value);
	pthread_mutex_unlock(&keyMutex);
#endif
	return ret;
}

void* 	Thread::getKeyData(void)
{
	void *value =NULL;
#ifdef WIN32
	return NULL;
#else
	pthread_mutex_lock(&keyMutex);
	value = pthread_getspecific(key);
	pthread_mutex_unlock(&keyMutex);
	#endif
	return value;
}


void 	Thread::setParam(void* obj)
{
	param = obj;
}

void* 	Thread::getParam(void)
{
	return param;
}

int		Thread::setPriority(int priority)
{
#ifdef WIN32
	return -1;
#else
	/* sched_priority will be the priority of the thread */
	schedParam.sched_priority = priority;

	/* only supported policy, others will result in ENOTSUP */
	policy = SCHED_OTHER;

	/* scheduling parameters of target thread */
	return pthread_setschedparam(thread_id, policy, &schedParam);
#endif
}

int		Thread::getPriority(void)
{
#ifndef WIN32
	int ret = pthread_getschedparam (thread_id, &policy, &schedParam);
	if(ret==0)
		return schedParam.sched_priority;
#endif
    return priority;
}

bool	Thread::isRunning()
{
	return running;
}

/******* DEFAULT IMPLEMENTATION ******************/

void*	Thread::run(void* _thrParam)
{
	return NULL;
}

/************************ RUN METHOD ****************************************/

#ifdef WIN32
unsigned int Thread::starter(void* _thrParam)
#else
void* Thread::starter(void* _thrParam)
#endif
{
	//printf("\n New thread...");
	ThreadParam* thrParam = (ThreadParam* )_thrParam;
	Thread* thread = (Thread*) thrParam->thread;
	thread->running  = true;
	thread->run(thrParam->arg);
	thread->running  = false;
	//pthread_exit(NULL);
#ifdef WIN32
	_endthreadex(thread->exitCode);
#endif
	return NULL;
}



/************************ EVENT IMPLEMENTATION ******************************/

MyEvent::MyEvent(void)
{
#ifdef WIN32

	event = CreateEvent(0, FALSE, FALSE, 0);
#else
	int ret=0;
	status = 0;
	ret = pthread_mutex_init(&cond_var_mutex, NULL);
	ret = pthread_cond_init(&cond_var, NULL);
#endif
}

MyEvent::~MyEvent()
{
#ifdef WIN32
	CloseHandle(event);
#else
	pthread_cond_destroy(&cond_var);
	pthread_mutex_destroy(&cond_var_mutex);
	//ret = pthread_condattr_destroy(&cond_var_attr);
#endif

}

void MyEvent::Signal(void)
{
#ifdef WIN32
	SetEvent(event);
#else
	pthread_mutex_lock(&cond_var_mutex);
	if (status == 0)
		pthread_cond_signal(&cond_var);
	status = 1;
	pthread_mutex_unlock(&cond_var_mutex);
#endif

}

int MyEvent::Wait(long timeoutInMilliSec)
{
	int err=0;
	int ret=0;

	if(timeoutInMilliSec<0)
		return Wait();

#ifdef WIN32
	err=WaitForSingleObject(event, timeoutInMilliSec);
	if(err!=WAIT_OBJECT_0)
		ret=EVT_TIMEDOUT;
#else

	long sec = (timeoutInMilliSec/1000);
	long nsec = (timeoutInMilliSec - (sec*1000))*1000000;

	pthread_mutex_lock(&cond_var_mutex);
	gettimeofday(&now,NULL);
	cond_var_timeout.tv_sec = now.tv_sec + sec;
	cond_var_timeout.tv_nsec = now.tv_usec*1000+ nsec;

	err=pthread_cond_timedwait(&cond_var, &cond_var_mutex,&cond_var_timeout);

	if(err==ETIMEDOUT)
	{
		ret=EVT_TIMEDOUT;
	}
	else
		status = 0;

	pthread_mutex_unlock(&cond_var_mutex);
#endif
	return ret;
}

int MyEvent::Wait(void)
{
	int err=0;
#ifdef WIN32
	err=WaitForSingleObject(event, INFINITE);
#else
	pthread_mutex_lock(&cond_var_mutex);

	while (status == 0)
	{
		err=pthread_cond_wait(&cond_var, &cond_var_mutex);
	}
	status = 0;
	pthread_mutex_unlock(&cond_var_mutex);
#endif
	return err;
}
