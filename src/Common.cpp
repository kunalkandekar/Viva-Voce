#include "Common.h"
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

Common::Common() {
	username = NULL;
	hostname = NULL;
	hostnamelen = 0;
	
	isRinging = 0;
	verbose = false;

	evtConn = new MyEvent();
	sendQ  	= new SyncQueue();
	codecQ 	= new SyncQueue();
	playQ	= new SyncQueue();
	signalQ = new SyncQueue();

	ipBufSize	= 1500;
	ipBuf		= (char*)malloc(ipBufSize);
	opBufSize	= 1500;	
	opBuf		= (char*)malloc(opBufSize);

	stateEnc = ADPCM_STATE_INIT;
    stateDec = ADPCM_STATE_INIT;

	precision = 8;
	nChannels = 1;
	nCoding = (char*)"ulaw";
	sampleRate = 8000;
	gain = 1;
	bytesRec = 0;
	bytesPld = 0;
	bytesEnc = 0;
	bytesDec = 0;
	bytesRcv = 0;
	bytesSnt = 0;
	unitsRec = 0;
	unitsPld = 0;
	unitsEnc = 0;
	unitsDec = 0;
	unitsRcv = 0;
	unitsSnt = 0;

	samplesPerCapture = 161;
	sampleByteSize    = precision / 8;
	bytesPerCapture   = samplesPerCapture * sampleByteSize;
	memoSize	      = samplesPerCapture*2+10;
	bytesPerFragment  = (samplesPerCapture-1)/2 + 2;
	msecPerCapture    = (samplesPerCapture*1000)/gain;
	msecLastBurst     = 0;

	rmg = new ResourceManager(20, memoSize);
	sipRmg = new ResourceManager(20, VIVOCE_SIP_MEMO_SIZE);

	isConnected = 0;
	go = 1;
	rmg->go = &(this->go);

	sipSocket = 0;
	rtpSocket = 0;
	rtcpSocket = 0;

	sipPort = VIVOCE_SIP_PORT;
	rtpPort = VIVOCE_RTP_PORT;
	rtcpPort = VIVOCE_RTCP_PORT;

	rtpMgr = new RTPManager();

	sendRR=false;
	sendSR=false;

	simLost = 0;
	packLossProb=5;

	port = 0;
	remoteName = NULL;
	audio_ip = 0;
	audio_op = 0;
	ai = NULL;
	ao = NULL;
	audio_ctl  = 0;
	audio_port = 0;

	audrSig = 0;
	udprSig = 0;
	audioGo = 0;
	audioMute = 0;
	audioGenTone = 0;
	codecStop = 0;
}

Common::~Common() {
	delete evtConn;
	free(ipBuf);
	free(opBuf);

	emptyQueue(sendQ,rmg);
	emptyQueue(codecQ,rmg);
	emptyQueue(playQ,rmg);
	emptyQueue(signalQ,sipRmg);

	delete sendQ;
	delete codecQ;
	delete playQ;
	delete signalQ;

	delete rmg;
	delete sipRmg;

	delete rtpMgr;
	
	if(username) free(username);
	if(hostname) free(hostname);
}

void Common::setUserName(char *uname) {
    if(username)
        free(username);
	int l = strlen(uname);
	username = (char*)malloc(l + 1);
	strcpy(username, uname);
}

void Common::setHostName(char *hname) {
    if(hostname)
        free(hostname);
	hostnamelen = strlen(hname);
	hostname = (char*)malloc(hostnamelen + 1);
	strcpy(hostname, hname);
}

void Common::reset(void) {
	precision = 8;
	nChannels = 1;
	nCoding = (char*)"ulaw";
	sampleRate = 8000;
	gain = 1;

	samplesPerCapture = 161;
	sampleByteSize    = precision / 8;
	bytesPerCapture   = samplesPerCapture * sampleByteSize;	
	memoSize	= samplesPerCapture*2+10;
	bytesPerFragment = (samplesPerCapture-1)/2 + 2;
	msecPerCapture = (samplesPerCapture*1000)/gain;
	msecLastBurst = 0;
	isConnected = 0;
	go=1;
	rmg->go = &(this->go);

	bytesRec = 0;
	bytesPld = 0;
	bytesEnc = 0;
	bytesDec = 0;
	bytesRcv = 0;
	bytesSnt = 0;
	unitsRec = 0;
	unitsPld = 0;
	unitsEnc = 0;
	unitsDec = 0;
	unitsRcv = 0;
	unitsSnt = 0;

	sendRR=false;
	sendSR=false;
	simLost = 0;
	packLossProb=5;
	rtpMgr->state->reset();
	audioGo   = 0;
	codecStop = 0;
}

int setBlocking(int fd, int ON) {
	int help;
	int flags;

	if ( (flags = fcntl(fd, F_GETFL, 0)) == -1 ) {
		fprintf(stderr, "Error in SetBlock: F_GETFL...\n");
		return -1;
	}

	if (ON)
		help = flags & ~O_NDELAY;		/* Reset O_NDELAY in flags (blocking) */
	else
		help = flags | O_NDELAY;		/* Set O_NDELAY in flags (nonblocking)*/

	if (fcntl(fd, F_SETFL, help) == -1 ) {
		fprintf(stderr, "Error in SetBlock: F_SETFL...\n");
		return -1;
	}
	return 0;
}

int emptyQueue(SyncQueue* queue, ResourceManager* rmgr) {
	Memo* memo;
	int transferred = 0;
	while(queue->count()) {
		memo = (Memo*)queue->deQ();
		rmgr->releaseMemo(memo);
		transferred++;
	}
	return transferred;
}
