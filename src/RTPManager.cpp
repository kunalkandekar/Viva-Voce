#include "RTPManager.h"

#define mod(d) (d<0?-d:d)

RTPState::RTPState(){
	sequence=0;
	isFirstPack = false;

	//statistics
	nPacketsRecvd = 0;
	nPacketsSent  = 0;

	sequence=0;
	seqWrap=0;

	recvSeqNext=0;
	recvSeq=0;
	recvSeqWrap=0;

	nSRep=0;
	nRRep=0;


	lastPacketCount=0;
	nPacketsLost=0;

	//Jitter calculations
	Dij = 0;
	Rj = 0;
	Ri = 0;
	Sj = 0;
	Si = 0;
	tLSRRecvd = 0;

	jitter_prev = 0;
	jitter = 0;
	SRPeriodSec = 5;
}

RTPState::~RTPState() {
}


void RTPState::reset(void){
	sequence=0;
	isFirstPack = false;

	//statistics
	nPacketsRecvd = 0;
	nPacketsSent  = 0;

	sequence=0;
	seqWrap=0;

	recvSeqNext=0;
	recvSeq=0;
	recvSeqWrap=0;

	nSRep=0;
	nRRep=0;

	struct timeval currtime;
	gettimeofday(&currtime,NULL);
	tSRLastSent=currtime.tv_sec+30;

	lastPacketCount=0;
	nPacketsLost=0;

	//Jitter calculations
	Dij = 0;
	Rj = 0;
	Ri = 0;
	Sj = 0;
	Si = 0;
	tLSRRecvd = 0;

	jitter_prev = 0;
	jitter = 0;
	SRPeriodSec = 5;
}


RTPManager::RTPManager(void) {
	fp = NULL;    
	formatter = new RTPPacketFormatter();
	state  = new RTPState();
	recvPack  = new RTPPacket();
	sendPack  = new RTPPacket();
	recvRTCPack= new RTCPPacket();
	sendRTCPack  = new RTCPPacket();
	sRep = new rtcpSReport();
	rRep = new rtcpRReport();
	sRepRecv = new rtcpSReport();
	rRepRecv = new rtcpRReport();
}

RTPManager::~RTPManager() {
	delete formatter;
	delete state;
	delete recvPack;
	delete sendPack;
	delete recvRTCPack;
	delete sendRTCPack;
	delete sRep;
	delete rRep;
	delete sRepRecv;
	delete rRepRecv;
}

int RTPManager::init(int type, char* fname) {
	state->sequence=0;

	state->SSRC = rand();
	//printf("\n SSRC = %ld",state->SSRC);
	sendPack->SSRC=state->SSRC;		//allocate random SSRC
	sendRTCPack->SSRC=state->SSRC;	//allocate random SSRC
	rRep->SSRC = state->SSRC;
	sendPack->payloadType=type;

	//init RTP timestamp to random value:
	state->rtpTimestampSend = rand();
	state->rtpTimestampRecv=0;
	state->recvSeqNext	=0;
	state->recvSeq=0;
	state->isFirstPack=true;

	state->rtpTimestampSend=rand();		//allocate random timestamp
	sendPack->timestamp = state->rtpTimestampSend;

    fp = NULL;
	if(fname) {
		char tname[30];
		sprintf(tname,"%s%lu.csv", fname, state->SSRC);
		fp = fopen(tname,"w");
		if(fp == NULL) {
			perror("Unable to open file!");
			return -1;
		}
		fprintf(fp,"SSRC,nPackets,Lost,Percent Loss,Seq Recvd,Jitter,");
		fprintf(fp,"LSR,DLSR,RTT(ms)\r\n");
	}
	return 0;
}

int RTPManager::close(void) {
	if(fp)
		fclose(fp);
    fp = NULL;
	return 0;
}


bool RTPManager::isTimeForSR(void) {
	gettimeofday(&currtime_w,NULL);
	return ((currtime_w.tv_sec - state->tSRLastSent) >= state->SRPeriodSec);
}


bool RTPManager::isPeerAlive(void) {
	gettimeofday(&currtime_w,NULL);
	return ((currtime_w.tv_sec - state->tSRLastSent)
		 >= state->SRPeriodSec*5);
}

int RTPManager::handleRTPSend(char* bytes,
						int len,
						char* buff,
						int buffSize) {
	int ret;
	rtpAudioPayload *payload;
	payload = (rtpAudioPayload *)bytes;
	payload->first = htons(payload->first);
	sendPack->payload=bytes;
	sendPack->length=len;
	sendPack->timestamp=state->rtpTimestampSend;
	sendPack->sequence=state->sequence;
	ret = formatter->writeBytes(sendPack,
								buff,
								buffSize);
	if(ret<0)
		return -1;
	state->nPacketsSent++;
	state->nBytesSent+=84;
	state->rtpTimestampSend+=161;
	state->sequence++;
	if(state->sequence <=0) {
		state->seqWrap++;
		state->sequence=0;
	}
	return ret;
}

int RTPManager::handleRTPRecv(char* buff,
						int buffSize,
						char* bytes,
						int len) {
	int ret;
	rtpAudioPayload *payload;

	gettimeofday(&currtime_r,NULL);
	recvPack->pointData(bytes, len);
	//parse bytes into RTP Packet
	ret = formatter->parseBytes(recvPack,buff,buffSize);

	if(state->isFirstPack) {
		state->remoteSSRC = recvPack->SSRC;
		state->isFirstPack=false;
	}

	payload = (rtpAudioPayload*) bytes;
	payload->first = ntohs(payload->first);

	if(recvPack->sequence != state->recvSeqNext) {
		if(state->nPacketsRecvd > 0) {
			//packet loss detected
			state->nPacketsLost+=(recvPack->sequence
								- state->recvSeqNext);
			state->recvSeq = recvPack->sequence;
		}
	}

	state->recvSeq = recvPack->sequence;
	state->recvSeqNext = state->recvSeq + 1;
	if(state->recvSeqNext<=0)	{
		//rollover
		state->recvSeqWrap++;
		state->recvSeqNext = 0;
	}
	state->nPacketsRecvd++;
	state->nBytesRecvd+=84;

	state->Ri = state->Rj;
	state->Rj = (double)((double)currtime_r.tv_sec*1000000
		+ (double)currtime_r.tv_usec);

	state->Si = state->Sj;
	state->Sj = recvPack->timestamp;
	state->Dij = (state->Rj - state->Ri)
				  - (double)(state->Sj - state->Si);
	state->jitter = state->jitter_prev
				+ ((mod(state->Dij)- state->jitter_prev)/16);
	state->jitter_prev = state->jitter;
	return ret;
}

int RTPManager::handleRTCPRecv(char* buff,
						int buffSize) {
	int ret=0;
	gettimeofday(&currtime_r,NULL);
	if(buffSize >0) {
		formatter->parseBytes(recvRTCPack,buff,buffSize);

		switch(recvRTCPack->getReportType()) {
		case RTCP_REP_TYPE_SR: 			//SR
			if(recvRTCPack->getSReport(sRepRecv)) {
				state->nSRep++;

				state->tLSRSec = currtime_r.tv_sec;
				state->LSR = getRTPTime(sRepRecv->ntpTimestamp1,
										sRepRecv->ntpTimestamp2);

				state->tLSRRecvd=getRTPTime(currtime_r.tv_sec,
										currtime_r.tv_usec);
				ret = 1;
			}
			else {
				ret=-1;
			}
			break;

		case RTCP_REP_TYPE_RR:			//RR
			state->nRRep++;
			if(recvRTCPack->nextRReport(rRepRecv)) {
				state->RTTPropDelay
					= (getRTPTime(currtime_r.tv_sec,
								  currtime_r.tv_usec)
						- rRepRecv->LSR
						- rRepRecv->DLSR);
				state->total = (rRepRecv->extSeqCycles*65536)
									+ rRepRecv->extSeqRecvd;
				if(state->total==0)
					state->total++;
				if(fp) {
					fprintf(fp,"%lu,%lu,%u,%.2f,%u,%lu,%lu,%lu,%lu\r\n",
							recvRTCPack->SSRC,
							state->total,
							rRepRecv->nLostPackets,
							((float)rRepRecv->nLostPackets*100/
								(float)state->total),
							rRepRecv->extSeqRecvd,
							rRepRecv->jitter,
							rRepRecv->LSR,
							rRepRecv->DLSR,
							(unsigned long)((double)state->RTTPropDelay*65.536));
					fflush(fp);
				}
			}
			else {
				ret = -1;
			}
			break;

		default:
			ret = -1;
			break;
		}
	}
	return ret;
}


int RTPManager::handleRTCPSendRR(char* buff, int buffSize) {
	int ret;

	gettimeofday(&currtime_w,NULL);
	sendRTCPack->reset();
	rRep->SSRC = state->remoteSSRC;
	rRep->fractionLost = 0;
	rRep->nLostPackets = state->nPacketsLost;
	rRep->extSeqCycles 	= state->recvSeqWrap;
	rRep->extSeqRecvd 	= state->recvSeq;
	rRep->jitter = (unsigned long)state->jitter;
	rRep->DLSR = getRTPTime(currtime_w.tv_sec,
							currtime_w.tv_usec)
							- state->tLSRRecvd;
	rRep->LSR = state->LSR;
	sendRTCPack->addRReport(rRep);
	ret = formatter->writeBytes(sendRTCPack,
								buff,
								buffSize);
	if(ret>=0)
		state->tRRLastSent = currtime_w.tv_sec;
	return ret;
}

int RTPManager::handleRTCPSendSR(char* buff, int buffSize) {
	int ret;
	if(state->nPacketsSent == state->lastPacketCount) {
		//if no data sent in this interval, don't send SR
		return -1;
	}
	state->lastPacketCount = state->nPacketsSent;

	sRep->ntpTimestamp1 = currtime_w.tv_sec;
	sRep->ntpTimestamp2 = currtime_w.tv_usec;
	sRep->rtpTimestamp  = state->rtpTimestampSend;
	sRep->nSentPackets 	= state->nPacketsSent;
	sRep->nSentBytes 	= state->nBytesSent;
	sendRTCPack->reset();
	sendRTCPack->setSReport(sRep);

	ret = formatter->writeBytes(sendRTCPack,
								buff,
								buffSize);
	if(ret>=0)
		state->tSRLastSent = currtime_w.tv_sec;
	return ret;
}
