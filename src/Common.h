#ifndef __COMMON_H
#define __COMMON_H

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
#include "ResourceManager.h"
#include "RTPManager.h"
#include "Thread.h"
#include "SyncQueue.h"
#include "kodek.h"

//global constants

#define  VIVOCE_ERR				-20
#define  VIVOCE_MSG_ENC			20
#define  VIVOCE_MSG_DEC			21
#define  VIVOCE_MSG_PLAY		22
#define  VIVOCE_MSG_REC			23
#define  VIVOCE_MSG_SEND		24
#define  VIVOCE_MSG_REPORT		25
#define  VIVOCE_RESPOND			99

#define  VIVOCE_MSG_MAX			1264
#define  VIVOCE_MSG_HDR			16
#define  VIVOCE_MSG_AUDIO		10

#define  VIVOCE_SIP_INVITE		31
#define  VIVOCE_SIP_ACCEPT		32
#define  VIVOCE_SIP_REJECT		33
#define  VIVOCE_SIP_BYE			34
#define  VIVOCE_SIP_SEND		35
#define  VIVOCE_SIP_RCVD		36

#define  VIVOCE_WAIT_MSEC		100

#define  VIVOCE_RTP_PORT		5004
#define  VIVOCE_RTCP_PORT		5005
#define  VIVOCE_SIP_PORT		5060

#define	 VIVOCE_SIP_MEMO_SIZE	1024

#define  NEWLINE				10

#define  PACKET_LOSS_PROB		5

#define  VIVAVOCE_DEBUG         1

//Utility functions
int setBlocking(int fd, int ON);

int emptyQueue(SyncQueue* queue, ResourceManager* rmg);

class Common {
public:
	Common();
	~Common();
	void reset(void);
    void setUserName(char *uname);
    void setHostName(char *hname);

	char 		*username;
	char 		*hostname;
	int			hostnamelen;
	bool		verbose;
	/*net*/
	int 		sipSocket;
	int 		rtpSocket;
	int 		rtcpSocket;

	int 		sipPort;
	int 		rtpPort;
	int 		rtcpPort;

	int 		port;
	char		*ipBuf;
	int 		ipBufSize;
	char		*opBuf;
	int 		opBufSize;

	struct sockaddr_in sipAddr;
	struct sockaddr_in remoteSIPAddr;
	struct sockaddr_in remoteTempAddr;

	struct sockaddr_in rtpAddr;
	struct sockaddr_in rtcpAddr;
	struct sockaddr_in remoteRTPAddr;
	struct sockaddr_in remoteRTCPAddr;
	struct sockaddr_in recvAddr;
	struct hostent 		*hp;

	int 	isConnected;
	int		isRinging;
	char	*remoteName;
	double  bytesRcv;
	double  bytesSnt;
	double  unitsRcv;
	double  unitsSnt;

	/*audio*/
	int 	audio_ip;
	int 	audio_op;
	void        *ai;
	void        *ao;
	uint32_t 	audio_ctl;
	uint32_t 	audio_port;

	/*audio settings*/
	int		precision;
	int		nChannels;
	char* 	nCoding;
	long	sampleRate;
	int		gain;
	double  bytesRec;
	double  bytesPld;
	double  unitsRec;
	double  unitsPld;

	/*codec*/
	adpcm_state_t stateEnc;
	adpcm_state_t stateDec;
	double  bytesEnc;
	double  bytesDec;
	double  unitsEnc;
	double  unitsDec;

	/*IPC*/
	int memoSize;
	MyEvent*	evtConn;
	SyncQueue* 	sendQ;
	SyncQueue* 	playQ;
	SyncQueue* 	codecQ;
	SyncQueue* 	signalQ;

	/* resource */
	int		bytesPerFragment;
	int 	samplesPerCapture;
	int     sampleByteSize;
	int     bytesPerCapture;
	int		audrSig;
	int		udprSig;
	ResourceManager* rmg;
	ResourceManager* sipRmg;

	RTPManager		*rtpMgr;

	bool			sendRR;
	bool			sendSR;

	unsigned long 	msecPerCapture;
	unsigned long	msecLastBurst;
	unsigned int 	simLost;
	int				packLossProb;

	int				go;
	int				audioGo;
	int				audioMute;
	int				audioGenTone;	
	int             codecStop;
	struct timeval 	currtime;
};

#endif
