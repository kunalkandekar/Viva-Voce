#ifndef __RTP_H
#define __RTP_H

#include <netinet/in.h>

#include <stdlib.h>
#include <string.h>
#include <vector>

#define RTP_FORMAT_DEFAULT_LEN 		1500
#define	RTP_FIX_HDR_LEN				12
#define	RTCP_FIX_HDR_LEN			8
#define RTP_AUDIO_PAYLOAD_NBYTES	80
#define RTCP_REP_TYPE_SR			200
#define RTCP_REP_TYPE_RR			201

#define RTP_PAYLOAD_TYPE_ADPCM		5

#define getRTPTime(X,Y) ((X<<16 & 0xFFFF0000)|(Y>>16 & 0x0000FFFF))


typedef std::vector<unsigned long> ulong_vec;

#pragma pack(1)
typedef struct _rtpAudioPayload {
	short 	first;
	char	stepsizeIndex;
	char	reserved;
	char 	data[RTP_AUDIO_PAYLOAD_NBYTES];
} rtpAudioPayload;

/* Uses bitfields for ease... should change this to use bit-manipulation for portability */

typedef struct _rtpFixedHeader {
	//Bit Fields
	unsigned 		version 	: 2;
	unsigned 		padding 	: 1;
	unsigned 		extension 	: 1;
	unsigned 		CSRCCount 	: 4;
	unsigned 		marker 		: 1;
	unsigned 		payloadType	: 7;	//} 16 bits
	//End of BitFields

	unsigned short 	sequence;			//16 bits
	unsigned long	timestamp;			//32 bits
	unsigned long	SSRC;				//32 bits
} rtpHeader;

typedef struct _rtcpFixedHeader {
	//Bit Fields
	unsigned 		version 	: 2;
	unsigned 		padding 	: 1;
	unsigned 		reportCount	: 5;
	unsigned 		rtcpType	: 8;	//16 bits
	unsigned short 	length;				//16 bits
	unsigned long	SSRC;				//32 bits
} rtcpHeader;

typedef struct _rtcpSReport {
	unsigned long 	ntpTimestamp1;
	unsigned long 	ntpTimestamp2;	//2*32 bits
	unsigned long	rtpTimestamp;	//32
	unsigned long 	nSentPackets;	//32
	unsigned long 	nSentBytes;	//32
} rtcpSReport;

typedef struct _rtcpRReport{
	unsigned long	SSRC;				//32 bits
	unsigned 		fractionLost : 8;
	unsigned 		nLostPackets : 24;	//} 32
	unsigned 		extSeqCycles : 16;
	unsigned 		extSeqRecvd  : 16;	//} 32
	unsigned long 	jitter;				//32
	unsigned long 	LSR;				//32
	unsigned long 	DLSR;				//32
} rtcpRReport;

#pragma pack(4)

class RTCPPacket {
private:
	int 	reportCount;
	int		rtcpType;
	int		length;
	bool    isSRepValid;
	rtcpSReport *sendRep;
	rtcpRReport	*recvRep;

	std::vector <rtcpRReport*> recvRepsVec;

public:

	RTCPPacket();
	~RTCPPacket();
	unsigned long	SSRC;

	int 	version;
	int		padding;

	int		getLength(void);
	void 	setSReport(rtcpSReport *report);
	int		getSReport(rtcpSReport *report);
	void 	addRReport(rtcpRReport *report);
	int 	countRReports(void);
	void 	clearRReports(void);
	int 	nextRReport(rtcpRReport *report);

	int		getReportType(void);
	void 	reset(void);
};

class RTPPacket {
private:
	ulong_vec 	CSRCList;
	int 		CSRCCount;
public:
	RTPPacket();
	~RTPPacket();
/*
	void 	addCSRC(long csrc);
	long 	getCSRCCount(void);
	long 	getCSRC(int index);

	int 	setData(char *data, int len);
	int 	getData(char *buf);
*/
	int 	pointData(char *data, int len);
	int 	countCSRC(void);

	int 			version;
	int				padding;
	int 			extension;
	int 			marker;
	int				payloadType;
	unsigned short 	sequence;
	unsigned long	timestamp;
	unsigned long	SSRC;

	char 	*payload;
	int 	length;
};

class RTPPacketFormatter {
private:
	int 		len;
	int			offset;
	int 		temp;
	int			rtcpLen;
	int			repCount;
	char		*buffer;
	char		*data;
	rtpHeader 	*rtpHdr;
	rtcpHeader 	*rtcpHdr;

	rtcpSReport *sRep;
	rtcpRReport *rRep;

public:
	RTPPacketFormatter(void);
	~RTPPacketFormatter(void);

	int writeBytes(RTPPacket *pack, char *msg, int len);

	int parseBytes(RTPPacket *pack, char *msg, int len);

	int	writeBytes(RTCPPacket *pack, char *msg, int len);

	int	parseBytes(RTCPPacket *pack, char *msg, int len);
};

#endif