#ifndef __RTPMANAGER_H
#define __RTPMANAGER_H

#include <sys/time.h>
#include <stdio.h>

#include "RTP.h"

class RTPState {
public:
	RTPState();
	~RTPState();
	void reset(void);

	unsigned short 	sequence;
	unsigned short 	seqWrap;

	bool 			isFirstPack;
	unsigned long   total;

	unsigned long   recvSeqNext;
	unsigned long   recvSeq;
	unsigned short 	recvSeqWrap;
	unsigned long 	nPacketsLost;
	unsigned long 	nPacketsRecvd;
	unsigned long 	nPacketsSent;
	unsigned long 	nBytesRecvd;
	unsigned long 	nBytesSent;

	struct timeval	ntpTimestamp;
	unsigned long	rtpTimestampSend;
	unsigned long	rtpTimestampRecv;

	unsigned long	tLSRSec;
	unsigned long	tLRRSec;
	unsigned long	LSR;

	unsigned int    nSRep;
	unsigned int    nRRep;

	unsigned long 	tSRLastSent;
	unsigned long 	tRRLastSent;

	unsigned long 	SSRC;
	unsigned long   remoteSSRC;

	//Jitter calculations
	double 			Dij;
	double 			Rj;
	double			Ri;
	unsigned long 	Sj;
	unsigned long 	Si;
	unsigned long 	tLSRRecvd;
	unsigned long   RTTPropDelay;

	double  		jitter_prev;
	double   		jitter;

	unsigned long	lastPacketCount;

	unsigned int	SRPeriodSec;
};


class RTPManager {
private:
	RTCPPacket 		*recvRTCPack;
	RTCPPacket 		*sendRTCPack;

	rtcpSReport 	*sRep;
	rtcpRReport 	*rRep;
	rtcpSReport 	*sRepRecv;
	rtcpRReport 	*rRepRecv;

public:
	RTPPacketFormatter 		*formatter;
	RTPPacket 		*recvPack;
	RTPPacket 		*sendPack;

	struct timeval 	currtime_r;
	struct timeval 	currtime_w;
	FILE			*fp;
	RTPState		*state;

	RTPManager();
	~RTPManager();
	int	 init(int type, char* fname);
	int  close(void);
	int  handleRTPSend(char *data,int len,char* buff,int buffSize);
	int  handleRTPRecv(char *buff,int buffSize,char* data,int len);

	int  handleRTCPRecv(char* buff, int buffSize);
	int  handleRTCPSendSR(char* buff, int buffSize);
	int  handleRTCPSendRR(char* buff, int buffSize);

	bool isTimeForSR(void);
	bool isPeerAlive(void);
};
#endif
