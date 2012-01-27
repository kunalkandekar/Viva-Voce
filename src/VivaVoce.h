#ifndef __VIVAVOCE_H
#define __VIVAVOCE_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <vector>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

//sockets
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//infra-structure
#include "Common.h"
#include "kodek.h"
#include "ulaw.h"
#include "RTP.h"
#include "SIP.h"
#include "SDP.h"
#include "UserAgent.h"
#include "AudioRW.h"
#include "UdpRW.h"
#include "AdpcmCodec.h"

class VivaVoce {
private:
	Common* common;

	AudioReader	*audioReader;
	AudioWriter	*audioWriter;
	UdpReader	*udpReader;
	UdpWriter	*udpWriter;
	AdpcmCodec	*adpcmCodec;
	UserAgent	*userAgent;
	
	int startThreads(void);
	int waitForThreadsToDie(void);

public:
	VivaVoce(char* username);
	~VivaVoce();

	int run(void);
	
	char *username;
    int  sipPort;
    int  rtpPort;
    int  rtcpPort;
    bool stdPorts;
};

//utility functions
int initAudio(Common* common);

void usage(void);

#endif
