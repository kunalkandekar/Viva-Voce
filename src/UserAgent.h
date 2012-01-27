#ifndef __USERAGENT_H
#define __USERAGENT_H

#include "Common.h"
#include "ResourceManager.h"
#include "Thread.h"
#include "SIP.h"
#include "SDP.h"
#include "UdpRW.h"

class UserAgent:public Thread {
private:
	Common* common;
	int state;
	Thread* audioReader;

	Memo	*memo;
	void	*data;
	int 	size;
	int 	ret;
	int 	itr;
	char	tag[20];
	char 	branch[20];

	char	sdpBuffer[500];
	char 	buffer[1000];

	bool  	checkSrc;
	bool  	discard;
	int   	remoteRtpPort;
	int   	remoteRtcpPort;
	int   	remoteSipPort;
	char* 	remoteId;
	char* 	remoteHost;

	char* 	localId;
	char* 	localHost;

	char* 	temp;
	double 	callId;
	char* 	callIdHost;

	long	waitMSec;
	int		defaultAction;
	int 	rptCount;
	char 	*name;
	int		cSeqCode;
	char 	*cSeqMethod;

	//init(co);

	SIPPacket*  sipPackSend;
	SIPPacket*  sipPackRecv;

	SDPPacket*  sdpPackSend;
	SDPPacket*  sdpPackRecv;

	SDPConnectionInfo* 	  sdpConnSend;
	SDPMediaDescription*  sdpMediaSend;
	SDPMediaDescription*  sdpMediaRecv;
	SDPTimeDescription*	  sdpTimeSend;

	int	 sendRequest(int code);
	void sendInvite(void);
	void sendBye(void);
	void sendOk(void);
	void sendAck(void);
	void sendDecline(void);
	void sendRinging(void);
	void sendBusy(void);
	void checkPacket(void);
	void doDefaultAction(void);
	void cleanup(void);
	void init(int port);
public:
	int  setThreadsToSignal(Thread* audioReader);
	bool isBusy(void);

	UserAgent(Common* common);
	virtual ~UserAgent();

	void* run(void* arg);

};

#endif
