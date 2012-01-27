#ifndef __UDPRW_H
#define __UDPRW_H

#include <sys/time.h>
#include <sys/types.h>

//sockets
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//infra-structure
#include "Common.h"
#include "Thread.h"
#include "RTPManager.h"

class UdpBase : public Thread {
protected:
	Common 		*common;
	RTPState 	*state;

public:
    UdpBase(int priority, int id, Common* common);
    virtual ~UdpBase();

    static int initTransport(Common *common);

    static int initRTPOverUDP(Common *common);

    static int initSIPOverUDP(Common *common);

    static int initRemoteAddr(Common *common,
            				   char* remoteName,
            				   int remoteRTPPort,
            				   int remoteRTCPPort,
            				   int remoteSIPPort);

    static int initTempRemoteAddr(Common *common, 
                                char* remoteName,
                                int remoteTempPort);

    static int closeTransport(Common *common);
                                
};

class UdpReader : public UdpBase {
public:
	UdpReader(Common* common);
	virtual ~UdpReader();

	void* run(void* arg);
};

class UdpWriter : public UdpBase {
public:
	UdpWriter(Common* common);
	virtual ~UdpWriter();

	void* run(void* arg);
};
#endif
