#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "VivaVoce.h"
#include "AudioTest.h"

VivaVoce::VivaVoce(char* name) {
    username = name;
    sipPort = 0;
    rtpPort = 0;
    rtcpPort = 0;
    stdPorts = true;

    //alloc stuff
	common = new Common();
	common->setUserName(username);

	audioReader = new AudioReader(common);
	audioWriter = new AudioWriter(common);
	udpReader   = new UdpReader(common);
	udpWriter   = new UdpWriter(common);
	adpcmCodec  = new AdpcmCodec(common);
	userAgent   = new UserAgent(common);
	userAgent->setThreadsToSignal(audioReader);
}

VivaVoce::~VivaVoce() {
	//free stuff
	common->go=0;

	close(common->rtpSocket);
	close(common->rtcpSocket);

	delete audioReader;
	delete audioWriter;
	delete udpReader;
	delete udpWriter;
	delete adpcmCodec;
	delete userAgent;
	delete common;
}

int VivaVoce::startThreads(void) {
	//start threads
	udpReader->start();
	udpWriter->start();
	adpcmCodec->start();
	audioReader->start();
	audioWriter->start();
	userAgent->start();
	return 0;
}

int VivaVoce::waitForThreadsToDie(void) {
	//start threads
	udpReader->join(NULL);
	udpWriter->join(NULL);
	adpcmCodec->join(NULL);
	audioReader->join(NULL);
	audioWriter->join(NULL);
	userAgent->join(NULL);
	return 0;
}


int VivaVoce::run(void) {
	Memo *memo;

	common->rmg->init();
	common->sipRmg->init();

	common->go=1;

	//randomize
	gettimeofday(&common->currtime,NULL);
	srand(common->currtime.tv_usec);

	if(!stdPorts) {
		common->sipPort=sipPort;
		common->rtpPort=rtpPort;
		common->rtcpPort=rtcpPort;
	}
	if(UdpBase::initTransport(common) <0) {
		return -1;
	}
	if(initAudio(common) <0) {
		printf("Error in opening audio\n");
		return -1;
	}

    startThreads();

	common->isConnected = 0;
	
	AudioIOTest *audioTest = NULL;

	char* ptr;
	char* line;
	printf("\nVIVAVOCE 1.2:\nWelcome %s@%s",
		common->username,
		common->hostname);
	while(1) {
	    char cmd[100];	   
	    //cmd[0] = '\0';
		printf("\n cmd: ");	
        fgets(cmd, sizeof(cmd), stdin);   //gets(cmd) is deprecated so using fgets instead.
        cmd[strlen(cmd) - 1] = '\0';      //remove newline

		if((strncmp("i", cmd, 1)==0)  || (strncmp("invite", cmd, 6)==0)) {
			if(common->isRinging || common->isConnected) {
				printf("\n\tError: Busy with another call.");
				continue;
			}
			line = strtok_r(cmd, " ", &ptr);
			line = strtok_r(NULL, " ", &ptr);

			if(line) {
				printf("Connecting to %s...",line);
				memo = common->sipRmg->allocMemo();
				memo->code=VIVOCE_SIP_INVITE;
				sprintf(memo->bytes,"%s",line);
				common->signalQ->signalData(memo);
			}
			else {
				printf("\n\tError: Invalid remote address.");
				sleep(1);
				continue;
			}
		}
		else if((strncmp("a", cmd, 1)==0) || (strncmp("accept", cmd, 6)==0)) {
			if(!common->isRinging) {
				printf("\n\tError: No incoming request to accept.");
				continue;
			}
			memo = common->sipRmg->allocMemo();
			memo->code = VIVOCE_SIP_ACCEPT;
			common->signalQ->signalData(memo);
		}
		else if((strncmp("r", cmd, 1)==0) || (strncmp("reject", cmd, 6)==0)) {
			if(!common->isRinging) {
				printf("\n\tError: No incoming request to reject.");
				continue;
			}
			memo = common->sipRmg->allocMemo();
			memo->code = VIVOCE_SIP_REJECT;
			common->signalQ->signalData(memo);
		}
		else if((strncmp("b", cmd, 1)==0) || (strncmp("bye", cmd, 3)==0)) {
			if(!common->isConnected) {
				printf("\n\tError: Not connected");
				continue;
			}
			memo = common->sipRmg->allocMemo();
			memo->code = VIVOCE_SIP_BYE;
			common->signalQ->signalData(memo);
		}
/*		else if(strcmp(cmd,"loss")==0) {
			printf("\n\tEnter packet loss probability (percent):");
			scanf("%d",&common->packLossProb);
			if((common->packLossProb<1)||(common->packLossProb>100)) {
				common->packLossProb = 20;
			}
			printf("\n\tPacket loss probability set to %d percent",
				common->packLossProb);
		}*/
		else if((strncmp("d", cmd, 1)==0) || (strncmp("diag", cmd, 4)==0)) {
			printf("\nDiagnostics for SSRC=%ld: %s%s",
				common->rtpMgr->state->SSRC,
				(common->isConnected?"Connected to ":"Disconnected"),
				(common->isConnected?common->remoteName:""));
			printf("\n\tRec=%.0f enc=%.0f snt=%.0f rcv=%.0f dec=%.0f pld=%.0f",
				common->unitsRec,
				common->unitsEnc,
				common->unitsSnt,
				common->unitsRcv,
				common->unitsDec,
				common->unitsPld);
			printf("\n\tSeq=%d cycles=%d snt=%ld rcv=%ld lost=%ld simLost=%d",
				common->rtpMgr->state->sequence,
				common->rtpMgr->state->seqWrap,
				common->rtpMgr->state->nPacketsSent,
				common->rtpMgr->state->nPacketsRecvd,
				common->rtpMgr->state->nPacketsLost,
				common->simLost);
			printf("\n\tSR rcvd=%u, RR rcvd=%u Jitter=%.3f RTT=%ld ms" ,
				common->rtpMgr->state->nSRep,
				common->rtpMgr->state->nRRep,
				common->rtpMgr->state->jitter,
				(common->rtpMgr->state->RTTPropDelay*65536/1000));
			printf("\n\tQ:codec=%d play=%d send=%d signal=%d",
				common->codecQ->count(),
				common->playQ->count(),
				common->sendQ->count(),
				common->signalQ->count());
			printf("\n\trmg:total=%d res=%d Sip:total=%d res=%d",
				common->rmg->getTotal(),
				common->rmg->getNMemosInReserve(),
				common->sipRmg->getTotal(),
				common->sipRmg->getNMemosInReserve());
		}
		else if((strncmp("v", cmd, 1)==0) || (strncmp("verbose", cmd, 7)==0)) {
			if(common->verbose) {
				common->verbose = false;
				printf("\n\tVerbose mode disabled");
			}
			else {
				common->verbose = true;
				printf("\n\tVerbose mode enabled");
			}
		}
		else if(strncmp("w", cmd, 1)==0) {
			memo = common->sipRmg->allocMemo();
			memo->code = VIVOCE_RESPOND;
			common->signalQ->signalData(memo);
			memo = common->rmg->allocMemo();
			memo->code = VIVOCE_RESPOND;
			common->sendQ->signalData(memo);
			memo = common->rmg->allocMemo();
			memo->code = VIVOCE_RESPOND;
			common->codecQ->signalData(memo);
			memo = common->rmg->allocMemo();
			memo->code = VIVOCE_RESPOND;
			common->playQ->signalData(memo);
			common->audrSig=1;
			common->udprSig=1;
		}
		else if((strncmp("t", cmd, 1)==0) || (strncmp("testaudio", cmd, 6)==0)) {
		    if(common->isConnected) {
		        printf("In a call right now, can't run test...\n"); fflush(stdout);
		    }
		    else {
                if (audioTest == NULL) {
                    char *flag = NULL;
                    int f = 0;
    		        line = strtok_r(cmd, " ", &ptr);
    		        line = strtok_r(NULL, " ", &ptr);
    		        if(ptr) {
                        flag = strtok_r(NULL, " ", &ptr);
                        if(flag) f = atoi(flag);
    		        }
    		        if (line && strncmp("c", line, 1) == 0) {
    		            printf("Audio test with codec, flags=%d...\n", f); fflush(stdout);
                        audioTest = new AudioIOTest(common, audioReader, audioWriter, adpcmCodec, true, f);
    		        }
    		        else {
    		            printf("Audio test without codec, flags=%d...\n", f); fflush(stdout);
    		            audioTest = new AudioIOTest(common, audioReader, audioWriter, adpcmCodec, false, f);
    		        }
    		    }
                if (audioTest->running) {
                    audioTest->running = false;
                    delete audioTest;
                    audioTest = NULL;
                }
                else {
                    audioTest->start();
                }
		    }
		}
		else if(!strncmp("m", cmd, 1) || !strncmp("mute", cmd, 4)) {
            common->audioMute = 1 - common->audioMute; //toggle
            if(common->audioMute) {
                printf("Audio muted."); fflush(stdout);
            }
            else {
                printf("Audio unmuted."); fflush(stdout);
            }            
	    }
		else if(!strncmp("s", cmd, 1) || !strncmp("sine", cmd, 4)) {
            common->audioGenTone = 1 - common->audioGenTone; //toggle
            if(common->audioGenTone) {
                printf("Transmitting a generated tone..."); fflush(stdout);
            }
            else {
                printf("Transmitting recorded audio..."); fflush(stdout);
            }
	    }	    
		else if(!strncmp("x", cmd, 1) || !strncmp("q", cmd, 1) || !strncmp("quit", cmd, 4) || !strncmp("exit", cmd, 4)) {
		    //printf("[%s], done...", cmd);
		    if (audioTest && (audioTest->running)) {
                audioTest->running = false;
                delete audioTest;
		    }
            break;
	    }
		else {
			if(strlen(cmd) && strncmp("exit", cmd, 4) && strncmp("x",cmd, 1))
				printf("\n\tUnrecognized command");
		}
	}
	if(common->isConnected) {
		memo = common->sipRmg->allocMemo();
		memo->code = VIVOCE_SIP_BYE;
		common->signalQ->signalData(memo);
		printf("\nDisconnecting.\n"); fflush(stdout); 
		sleep(1);
	}
	printf("\nShutting down\n");
	common->go=0;
	common->isConnected=0;
	common->signalQ->signal();
	common->sendQ->signal();
	common->codecQ->signal();
	common->playQ->signal();
	common->rtpMgr->close();
	
	printf("\nWaiting for threads to die...\n");
	waitForThreadsToDie();

	common->rmg->clear();
	common->sipRmg->clear();	
	//udpReader->join(NULL);
	if(UdpBase::closeTransport(common) <0) {
		printf("\n\nError in closing sockets... \n");
	}
	printf("Done.\n");
	return 0;
}

int runWithStdPorts(char *name) {
	VivaVoce vivavoce(name);
	return vivavoce.run();
}


int runWithNonStdPorts(char *name, int sipPort, int rtpPort, int rtcpPort) {
	VivaVoce vivavoce(name);
	vivavoce.sipPort  = sipPort;
	vivavoce.rtpPort  = rtpPort;
	vivavoce.rtcpPort = rtcpPort;
	vivavoce.stdPorts = false;
	return vivavoce.run();
}

void usage(void) {
	printf("\nUSAGE: \n\t vivavoce <user-name> [<sip-port> <rtp-port>]\n");
}


int main(int argc, char *argv[]) {
	int 	ret = 0;
//	int 	localPort;
//	char	*remoteAddr;
//	int 	remotePort;

	if((argc != 2) && (argc!=4)) {
		usage();
		return -1;
	}
    
	if(argc == 4) {
		int sipPort  = atoi(argv[2]);
		int rtpPort  = atoi(argv[3]);
		int rtcpPort = rtpPort + 1;
		ret = runWithNonStdPorts(argv[1], sipPort, rtpPort, rtcpPort);
	}
	else {
        ret = runWithStdPorts(argv[1]);
	}

	return ret;
}
