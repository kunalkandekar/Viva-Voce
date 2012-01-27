#include "UserAgent.h"

#define  SIP_UA_DISCONNECTED	60
#define  SIP_UA_SENT_INV		61
#define  SIP_UA_RCVD_INV		62
#define	 SIP_UA_SENT_BYE		63
#define  SIP_UA_ACCEPTED		64
#define	 SIP_UA_REJECTED		65
#define	 SIP_UA_IN_CALL			66

#define  SIP_NONE		50
#define  SIP_DISCONN	51
#define  SIP_CONN		52
#define  SIP_INV_ACK	53
#define  SIP_INCOMING	54
#define  SIP_RINGING	55
#define  SIP_SEND_RING	56
#define  SIP_BYE		57

UserAgent::UserAgent(Common* common):Thread(3, 0, common) {
	state=0;
	callId=0;
	this->common = common;
	audioReader  = NULL;
	checkSrc = false;
	discard  = false;
	remoteRtpPort  = 0;
	remoteRtcpPort = 0;
	remoteSipPort  = 0;
	remoteId   = NULL;
	remoteHost = NULL;

	localId    = NULL;
	localHost  = NULL;

	callId     = 0;
	callIdHost = NULL;

	waitMSec = VIVOCE_WAIT_MSEC;
	defaultAction = 0;
	rptCount = 0;
	cSeqCode = 0;
	sipPackSend = new SIPPacket();
	sipPackRecv = new SIPPacket();

	sdpPackSend = new SDPPacket();
	sdpPackRecv = new SDPPacket();

	sdpConnSend  = new SDPConnectionInfo();
	sdpMediaSend = new SDPMediaDescription();
	sdpMediaRecv = NULL;
	sdpTimeSend  = new SDPTimeDescription();
}

UserAgent::~UserAgent() {
	delete sipPackSend;
	delete sipPackRecv;

	delete sdpPackSend;
	delete sdpPackRecv;

	delete sdpConnSend;
	delete sdpMediaSend;
	delete sdpMediaRecv;
	delete sdpTimeSend;
	
	if(localId)   free(localId);
	if(localHost) free(localHost);
}

int UserAgent::setThreadsToSignal(Thread* audioR) {
	audioReader = audioR;
	return 0;
}

void* UserAgent::run(void* arg) {
	common = (Common *)arg;

	init(0);

	while(common->go) {
		memo = (Memo*)common->signalQ->waitForData(waitMSec);

		if(memo) switch(memo->code) {
			case VIVOCE_SIP_INVITE:
				if(state==SIP_UA_DISCONNECTED) {
					//printf("\n\tSending INVITE....");
					name = strtok_r(memo->bytes,"@",&temp);
					remoteId = (char*)malloc(strlen(name)+1);
					sprintf(remoteId,"%s",name);

					name = strtok_r(NULL,":",&temp);
					if(name) {
						remoteHost = (char*) malloc(strlen(name)+1);
						sprintf(remoteHost,"%s",name);
						name=name = strtok_r(NULL,":",&temp);
						if(name)
							remoteSipPort = atoi(name);
						else
							remoteSipPort = VIVOCE_SIP_PORT;
						sendInvite();
						common->reset();
					}
					else {
						printf("\n\tSIP: Malformed URI");
						common->sipRmg->releaseMemo(memo);
					}
				}
				else
					common->sipRmg->releaseMemo(memo);
				break;

			case VIVOCE_SIP_BYE:
				if(common->isConnected) {
					common->isConnected=0;
					printf("\n\tDisconnecting....");
					rptCount=0;
					sendBye();
				}
				else
					common->sipRmg->releaseMemo(memo);
				break;

			case VIVOCE_SIP_ACCEPT:
				if(state == SIP_UA_RCVD_INV) {
					//printf("\n\tSending OK....");
					sendOk();
					state = SIP_UA_ACCEPTED;
					common->isRinging=0;
				}
				else
					common->sipRmg->releaseMemo(memo);
				break;
			case VIVOCE_SIP_REJECT:
				if(state == SIP_UA_RCVD_INV) {
					//printf("\n\tSending DECLINE....");
					sendDecline();
					common->isRinging=0;
				}
				else
					common->sipRmg->releaseMemo(memo);
				break;

			case VIVOCE_SIP_RCVD:
			//incoming!!
			if(common->verbose) {
				printf("\n\tINFO: SIP Msg Recv=%d bytes:\n%s", memo->data1,memo->bytes);
			}
			sipPackRecv->reset();
			ret = SIPHandler::read(memo->bytes,
								memo->data1,
								sipPackRecv);
			if(ret<0) {
				printf("\n\tSIP: Error in parsing SIP packet");
			}
			else {
			if((!sipPackRecv->isRequest)
				&& (sipPackRecv->responseCode!=SIP_RESPONSE_OK)) {
				printf("\n\tSIP: Received response code %d.",
					sipPackRecv->responseCode);
			}
			switch(state) {
				case SIP_UA_DISCONNECTED:
				if((sipPackRecv->isRequest)
					&& (sipPackRecv->requestCode==SIP_METHOD_INV)) {
					checkPacket();
					if(discard) {
						printf("\n\tSIP: Incorrect SDP content: discarding");
						common->sipRmg->releaseMemo(memo);
						checkSrc=false;
					}
					else {
						callId = sipPackRecv->callId;
						callIdHost = (char*)malloc(strlen(sipPackRecv->callIdHost)+1);
						sprintf(callIdHost, "%s", sipPackRecv->callIdHost);

						remoteId = (char*)malloc(strlen(sipPackRecv->contactUri.id)+1);
						sprintf(remoteId, "%s", sipPackRecv->contactUri.id);

						remoteHost = (char*)malloc(strlen(sipPackRecv->contactUri.host)+1);
						sprintf(remoteHost,"%s",
							sipPackRecv->contactUri.host);
						printf("\n\tSIP: Recvd invite from %s at %s!!!",
							sipPackRecv->contactUri.id,
							sipPackRecv->contactUri.host);
						printf("\n\tRing!!!");
						sendRinging();
						waitMSec=2500;
						state = SIP_UA_RCVD_INV;
						defaultAction = SIP_SEND_RING;
					}
				}
				else
					common->sipRmg->releaseMemo(memo);	//else discard
				break;

				case SIP_UA_SENT_INV:
				if((!sipPackRecv->isRequest)
					&& (sipPackRecv->callId == callId)
					&& (!strcmp(callIdHost,sipPackRecv->callIdHost))) {

					if(sipPackRecv->responseCode==SIP_RESPONSE_RING) {
						printf("\n\tSIP: Ringing callee....");
						common->isRinging=1;
						waitMSec=5000;
						defaultAction = SIP_CONN;
						//display ringing for some time
						common->sipRmg->releaseMemo(memo);
					}
					else if(sipPackRecv->responseCode==SIP_RESPONSE_BUSY) {
						printf("\n\tSIP: Callee is Busy! Try again later");
						waitMSec=500;
						defaultAction = SIP_DISCONN;
						common->sipRmg->releaseMemo(memo);
						common->isRinging=0;
					}
					else if(sipPackRecv->responseCode==SIP_RESPONSE_DECLINE) {
						printf("\n\tSIP: Callee has declined to accept your call");
						waitMSec=500;
						defaultAction = SIP_DISCONN;
						common->sipRmg->releaseMemo(memo);
						common->isRinging=0;
					}
					else if(sipPackRecv->responseCode==SIP_RESPONSE_OK) {
						checkPacket();
						if(discard) {
							printf("\n\tSIP: Incorrect SDP content: discarding");
							common->sipRmg->releaseMemo(memo);
							checkSrc=false;
						}
						else {
							if(common->verbose) {
								printf("\n\tINFO: Listening for RTP/RTCP on ports %d/%d",
									remoteRtpPort,
									remoteRtcpPort);
							}
							UdpBase::initRemoteAddr(common,
                    								remoteHost,
                    								remoteRtpPort,
                    								remoteRtcpPort,
                    								0);
							printf("\n\tSIP: Connected!!!");
							sendAck();
							common->isRinging=0;
						}
					}
					else
						checkSrc=true;
				}
				else
					checkSrc=true;
				break;

				case SIP_UA_SENT_BYE:
				if(!sipPackRecv->isRequest) {
					if(sipPackRecv->responseCode==SIP_RESPONSE_OK) {
						printf("\n\tSIP: Disconnect complete.");
						state = SIP_UA_DISCONNECTED;
						cleanup();
						defaultAction = SIP_NONE;
					}
					common->sipRmg->releaseMemo(memo);
				}
				else
					checkSrc=true;
				break;

				case SIP_UA_ACCEPTED:
				if(sipPackRecv->isRequest
					&& (sipPackRecv->requestCode== SIP_METHOD_ACK)
					&& (sipPackRecv->callId == callId)
					&& (!strcmp(callIdHost, sipPackRecv->callIdHost))) {
					//printf("\n\tSIP: Acknowledged....");
					common->isConnected = 1;
					common->rtpMgr->init(RTP_PAYLOAD_TYPE_ADPCM, const_cast<char*>("stats"));
					waitMSec=2500;
					defaultAction = SIP_NONE;
					state = SIP_UA_IN_CALL;
					common->sipRmg->releaseMemo(memo);
					common->isRinging=0;
				}
				else
					checkSrc=true;
				break;

				case SIP_UA_REJECTED:
				if(sipPackRecv->isRequest
					&& (sipPackRecv->requestCode== SIP_METHOD_ACK)
					&& (sipPackRecv->callId == callId)
					&& (!strcmp(callIdHost,sipPackRecv->callIdHost))) {
					//printf("\n\tSIP: Acknowledged....");
					common->isConnected=0;
					waitMSec=500;
					defaultAction = SIP_DISCONN;
					common->sipRmg->releaseMemo(memo);
					common->isRinging=0;
				}
				else
					checkSrc=true;
				break;

				case SIP_UA_IN_CALL:
				if(sipPackRecv->isRequest
					&& (sipPackRecv->requestCode==SIP_METHOD_BYE)
					&& (sipPackRecv->callId == callId)) {
					//printf("\n\tSending OK for BYE....");
					printf("\n\tSIP: Disconnected by peer.");
					sendOk();
					common->isConnected=0;
					waitMSec=2500;
					defaultAction = SIP_DISCONN;
					state = SIP_UA_DISCONNECTED;
				}
				else
					checkSrc=true;
				break;

				default:
					checkSrc=true;
				break;
			}
			if(checkSrc) {
				checkSrc=false;
				if(sipPackRecv->callId != callId) {
					//printf("\n\tSending BUSY");
					UdpBase::initTempRemoteAddr(common,
                        						sipPackRecv->contactUri.host,
                        						remoteSipPort);
					sendBusy();
				}
				else
					common->sipRmg->releaseMemo(memo);
			}
			break;
			}
			break;

			default:
				printf("\n\tSIP: In invalid state! %d",state);
				common->sipRmg->releaseMemo(memo);
				break;

			case VIVOCE_RESPOND:
				printf("\n\tUser Agent active - %d",state);
				common->sipRmg->releaseMemo(memo);
				break;
		}
		else
			doDefaultAction();

	}
	printf("UserAgent stopped. ");
	return NULL;
}

//util function
//char *strmalloc(const char *str) {
//    int n = strlen(str);
//    char *s = (char*)malloc(n + 1);
//    memcpy(s, str, n + 1);
//    return s;
//}

void UserAgent::init(int port) {
	buffer[0]='\0';
	state = SIP_UA_DISCONNECTED;

	localId = (char*) malloc(strlen(common->username) + 1);
	sprintf(localId,"%s",common->username);
	localHost = (char*) malloc(strlen(common->hostname) + 1);
	sprintf(localHost,"%s",common->hostname);

	if(!common->hostname) {
		common->go=0;
		printf("\n\tSIP: Error - Incorrect uri!");
		return;
	}

	sdpPackSend->reset();
	sdpPackSend->username = common->username;

	sdpPackSend->setVersion(2);
	sdpPackSend->sessionId = rand();
	sdpPackSend->ownerVersion = rand();
	sdpPackSend->networkType = const_cast<char*>("IN");
	sdpPackSend->addressType = const_cast<char*>("IP4");
	sdpPackSend->address     = const_cast<char*>("127.0.0.0");
	sdpPackSend->name        = const_cast<char*>("SIP/SDP/RTP/ADPCM");
	sdpPackSend->info        = const_cast<char*>("VivaVoce IP telephony");
	sdpPackSend->uri         = const_cast<char*>("http://sites.google.com/kunalkandekar/");
	sdpPackSend->email       = const_cast<char*>("kunalkandekar@gmail.com");

	sdpConnSend->networkType = const_cast<char*>("IN");
	sdpConnSend->addressType = const_cast<char*>("IP4");
	sdpConnSend->address     = const_cast<char*>("127.0.0.0");
	sdpPackSend->setConnectionInfo(sdpConnSend);

	sdpMediaSend->media      = const_cast<char*>("audio");
	sdpMediaSend->transport  = const_cast<char*>("RTP/AVP");
	sdpMediaSend->port=common->rtpPort;
	sdpMediaSend->addAttribute(const_cast<char*>("rtpmap:0 PCMU/8000"));
	sdpPackSend->addMediaDescription(sdpMediaSend);

	sdpTimeSend->startTimeSec=0;
	sdpTimeSend->stopTimeSec=0;
	sdpPackSend->addTimeDescription(sdpTimeSend);
    
	ret = SDPHandler::write(sdpPackSend, sdpBuffer, sizeof(sdpBuffer));

	sprintf(tag,"%ud",-1*rand());
	sipPackSend->fromTag=tag;
	sprintf(branch,"%s%ud",SIP_BRANCH_START,-1*rand());
	VIA *via = new VIA();
	via->transport = const_cast<char*>("UDP");
	via->port=common->sipPort;
	via->host=localHost;
	via->branch=branch;
	via->isAuto=true;  //set for autodelete

	sipPackSend->addViaField(via);
	sipPackSend->contactUri.id=localId;
	sipPackSend->contactUri.host = localHost;
	sipPackSend->contactTransport = const_cast<char*>("UDP");
}


void UserAgent::sendInvite(void) {
	sipPackSend->isRequest=true;
	sipPackSend->requestCode=SIP_METHOD_INV;
	sipPackSend->requestUri.id = remoteId;
	sipPackSend->requestUri.host= remoteHost;

	//add From and To
	sipPackSend->fromName 	  = localId;
	sipPackSend->fromUri.id   = localId;
	sipPackSend->fromUri.host = localHost;
	sipPackSend->toName       = remoteId;
	sipPackSend->toUri.id     = remoteId;
	sipPackSend->toUri.host   = remoteHost;
	sprintf(sipPackSend->fromUri.host,"%s",common->hostname);
	sipPackSend->fromName = sipPackSend->fromUri.id;
	sipPackSend->toName = sipPackSend->toUri.id;

	//add SDP
	sipPackSend->contentData = sdpBuffer;
	sipPackSend->contentType = const_cast<char*>("application/sdp");
	sipPackSend->contentLength = strlen(sdpBuffer);

	callId = (double)rand()*(double)rand()*(double)rand();
	callIdHost = localHost;
	sipPackSend->callId = callId;
	sipPackSend->callIdHost=callIdHost;

	sipPackSend->cSeqCode=1;
	sipPackSend->cSeqMethod = const_cast<char*>("INVITE");
	ret = SIPHandler::write(sipPackSend,
			memo->bytes,
			memo->size);
	memo->code=VIVOCE_SIP_SEND;
	memo->data1 = ret;
	UdpBase::initRemoteAddr(common,
                    		remoteHost,
                    		0,
                    		0,
                    		remoteSipPort);
	if(common->verbose) {
		printf("\n\tINFO: SIP Msg=%d bytes:\n%s", ret,memo->bytes);
	}
	common->sendQ->signalData(memo);
	sipPackSend->contentData = NULL;
	sipPackSend->contentType = NULL;
	sipPackSend->contentLength = 0;
	state = SIP_UA_SENT_INV;
	waitMSec=5000;
	defaultAction = SIP_CONN;
	//reset state, show msg
}

void UserAgent::sendBye(void) {
	sipPackSend->isRequest    = true;
	sipPackSend->requestCode  = SIP_METHOD_BYE;
	sipPackSend->requestUri.id   = remoteId;
	sipPackSend->requestUri.host = remoteHost;
	sipPackSend->cSeqCode     = 1;
	sipPackSend->cSeqMethod   = const_cast<char*>("BYE");

	sipPackSend->fromName 	  = localId;
	sipPackSend->fromUri.id   = localId;
	sipPackSend->fromUri.host = localHost;
	sipPackSend->toName 	  = remoteId;
	sipPackSend->toUri.id 	  = remoteId;
	sipPackSend->toUri.host   = remoteHost;
	//add From and To

	ret = SIPHandler::write(
			sipPackSend,
			memo->bytes,
			memo->size);
	memo->code=VIVOCE_SIP_SEND;
	memo->data1 = ret;
	if(common->verbose) {
		printf("\n\tINFO: SIP Msg=%d bytes:\n%s", ret,memo->bytes);
	}
	common->sendQ->signalData(memo);
	state = SIP_UA_SENT_BYE;
	waitMSec=1000;
	defaultAction = SIP_BYE;
	//reset state,cleanup
}

void UserAgent::sendOk(void) {
	sipPackSend->isRequest=false;
	sipPackSend->responseCode=SIP_RESPONSE_OK;

	sipPackSend->contentData = sdpBuffer;
	sipPackSend->contentType = const_cast<char*>("application/sdp");
	sipPackSend->contentLength = strlen(sdpBuffer);
	sipPackSend->callId = callId;
	sipPackSend->callIdHost = callIdHost;
	sipPackSend->isRequest=false;

	//add From and To
	sipPackSend->fromName 	  = sipPackRecv->fromName;
	sipPackSend->fromUri.id   = sipPackRecv->fromUri.id;
	sipPackSend->fromUri.host = sipPackRecv->fromUri.host;
	sipPackSend->toName 	  = sipPackRecv->toName;
	sipPackSend->toUri.id 	  = sipPackRecv->toUri.id;
	sipPackSend->toUri.host	  = sipPackRecv->toUri.host;
	sipPackSend->cSeqCode     = cSeqCode;
	sipPackSend->cSeqMethod   = cSeqMethod;

	ret = SIPHandler::write(sipPackSend,
			memo->bytes,
			memo->size);
	memo->code=VIVOCE_SIP_SEND;
	memo->data1 = ret;
	if(common->verbose) {
		printf("\n\tINFO: SIP Msg=%d bytes:\n%s", ret,memo->bytes);
	}
	common->sendQ->signalData(memo);
	sipPackSend->contentData = NULL;
	sipPackSend->contentType = NULL;
	sipPackSend->contentLength = 0;
}

void UserAgent::sendAck(void) {
	sipPackSend->isRequest=true;
	sipPackSend->requestCode =
		SIP_METHOD_ACK;
	sipPackSend->fromName 	  = sipPackRecv->fromName;
	sipPackSend->fromUri.id   = sipPackRecv->fromUri.id;
	sipPackSend->fromUri.host = sipPackRecv->fromUri.host;
	sipPackSend->toName 	  = sipPackRecv->toName;
	sipPackSend->toUri.id 	  = sipPackRecv->toUri.id;
	sipPackSend->toUri.host	  = sipPackRecv->toUri.host;
	sipPackSend->cSeqCode     = 1;
	sipPackSend->cSeqMethod   = const_cast<char*>("ACK");
	ret = SIPHandler::write(sipPackSend,
			memo->bytes,
			memo->size);
	memo->code=VIVOCE_SIP_SEND;
	memo->data1 = ret;
	common->sendQ->signalData(memo);
	state = SIP_UA_IN_CALL;
	common->isConnected=1;
	common->rtpMgr->init(RTP_PAYLOAD_TYPE_ADPCM, NULL);
	audioReader->signal();
	waitMSec=2500;
	defaultAction = SIP_NONE;
}

void UserAgent::sendDecline(void) {
	sipPackSend->isRequest=false;
	sipPackSend->responseCode = SIP_RESPONSE_DECLINE;

	//add From and To
	sipPackSend->fromName 	  = sipPackRecv->fromName;
	sipPackSend->fromUri.id   = sipPackRecv->fromUri.id;
	sipPackSend->fromUri.host = sipPackRecv->fromUri.host;
	sipPackSend->toName 	  = sipPackRecv->toName;
	sipPackSend->toUri.id 	  = sipPackRecv->toUri.id;
	sipPackSend->toUri.host	  = sipPackRecv->toUri.host;
	sipPackSend->cSeqCode     = cSeqCode;
	sipPackSend->cSeqMethod   = cSeqMethod;

	ret = SIPHandler::write(
			sipPackSend,
			memo->bytes,
			memo->size);
	memo->code=VIVOCE_SIP_SEND;
	memo->data1 = ret;
	if(common->verbose) {
		printf("\n\tINFO: SIP Msg=%d bytes:\n%s", ret,memo->bytes);
	}
	common->sendQ->signalData(memo);
	state = SIP_UA_REJECTED;
	defaultAction = SIP_DISCONN;
}

void UserAgent::sendBusy(void) {
	//send busy signal
	sipPackSend->isRequest=false;
	sipPackSend->responseCode=SIP_RESPONSE_BUSY;

	//add From and To
	sipPackSend->fromName 	  = sipPackRecv->fromUri.id;
	sipPackSend->fromUri.id   = sipPackRecv->fromUri.id;
	sipPackSend->fromUri.host = sipPackSend->fromUri.host;
	sipPackSend->toName 	  = sipPackRecv->toUri.id;
	sipPackSend->toUri.id 	  = sipPackRecv->toUri.id;
	sipPackSend->toUri.host	  = sipPackRecv->toUri.host;
	sipPackSend->callId 	  = sipPackRecv->callId;
	sipPackSend->callIdHost   = sipPackRecv->callIdHost;

	sipPackSend->cSeqCode   = 1;
	sipPackSend->cSeqMethod = const_cast<char*>("INVITE");
	ret = SIPHandler::write(
			sipPackSend,
			memo->bytes,
			memo->size);
	memo->code=VIVOCE_SIP_SEND;
	memo->data1 = ret;
	memo->data2 = 1;
	if(common->verbose) {
		printf("\nINFO: SIP Msg=%d bytes:\n%s", ret,memo->bytes);
	}
	common->sendQ->signalData(memo);
}

void UserAgent::sendRinging(void) {
	sipPackSend->callId = callId;
	sipPackSend->callIdHost = callIdHost;
	sipPackSend->isRequest=false;
	UdpBase::initRemoteAddr(common,
                    		remoteHost,
                    		remoteRtpPort,
                    		remoteRtcpPort,
                    		remoteSipPort);

	//add From and To
	sipPackSend->fromName 	  = sipPackRecv->fromName;
	sipPackSend->fromUri.id   = sipPackRecv->fromUri.id;
	sipPackSend->fromUri.host = sipPackRecv->fromUri.host;
	sipPackSend->toName 	  = sipPackRecv->toName;
	sipPackSend->toUri.id 	  = sipPackRecv->toUri.id;
	sipPackSend->toUri.host	  = sipPackRecv->toUri.host;

	cSeqCode = sipPackRecv->cSeqCode;
	//sprintf(cSeqMethod,"%s", sipPackRecv->cSeqMethod);
	cSeqMethod = sipPackRecv->cSeqMethod;
	sipPackSend->cSeqCode     = cSeqCode;
	sipPackSend->cSeqMethod   = cSeqMethod;

	remoteId = (char*)malloc(strlen(sipPackRecv->contactUri.id)+1);
	sprintf(remoteId,"%s",sipPackRecv->contactUri.id);
	remoteHost = (char*)malloc(strlen(sipPackRecv->contactUri.host)+1);
	sprintf(remoteHost,"%s",sipPackRecv->contactUri.host);

	sipPackSend->responseCode = SIP_RESPONSE_RING;
	ret = SIPHandler::write(sipPackSend,
			memo->bytes,
			memo->size);
	memo->code=VIVOCE_SIP_SEND;
	memo->data1 = ret;
	if(common->verbose) {
		printf("\n\tINFO: Listening for RTP/RTCP on ports %d/%d",
			remoteRtpPort,
			remoteRtcpPort);
		printf("\n\tINFO: SIP Msg=%d bytes:\n%s", ret,memo->bytes);
	}
	common->sendQ->signalData(memo);
	state = SIP_UA_RCVD_INV;
	common->isRinging=1;
	waitMSec=2500;
	defaultAction = SIP_SEND_RING;
	//reset isRinging
}

void UserAgent::checkPacket(void) {
	discard=true;
	sipPackRecv->contentType = strtok_r(sipPackRecv->contentType," ",&temp);
	if((sipPackRecv->contentLength)
		&& (!strncasecmp(sipPackRecv->contentType,"application/sdp",15))){
		sdpPackRecv->reset();
		ret = SDPHandler::read(sipPackRecv->contentData,
				sipPackRecv->contentLength,
				sdpPackRecv);
		if(ret < 0) {
			//malformed packet
			printf("\n\tSDP: Discarding malformed packet (%d)",ret);
		}
		else if(sdpPackRecv->getNumMediaDescriptions()) {
			sdpMediaRecv = sdpPackRecv->getMediaDescription(0);
			sdpMediaRecv->transport
				= strtok_r(sdpMediaRecv->transport," ",&temp);
			sdpMediaRecv->media
				= strtok_r(sdpMediaRecv->media," ",&temp);
			if(!strncasecmp("audio",sdpMediaRecv->media,5)
			   && !strncasecmp("RTP/AVP",sdpMediaRecv->transport,7)) {
				remoteRtpPort = sdpMediaRecv->port;
				remoteRtcpPort = remoteRtpPort+1;
				discard=false;
				printf("\n\tINFO: SDP Msg:\n RTP/RTCP = %d/%d",
					remoteRtpPort,
					remoteRtcpPort);
			}
			else {
				printf("\n\tSIP: Error - Unsupported media/transport: discarding");
			}
		}
		if(common->verbose) {
			if((ret = sdpPackRecv->toString(buffer,1000)) > 0) {
				printf("\n\tINFO: SDP Msg:\n%s", buffer);
			}
			else {
				printf("\n\tINFO: SDP parse error %d",ret);
			}
		}
	}
	else {
		printf("\n\tSIP: Discarding unsupported content type: %s.", 
		  (sipPackRecv->contentLength ? sipPackRecv->contentType : "null"));
	}
	if(sipPackRecv->getNumViaFields()) {
		VIA *via = sipPackRecv->getViaField(0);
		remoteSipPort = via->port;
	}
	else {
		printf("\n\tSIP: Via field missing... discarding");
		discard=true;
	}
}

void UserAgent::doDefaultAction(void) {
	switch(defaultAction) {
		case SIP_DISCONN:
			printf("\n\tSIP: Disconnected.");
			cleanup();
			break;

		case SIP_BYE:
			if(5 > rptCount++) {
				memo =  common->sipRmg->allocMemo();
				sendBye();
				defaultAction = SIP_BYE;
			}
			else {
				rptCount=0;
				state = SIP_UA_DISCONNECTED;
				defaultAction = SIP_DISCONN;
			}
			waitMSec=500;
			break;

		case SIP_CONN:
			printf("\n\tSIP: Connect failed: Timed out!");
			state = SIP_UA_DISCONNECTED;
			cleanup();
			defaultAction= SIP_NONE;
			break;

		case SIP_INV_ACK:
			defaultAction= SIP_NONE;
			break;

		case SIP_INCOMING:
			common->isRinging=0;
			defaultAction= SIP_NONE;
			cleanup();
			break;

		case SIP_RINGING:
			printf("\n\tSIP: Ringing callee...");
			waitMSec=3000;
			break;

		case SIP_SEND_RING:
			printf("\n\tRing #%d!!!",rptCount);
			if(5 > rptCount++) {
				memo =  common->sipRmg->allocMemo();
				sendRinging();
				defaultAction = SIP_SEND_RING;
			}
			else {
				rptCount=0;
				defaultAction = SIP_INCOMING;
			}
			waitMSec=3000;
			break;

		default:
			break;
	}
}

void UserAgent::cleanup(void) {
	common->isConnected=0;
	common->isRinging=0;
	common->rtpMgr->close();
	sipPackSend->toName=NULL;
	sipPackSend->toUri.id=NULL;
	sipPackSend->toUri.host=NULL;
	sipPackSend->callId = 0;
	callId=0;
	if((callIdHost) && (localHost!=callIdHost))
		free(callIdHost);
	if(remoteHost)
		free(remoteHost);
	if(remoteId)
		free(remoteId);
	remoteHost=NULL;
	remoteId=NULL;

	emptyQueue(common->sendQ,common->rmg);
	emptyQueue(common->codecQ,common->rmg);
	emptyQueue(common->playQ,common->rmg);

	sipPackSend->cSeqCode = 0;
	sipPackSend->cSeqMethod = NULL;
	state = SIP_UA_DISCONNECTED;
	defaultAction= SIP_NONE;
}
