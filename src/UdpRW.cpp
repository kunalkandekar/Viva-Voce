#include "UdpRW.h"

#define debug //print

#define mod(d) (d<0?-d:d)

UdpBase::UdpBase(int id, int priority, Common* common) : Thread(id, priority, common) {
}

UdpBase::~UdpBase() {
}

int UdpBase::initTransport(Common *common) {
    char hname[256];
	gethostname(hname,sizeof(hname));
	common->setHostName(hname);
	return initSIPOverUDP(common) + initRTPOverUDP(common);
}

int UdpBase::closeTransport(Common *common) {
	int ret=0;
	if(common->sipSocket) {
		ret+=close(common->sipSocket);
	}
	if(common->rtpSocket) {
		ret+=close(common->rtpSocket);
	}
	if(common->rtcpSocket) {
		ret+=close(common->rtcpSocket);
	}
	return ret;
}


int UdpBase::initSIPOverUDP(Common *common) {
	if ((common->sipSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("\tCan't open socket");
		return -1;
	}
	common->sipAddr.sin_family = AF_INET;
	common->sipAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	common->sipAddr.sin_port = htons(common->sipPort);
	if(bind(common->sipSocket,
			(struct sockaddr *)&common->sipAddr,
			sizeof(common->sipAddr)) < 0)	{
		fprintf(stderr,"Port %d is being used\n",VIVOCE_SIP_PORT);
		return -1;
	}
	return 0;
}

int UdpBase::initRTPOverUDP(Common *common) {
	if ((common->rtpSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("\tCan't open socket");
		return -1;
	}
	common->rtpAddr.sin_family = AF_INET;
	common->rtpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	common->rtpAddr.sin_port = htons(common->rtpPort);
	if(bind(common->rtpSocket,
			(struct sockaddr *)&common->rtpAddr,
			sizeof(common->rtpAddr)) < 0)	{
		fprintf(stderr,"Port %d is being used\n",VIVOCE_RTP_PORT);
		return -1;
	}

	if ((common->rtcpSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("\tCan't open socket");
		return -1;
	}
	common->rtcpAddr.sin_family = AF_INET;
	common->rtcpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	common->rtcpAddr.sin_port = htons(common->rtcpPort);
	if(bind(common->rtcpSocket,
			(struct sockaddr *)&common->rtcpAddr,
			sizeof(common->rtcpAddr)) < 0)	{
		fprintf(stderr,"Port %d is being used\n",VIVOCE_RTCP_PORT);
		return -1;
	}
	return 0;
}

int UdpBase::initRemoteAddr( Common *common,
					char* remoteName,
					int remoteRTPPort,
					int remoteRTCPPort,
					int remoteSIPPort) {
	common->hp = gethostbyname(remoteName);
	if (common->hp ==  NULL) {
		fprintf(stderr,"Unknown host\n");
		return -1;
	}
	//printf("\n\tUDP: Connecting to addr = %s \n\t",common->hp->h_name);
	common->remoteName = remoteName;

	if(remoteRTPPort) {
		common->remoteRTPAddr.sin_family = AF_INET;
		common->remoteRTPAddr.sin_addr
			= *(struct in_addr*) common->hp->h_addr;
		common->remoteRTPAddr.sin_port = htons(remoteRTPPort);
	}
	if(remoteRTCPPort) {
		common->remoteRTCPAddr.sin_family = AF_INET;
		common->remoteRTCPAddr.sin_addr
			= *(struct in_addr*) common->hp->h_addr;
		common->remoteRTCPAddr.sin_port = htons(remoteRTCPPort);
	}

	if(remoteSIPPort) {
		common->remoteSIPAddr.sin_family = AF_INET;
		common->remoteSIPAddr.sin_addr
			= *(struct in_addr*) common->hp->h_addr;
		common->remoteSIPAddr.sin_port = htons(remoteSIPPort);
	}
	return 0;
}

int UdpBase::initTempRemoteAddr( Common *common,
					char* remoteName,
					int remoteTempPort) {
	common->hp = gethostbyname(remoteName);
	if (common->hp ==  NULL) {
		fprintf(stderr,"Unknown host\n");
		return -1;
	}
	if(remoteTempPort) {
		common->remoteTempAddr.sin_family = AF_INET;
		common->remoteTempAddr.sin_addr
			= *(struct in_addr*) common->hp->h_addr;
		common->remoteTempAddr.sin_port = htons(remoteTempPort);
	}
	return 0;
}


UdpReader::UdpReader(Common* common):UdpBase(4, 0, common) {

}

UdpReader::~UdpReader() {

}

//rtpAudioPayload* payload;

void* UdpReader::run(void* arg) {
	common = (Common *)arg;

	struct timeval* timeout   = new timeval();
	int 	maxSock;
	int 	nFound;

	fd_set	readSet;
	int 	ret;
	int 	addrLen = sizeof(common->recvAddr);


	Memo	*memo;

	maxSock	= (	common->rtpSocket > common->rtcpSocket?
				common->rtpSocket+1:common->rtcpSocket+1);

	maxSock	= (	maxSock > common->sipSocket?
				maxSock:common->sipSocket+1);

	while(common->go) {
		FD_ZERO(&readSet);
		FD_SET(common->sipSocket, &readSet);
		FD_SET(common->rtpSocket, &readSet);
		FD_SET(common->rtcpSocket, &readSet);

		timeout->tv_sec	=2;
		timeout->tv_usec=500000;

		nFound= select(maxSock, &readSet, (fd_set*)0, (fd_set*)0, timeout);

		if(nFound==0) {
			//timed out
			if(common->udprSig) {
				printf("\n\tUDP Reader active");
				common->udprSig=0;
			}
			continue;
		}

		if(nFound <0) {
			perror("\nError in reading socket:");
			break;
		}

		//available to read on SIP port
		if((nFound = FD_ISSET(common->sipSocket, &readSet)) > 0) {
			memo =  common->sipRmg->allocMemo();
			memo->code = VIVOCE_SIP_RCVD;

			ret =  recvfrom(common->sipSocket,
							memo->bytes,
							memo->size,
							0,
							(sockaddr*)&common->recvAddr,
							(socklen_t*)&addrLen);
			if(ret > 0) {
				memo->data1 = ret;
				common->signalQ->signalData(memo);
				if(common->verbose) {
					printf("\n\tINFO: Recvd signal %d bytes on SIP socket",
						ret);
					common->hp =
						gethostbyaddr(
								(char*)&common->recvAddr.sin_addr.s_addr,
								sizeof(in_addr),
								AF_INET);
					if (common->hp ==  NULL) {
						fprintf(stderr,"Unknown host\n");
					}
					else {
					   printf(" from %s \n",common->hp->h_name);
					}
				}
			}
			else {
				common->sipRmg->releaseMemo(memo);
				if(ret <0) {
					perror("\n\tError in reading SIP socket:");
				}
				else if(ret == 0) {
					//sock closed
				}
			}
		}

		//available to read on RTP port
		if((nFound = FD_ISSET(common->rtpSocket, &readSet)) > 0) {
			ret = recvfrom(	common->rtpSocket,
							common->ipBuf,
							common->ipBufSize,
							0,
							(sockaddr*)&common->recvAddr,
							(socklen_t*)&addrLen);
			if(ret > 0) {
				if(common->isConnected) {
					//only handle if connected, else discard
					common->bytesRcv+=ret;
					common->unitsRcv++;
					memo =  common->rmg->allocMemo();
					memo->data1 = common->rtpMgr->handleRTPRecv(common->ipBuf,
									ret,
									memo->bytes,
									memo->size);
					memo->code = VIVOCE_MSG_DEC;
					common->codecQ->signalData(memo);
				}
			}
			else if(ret <0) {
				perror("\n\tError in reading RTP socket:");
			}
			else {
				//sock closed
			}
		}

		//available to read on RTCP port
		if((nFound = FD_ISSET(common->rtcpSocket, &readSet)) > 0) {
			ret = recvfrom(	common->rtcpSocket,
							common->ipBuf,
							common->ipBufSize,
							0,
							(sockaddr*)&common->recvAddr,
							(socklen_t*)&addrLen);
			if(ret >0) {
				if(common->isConnected) {
					ret = common->rtpMgr->handleRTCPRecv(common->ipBuf,ret);
					//only handle if connected, else discard
					if(common->verbose)
						printf("\n\tINFO: Recvd RTCP %cR",(ret>0?'S':'R'));
					if(ret > 0) {
						common->sendRR=true;		//respond with an RR
						common->sendQ->signal();
					}
					else if(ret<0){
						printf("\n\t Error: malformed RTCP packet");
					}
				}
			} else if(ret <0) {
				perror("\n\tError in reading RTCP socket:");
			} else {
				//sock closed
			}
		}
	}
	delete timeout;
	printf("UdpReader stopped. ");
	return NULL;
}

UdpWriter::UdpWriter(Common* common):UdpBase(5, 0, common) {

}

UdpWriter::~UdpWriter() {
}

int d;
int delay;

void simulateJitter(int avg) {
	delay = rand()%10000;

	for(d=0;d<delay; d++) {
		//do nothing
	}
}

void* UdpWriter::run(void* arg) {
	common = (Common *)arg;
	Memo* 	memo;
	int 	ret=0;
	int 	sipAddrLen = sizeof(common->remoteSIPAddr);
	int 	rtpAddrLen = sizeof(common->remoteRTPAddr);
	int 	rtcpAddrLen = sizeof(common->remoteRTCPAddr);

	common->msecLastBurst=0;

	while(common->go) {
		memo = (Memo*)common->sendQ->waitForData(20);
		if(memo!=NULL) {
			switch(memo->code) {
			case VIVOCE_SIP_SEND:
				if(memo->data2) {
					//send to temp address
					memo->data2=0;
					ret = sendto(common->sipSocket,
							 (char*)memo->bytes,
							 memo->data1,
							 0,
							 (sockaddr*)&common->remoteTempAddr,
							 sizeof(common->remoteTempAddr));
					if(common->verbose) {
						common->hp =
							gethostbyaddr(
									(char*)&common->remoteTempAddr.sin_addr.s_addr,
									sizeof(common->remoteTempAddr.sin_addr.s_addr),
									AF_INET);
						if (common->hp ==  NULL) {
							perror("Unknown host");
						}
						printf(" to temp %s \n",common->hp->h_name);
						if(ret < 0)
							perror("\tError on SIP Socket");
					}
				}
				else {
					//send to established address
					ret = sendto(common->sipSocket,
							 (char*)memo->bytes,
							 memo->data1,
							 0,
							 (sockaddr*)&common->remoteSIPAddr,
							 sipAddrLen);
					if(common->verbose) {
						common->hp =
							gethostbyaddr(
									(char*)&common->remoteSIPAddr.sin_addr.s_addr,
									sizeof(common->remoteSIPAddr.sin_addr.s_addr),
									AF_INET);
						if (common->hp ==  NULL) {
							perror("Unknown host\n");
						}
						else {
						  printf(" to %s \n",common->hp->h_name);
						}
						if(ret < 0)
							perror("\tError on SIP Socket");
					}
				}
				common->sipRmg->releaseMemo(memo);
				break;

			case VIVOCE_MSG_SEND:
				//Send encoded audio data from Q
				if(common->isConnected) {
					ret = common->rtpMgr->handleRTPSend(memo->bytes,
											memo->data1,
											common->opBuf,
											common->opBufSize);
					ret = sendto(common->rtpSocket,
								 (char*)common->opBuf,
								 ret,
								 0,
								 (sockaddr*)&common->remoteRTPAddr,
								 rtpAddrLen);
					common->bytesSnt+=ret;
					common->unitsSnt++;

					if(ret < 0 ) {
						perror("\tError on RTP Socket");
						printf("\t%d:%d:%d",
							common->rtpSocket,
							ret,
							rtpAddrLen);
						common->go=0;
						common->isConnected=0;
					}
				}
				common->rmg->releaseMemo(memo);
				break;

			case VIVOCE_MSG_REPORT:		//data from Q
				common->rmg->releaseMemo(memo);
				break;

			case VIVOCE_RESPOND:
				printf("\n\tUDP Writer active");
				common->rmg->releaseMemo(memo);
				break;
			}
		}
		if( common->isConnected ) {
			/*if(!common->rtpMgr->isPeerAlive()) {
				printf("\n\tError: No reports from peer for long time");
				printf("\n\tPeer may be down;");
				printf("Disconnecting stream, you can try ringing again.\n");
				memo = common->sipRmg->allocMemo();
				memo->code = VIVOCE_SIP_BYE;
				common->signalQ->signalData(memo);
				common->isConnected=0;
				continue;
			}*/
			if(!common->sendSR)
				common->sendSR = common->rtpMgr->isTimeForSR();
			//Send Sender Report
			if(common->sendSR) {
				common->sendSR = false;
				ret = common->rtpMgr->handleRTCPSendSR(common->opBuf,
										common->opBufSize);

				if(ret>=0) {
					ret = sendto(common->rtcpSocket,
							 (char*)common->opBuf,
							 ret,
							 0,
							 (sockaddr*)&common->remoteRTCPAddr,
							 rtcpAddrLen);
					if(common->verbose) {
						printf("\n\tINFO: Sent RTCP SR msg of %d bytes", ret);
						common->hp =
							gethostbyaddr(
									(char*)&common->remoteRTCPAddr.sin_addr.s_addr,
									sizeof(common->remoteRTCPAddr.sin_addr.s_addr),
									AF_INET);
						if (common->hp ==  NULL) {
							perror("Unknown host");
						}
						else
							printf(" to RTCP %s \n",common->hp->h_name);
					}
					if(ret < 0)
						perror("\tError on RTCP Socket for SR");
					ret = common->rmg->downsize();
					//printf("\n\tDownsized rmg by %d ",ret);
					ret = common->sipRmg->downsize();
					//printf("and sipRmg by %d ",ret);
				}
			}

			//Send Receiver Report
			if(common->sendRR) {
				common->sendRR=false;
				ret = common->rtpMgr->handleRTCPSendRR(common->opBuf,
						common->opBufSize);
				if(ret >=0) {
					ret = sendto(common->rtcpSocket,
							 (char*)common->opBuf,
							 ret,
							 0,
							 (sockaddr*)&common->remoteRTCPAddr,
							 rtcpAddrLen);
					if(common->verbose) {
						printf("\n\tINFO: Sent RTCP RR msg of %d bytes", ret);
						common->hp =
							gethostbyaddr(
								(char*)&common->remoteRTCPAddr.sin_addr.s_addr,
								sizeof(common->remoteRTCPAddr.sin_addr.s_addr),
								AF_INET);
						if (common->hp ==  NULL) {
							perror("Unknown host");
						}
						else
							printf(" to RTCP %s\n",common->hp->h_name);
					}
					if(ret < 0)
						perror("\tError on RTCP Socket for RR");
				}
			}
		}
	}
	printf("UdpWriter stopped. ");
	return NULL;
}