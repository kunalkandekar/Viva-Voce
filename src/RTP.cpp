#include "RTP.h"

/******************** PACKETS ******************************/

/** RTP Packets **/
RTPPacket::RTPPacket(void) {
	version=2;
	padding=0;
	extension=0;
	CSRCCount=0;
	marker=0;
	payloadType=0;
	sequence=0;
	timestamp=0;
	SSRC=0;

	payload=NULL;
	length=0;
}

RTPPacket::~RTPPacket(void) {

}

int 	RTPPacket::countCSRC(void) {
	return CSRCCount;
}

int 	RTPPacket::pointData(char *data, int len) {
	this->payload = data;
	this->length = len;
	return 0;
}

/** RTCP Packets **/
RTCPPacket::RTCPPacket() {
	sendRep=new rtcpSReport();
	reset();
}

void RTCPPacket::reset(void) {
	reportCount=0;
	rtcpType=-1;		//Invalid
	isSRepValid=false;
	version=2;
	padding=0;
	length = RTCP_FIX_HDR_LEN;
	recvRepsVec.clear();
}

RTCPPacket::~RTCPPacket() {
	delete sendRep;
	clearRReports();
}

int	RTCPPacket::getReportType(void) {
	return rtcpType;
}

void RTCPPacket::setSReport(rtcpSReport *report) {
	if(report) {
		rtcpType=200;
		memcpy(sendRep,report,sizeof(rtcpSReport));
		isSRepValid=true;
		length+=sizeof(rtcpSReport);
	}
}

int RTCPPacket::getSReport(rtcpSReport *report) {
	if(isSRepValid) {
		memcpy(report,sendRep,sizeof(rtcpSReport));
		isSRepValid=false;
		return 1;
	}
	return 0;
}

void RTCPPacket::addRReport(rtcpRReport *report) {
	if(report) {
		if(rtcpType!=200) {
			rtcpType=201;
		}
		reportCount++;
		recvRep = new rtcpRReport();
		memcpy(recvRep,report,sizeof(rtcpRReport));
		recvRepsVec.push_back(recvRep);
		length+=sizeof(rtcpRReport);
	}
}

void RTCPPacket::clearRReports(void) {
	length-=(recvRepsVec.size()*sizeof(rtcpRReport));
	reportCount=0;
	while(!recvRepsVec.empty()) {
		recvRep = recvRepsVec.back();
		recvRepsVec.pop_back();
		delete recvRep;
	}
	recvRepsVec.clear();
}

int RTCPPacket::countRReports(void) {
	return recvRepsVec.size();
}

int RTCPPacket::nextRReport(rtcpRReport* report) {
	if(!recvRepsVec.empty()) {
		recvRep = recvRepsVec.back();
		memcpy(report,recvRep,sizeof(rtcpRReport));
		delete recvRep;
		recvRep = NULL;
		recvRepsVec.pop_back();
		return 1;
	}
	return 0;
}

int RTCPPacket::getLength(void) {
	return length;
}

/**************** FORMATTER  ******************************/


RTPPacketFormatter::RTPPacketFormatter(void) {
	len 	= RTP_FORMAT_DEFAULT_LEN;
	offset	= RTP_FIX_HDR_LEN;
	buffer  = (char*)malloc(len);
	rtpHdr 	= (rtpHeader*)buffer;
	rtcpHdr = (rtcpHeader*)buffer;
	data	= buffer+RTP_FIX_HDR_LEN;
	memset(buffer,0,len);
	sRep = new rtcpSReport();
	rRep = new rtcpRReport();
}

RTPPacketFormatter::~RTPPacketFormatter(void) {
	free(buffer);
	delete sRep;
	delete rRep;
}

int RTPPacketFormatter::writeBytes(RTPPacket *pack, char *msg, int len) {
	offset=0;
	rtpHdr->version     = pack->version;
	rtpHdr->padding 	   = (pack->padding?1:0);
	rtpHdr->extension   = (pack->extension?1:0);
	rtpHdr->CSRCCount   = pack->countCSRC();
	rtpHdr->marker      = (pack->marker?1:0);
	rtpHdr->payloadType = pack->payloadType;
	rtpHdr->sequence    = htons(pack->sequence);
	rtpHdr->timestamp   = htonl(pack->timestamp);
	rtpHdr->SSRC 	    = htonl(pack->SSRC);

	offset+=RTP_FIX_HDR_LEN;
	if(pack->countCSRC()) {
		//add CRSCs
	}

	memcpy(msg,buffer,RTP_FIX_HDR_LEN);

	if(len>pack->length)
		len = pack->length;

	memcpy(msg+offset,pack->payload,len);
	return len+RTP_FIX_HDR_LEN;
}

int RTPPacketFormatter::parseBytes(RTPPacket *pack, char *msg, int len) {
	offset=0;
	memcpy(buffer,msg,RTCP_FIX_HDR_LEN);
	pack->version 	= rtpHdr->version;
	pack->padding 	= rtpHdr->padding;
	pack->extension = rtpHdr->extension;
	temp 			= rtpHdr->CSRCCount;
	pack->marker    = rtpHdr->payloadType;
	pack->sequence  = ntohs(rtpHdr->sequence);
	pack->timestamp = ntohl(rtpHdr->timestamp);
	pack->SSRC 	 	= ntohl(pack->SSRC);

	offset=RTP_FIX_HDR_LEN;
	if(temp) {
		//extract CRSCs
	}

	memcpy(pack->payload,msg+offset, len-RTP_FIX_HDR_LEN);
	return len-RTP_FIX_HDR_LEN;
}


int RTPPacketFormatter::writeBytes(RTCPPacket *pack, char *msg, int len) {
	offset=0;
	rtcpHdr->version     = pack->version;
	rtcpHdr->padding 	 = (pack->padding?1:0);
	rtcpHdr->reportCount = pack->countRReports();
	rtcpHdr->rtcpType 	 = pack->getReportType();

	rtcpHdr->SSRC 	 	 = htonl(pack->SSRC);
	//write header
	rtcpHdr->length	 	 = htons(pack->getLength()-1);
	memcpy(msg,buffer,RTCP_FIX_HDR_LEN);

	offset+=RTCP_FIX_HDR_LEN;

	//write  sender report
	if(pack->getSReport(sRep)) {
		sRep->ntpTimestamp1	= htonl(sRep->ntpTimestamp1);
		sRep->ntpTimestamp2	= htonl(sRep->ntpTimestamp2);
		sRep->rtpTimestamp	= htonl(sRep->rtpTimestamp);
		sRep->nSentPackets	= htonl(sRep->nSentPackets);
		sRep->nSentBytes	= htonl(sRep->nSentBytes);
		memcpy(msg+offset,sRep,sizeof(rtcpSReport));
		offset+=sizeof(rtcpSReport);
	}

	//write receiver report
	while(pack->countRReports()) {
		if(pack->nextRReport(rRep)) {
			rRep->SSRC 			= htonl(rRep->SSRC);
			rRep->extSeqRecvd	= htons(rRep->extSeqRecvd);
			rRep->extSeqCycles	= htons(rRep->extSeqCycles);
			rRep->jitter	 	= htonl(rRep->jitter);
			rRep->LSR		 	= htonl(rRep->LSR);
			rRep->DLSR		 	= htonl(rRep->DLSR);
			memcpy(msg+offset,rRep,sizeof(rtcpRReport));
			offset+=sizeof(rtcpRReport);
		}
	}
	pack->reset();
	return offset;
}

int RTPPacketFormatter::parseBytes(RTCPPacket *pack, char *msg, int len) {
	offset=0;
	pack->reset();
	memcpy(buffer,msg,RTCP_FIX_HDR_LEN);
	pack->version 	= rtcpHdr->version;
	pack->padding 	= rtcpHdr->padding;
	pack->SSRC 	 	= ntohl(rtcpHdr->SSRC);
	rtcpLen			= ntohs(rtcpHdr->length);
	repCount 		= rtcpHdr->reportCount;
	temp 			= rtcpHdr->rtcpType;

	offset+=RTCP_FIX_HDR_LEN;
	if(temp==200) {
		//sRep = new rtcpSReport();
		memcpy(sRep,msg+offset,sizeof(rtcpSReport));
		sRep->ntpTimestamp1	= ntohl(sRep->ntpTimestamp1);
		sRep->ntpTimestamp2	= ntohl(sRep->ntpTimestamp2);
		sRep->rtpTimestamp	= ntohl(sRep->rtpTimestamp);
		sRep->nSentPackets	= ntohl(sRep->nSentPackets);
		sRep->nSentBytes	= ntohl(sRep->nSentBytes);
		offset+=sizeof(rtcpSReport);
		pack->setSReport(sRep);
	}
	else if (temp!=201) {
		return -1;			//unsupported type
	}

	for(temp=0; temp < repCount; temp++) {
		//rRep = new rtcpRReport();
		memcpy(rRep,msg+offset,sizeof(rtcpRReport));
		rRep->SSRC 			= ntohl(rRep->SSRC);
		rRep->extSeqRecvd	= ntohs(rRep->extSeqRecvd);
		rRep->extSeqCycles	= ntohs(rRep->extSeqCycles);
		rRep->jitter	 	= ntohl(rRep->jitter);
		rRep->LSR		 	= ntohl(rRep->LSR);
		rRep->DLSR		 	= ntohl(rRep->DLSR);
		offset+=sizeof(rtcpRReport);
		pack->addRReport(rRep);
	}
	return offset;
}