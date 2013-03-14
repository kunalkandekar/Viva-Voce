#include "SDP.h"

static const char* SDPFields 		= "vosiuepcbtrzkamicbka";
static const char* SDPMediaFields	= "micbka";
//static const char* SDPTimeFields	= "tr";

#define debug(...) if(0)printf(__VA_ARGS__)

/**************** Connection Descriptor ****************************/
SDPConnectionInfo::SDPConnectionInfo() {
	inputString = NULL;
	isAuto = false;
    TTL = 0;
    numAddress = 0;
}

SDPConnectionInfo::~SDPConnectionInfo() {

}


int SDPConnectionInfo::setConnectionInfo(char* str) {
	char *temp;

	inputString = str;
	networkType = strtok(inputString," ");
	if(networkType==NULL)
		return -1;

	addressType = strtok(NULL," ");
	if(addressType==NULL)
		return -1;
	temp 		= strtok(NULL," ");
	if(temp==NULL)
		return -1;

	address 	= strtok(temp,"/");
	addressLen  = strlen(address);
	ipAddress   = inet_addr(address);

	temp 		= strtok(NULL,"/");
	if(temp) {
		TTL  = atoi(temp);
		temp = strtok(NULL,"/");
		if(temp) {
			numAddress = atoi(temp);
		}
	}
	return 0;
}

/**
 * Convenience method - for display
 */
int SDPConnectionInfo::toString(char* buffer, int len) {
	if(len < 150)
		return -1;
	return sprintf(buffer,
		"\n\t Network type = %s %s%s %s%s %s%d %s%d",
		(networkType?networkType:""),
		"\n\t Address type = ",
		(addressType?addressType:""),
		"\n\t Address      = ",
		(address?address:""),
		"\n\t TTL          = ",
		TTL,
		"\n\t Num Address  = ",
		numAddress);
}


/************************ MEDIA DESCRIPTOR *************************/
SDPMediaDescription::SDPMediaDescription() {
	port=0;
	nPorts=0;
	fmtList = NULL;
	fmtListLen = 0;
	title = NULL;
	bandwidth = NULL;
	encryptionKey = NULL;
	isAuto=false;
	sdpConnInfo = NULL;
}

SDPMediaDescription::~SDPMediaDescription() {
	char *temp;
	if(sdpConnInfo)
		delete sdpConnInfo;
	if(fmtList)
		delete [] fmtList;
	for(unsigned int itr=0; itr < attribsList.size(); itr++){
		temp = (char*)attribsList[itr];
		free(temp);
	}
	attribsList.clear();
}

int SDPMediaDescription::setField(char id, char* str) {
	int 	itr=0;
	char*	stkField;
	char* 	stkSubField;
	char*	temp;

	switch(id) {
		case 'm':
			//media
			media = strtok_r(str," ",&stkField);
			if(media==NULL)
				return -1;
			temp = strtok_r(NULL," ",&stkField);
			if(temp==NULL)
				return -1;

			//port
			temp = strtok_r(temp,"/",&stkSubField);
			if(temp==NULL)
				return -1;
			port = atoi(temp);

			//nports
			temp = strtok_r(NULL,"/",&stkSubField);
			if(temp)
				nPorts = atoi(temp);

			transport = strtok_r(NULL," ",&stkField);
			if(transport==NULL)
				return -1;

			temp = strtok_r(NULL," ",&stkField);
			if(temp) {
				std::vector<int> tempList;
				for(temp = strtok_r(temp," ",&stkSubField);
					temp;
					temp = strtok_r(NULL," ",&stkSubField)) {
					tempList.push_back(atoi(temp));
				}
				fmtListLen = tempList.size();
				fmtList = new int[fmtListLen];
				for(itr=0; itr<fmtListLen; itr++) {
					fmtList[itr] = (int)tempList[itr];
				}
				tempList.clear();
			}
			break;
		case 'i':
			title = str;
			break;
		case 'c':
			setConnectionInfo(str);
			break;
		case 'b':
			bandwidth = str;
			break;
		case 'k':
			encryptionKey = str;
			break;
		case 'a':
			addAttribute(str);
			break;
		default:
			//unspecified type
			return -1;
	}
	return 0;
}

/**
 * Add media-level attributes
 */
void SDPMediaDescription::addAttribute(char* str) {
    int l = strlen(str);
	char *temp = (char*)malloc(l+1);
	strcpy(temp,str);
	//temp[l] = '\0';
	attribsList.push_back((void*)temp);
}

/**
 * Return all session-level attributes
 */
int SDPMediaDescription::getNumAttributes(void) {
	return attribsList.size();
}


int	SDPMediaDescription::getAttribute(char *buff, unsigned int len, unsigned int index) {
	if(index >= attribsList.size())
		return 0;
	if(len < (strlen((char*)attribsList[index]) + 1))
		return -1;
	strcpy(buff,(char*)attribsList[index]);
	buff[strlen((char*)attribsList[index])] = '\0';
	return 1;
}

/**
 * Connection info
 */
int SDPMediaDescription::setConnectionInfo(char* str) {
	//connInfo = new char[strlen(str)+1];
	//strcpy(connInfo,str);

	sdpConnInfo = new SDPConnectionInfo();

	if( sdpConnInfo->setConnectionInfo(str) < 0) {
		delete sdpConnInfo;
		sdpConnInfo = NULL;
		return -1;
	}
	return 0;
}

char* SDPMediaDescription::getConnectionString(void) {
	return NULL;
}

SDPConnectionInfo* SDPMediaDescription::getConnectionInfo(void) {
	return sdpConnInfo;
}

/**
 * Convenience method - for display
 */
int SDPMediaDescription::toString(char* buffer, int len) {
	int ret;
	int posn=0;
	if(len < 250)
		return -1;
	ret = sprintf(buffer,
		"\n\t Media     = %s %s%d%s%d",
		(media?media:""),
		"\n\t port      = ",
		port,
		", nPorts   = ",
		nPorts);
	posn+=ret;

	if(transport!=NULL) {
		posn+=sprintf(buffer+posn,
				", Transport = %s",
				(transport?transport:""));
	}

	if(fmtList!=NULL) {
		ret = sprintf(buffer+posn,
			"\n\t Formats   = [");
		posn+=ret;
		for(int i=0; i<fmtListLen; i++) {
			posn+=sprintf(buffer+posn,
				"%d,",fmtList[i]);
		}
		buffer[posn]=']';
		posn++;
	}
	buffer[posn] = '\n';
	posn++;
	return posn;
}

/************************ TIME DESCRIPTOR *************************/
int SDPTimeDescription::isTimeField(char c) {
	return ((c=='t')||(c=='r'));
}

SDPTimeDescription::SDPTimeDescription(void) {
	time = NULL;
	reps = NULL;

	startTimeSec = 0;
	stopTimeSec = 0;

	ntpStartTimeSec = 0;
	ntpStopTimeSec = 0;

	repeatIntervalSec = 0;
	activeDurationSec = 0;
	offsets=NULL;
	nOffsets=0;
	isAuto=false;
}


SDPTimeDescription::~SDPTimeDescription(void) {
	if(offsets)
		delete [] offsets;
    offsets = NULL;
}


long SDPTimeDescription::parseTime(char* str) {
	int len = strlen(str);
	char ch = str[len-1];
	unsigned long mul=1;
	if(!isdigit(ch)) {
		switch(ch) {
			case 'd':
				mul=86400;
				break;
			case 'h':
				mul=3600;
				break;
			case 'm':
				mul=60;
				break;
			case 's':
				break;
			default:
				return -1;
		}
	}
	char* tempStr = str;   //new char[len];
//	strncpy(tempStr,str,len-1);
//	tempStr[len]='\0';
	mul = mul*atol(tempStr);
//	delete tempStr;
	return mul;
}

int SDPTimeDescription::setField(char id, char* str) {
	char *temp;
	if(id=='t') {
		time = str;

		temp = strtok(time," ");
		if(temp) {
			ntpStartTimeSec = parseTime(temp);
		}
		else
			return -1;
		temp = strtok(NULL," ");
		if(temp) {
			ntpStopTimeSec = parseTime(temp);
		}
		else
			return -1;
		//startTimeSec = ntpStartTimeSec + NTP_2_UNIX_DIFF*100;
		//stopTimeSec = ntpStopTimeSec +  NTP_2_UNIX_DIFF*100;
	} else if(id=='r') {
		reps = str;

		temp = strtok(reps," ");
		if(temp) {
			repeatIntervalSec = atol(temp);
		}
		else
			return -1;

		temp = strtok(NULL," ");
		if(temp) {
			activeDurationSec = atol(temp);
		}
		else
			return -1;

		Vector tempList;
		for(temp = strtok(NULL," ");
			temp;
			temp = strtok(NULL," ")) {
			tempList.push_back((void*) atol(temp));
		}
		nOffsets = tempList.size();
		offsets = new long[nOffsets];
		for(int itr=0; itr<nOffsets; itr++){
			offsets[itr] = (long)tempList[itr];
		}
		tempList.clear();
	} else {
		//unrecognized field
		return -1;
	}

	return 0;
}


int SDPTimeDescription::setFields(char* strTime, char* strReps) {
	return (setField('t',strTime) + setField('r',strReps));
}

/**
 * Convenience method - for display
 */
int SDPTimeDescription::toString(char* buffer, int len) {
	int ret;

	int posn=0;
	//SDPTimeDescription* timeDesc;

	if(len < 250)
		return -1;

	ret = sprintf(buffer,
		"\n\t Start time = %ld %s%ld %s%ld %s%ld",
		ntpStartTimeSec,
		"\n\t Stop time  = ",
		ntpStopTimeSec,
		"\n\t Interval   = ",
		repeatIntervalSec,
		"\n\t Duration   = ",
		activeDurationSec);
	posn+=ret;

	if(offsets!=NULL) {
		posn+=sprintf(buffer+posn,"\n\t Offsets    = [");

		for(int i=0; i<nOffsets; i++) {
			posn+=sprintf(buffer+posn,
				"%ld,",
				offsets[i]);
		}
		buffer[posn] = ']';
		posn++;
	}
	buffer[posn] = '\n';
	posn++;
	return posn;
}

/****************************** PACKET ******************************/
SDPPacket::SDPPacket() {
	nConnInfo=0;
	version = 0;
	owner = NULL;

	//owner info
	username = NULL;
	sessionIdStr = NULL;
	ownerVersionStr = NULL;
	networkType = NULL;
	address = NULL;
	addressType = NULL;

	//session info
	name = NULL;
	info = NULL;
	uri = NULL;
	email = NULL;
	phone = NULL;

	bandwidth = NULL;

	timezoneAdj = NULL;
	encryptionKey = NULL;

	//connection info
	connInfo = NULL;
	sdpConnInfo = NULL;
	input = NULL;
}

/**
 * Destructor
 */
SDPPacket::~SDPPacket() {
	reset();
}

void SDPPacket::reset() {
	char *temp;
	unsigned int itr;
	SDPMediaDescription* mediaDesc = NULL;
	SDPTimeDescription* timeDesc = NULL;

	for(itr=0; itr < mediaDescList.size(); itr++){
		mediaDesc = (SDPMediaDescription*)mediaDescList[itr];
		if(mediaDesc->isAuto)
			delete mediaDesc;
	}
	mediaDescList.clear();

	for(itr=0; itr < timeDescList.size(); itr++){
		timeDesc = (SDPTimeDescription*)timeDescList[itr];
		if(timeDesc->isAuto)
			delete timeDesc;
	}
	timeDescList.clear();

	nConnInfo=0;
	version = 0;

	//session info
	/*
	if(owner)
		delete owner;
	owner = NULL;

	if(name)
		delete name;
	name = NULL;
	if(info)
		delete info;
	info = NULL;
	if(uri)
		delete uri;
	uri = NULL;
	if(email)
		delete email;
	email = NULL;
	if(phone)
		delete phone;
	phone = NULL;
	if(bandwidth)
		delete bandwidth;
	bandwidth = NULL;
	if(timezoneAdj)
		delete timezoneAdj;
	timezoneAdj = NULL;
	if(encryptionKey)
		delete encryptionKey;
	encryptionKey = NULL;
	*/

	//connection info
	if(connInfo)
		delete connInfo;
	connInfo = NULL;
	if(sdpConnInfo && sdpConnInfo->isAuto)
		delete sdpConnInfo;
	sdpConnInfo = NULL;

	if(input)
		free(input);
	input = NULL;

	for(unsigned int itr=0; itr<attribsList.size(); itr++){
		temp = (char*)attribsList[itr];
		free(temp);
	}
	attribsList.clear();
}


int SDPPacket::setVersion(char* str) {
	version = atoi(str);
	return 0;
}

int SDPPacket::setVersion(int ver) {
	version = ver;
	return 0;
}

int SDPPacket::getVersion(void) {
	return version;
}

void SDPPacket::setOwner(char* str) {
	owner = str;
	/*new char[strlen(str)+1];
	strcpy(owner,str);
	owner[strlen(str)] = '\0';*/

	username = strtok(owner," ");
	if(username) {
		sessionIdStr = strtok(NULL," ");
		if(sessionIdStr) {
			sessionId = atol(sessionIdStr);
			ownerVersionStr = strtok(NULL," ");
			if(ownerVersionStr) {
				ownerVersion = atol(ownerVersionStr);
				networkType = strtok(NULL," ");
				if(networkType) {
					addressType = strtok(NULL," ");
					if(addressType) {
						address = strtok(NULL," ");
					}
				}
			}
		}
	}
}

char* SDPPacket::getOwner() {
	return owner;
}

/**
 * Connection info
 */
int SDPPacket::setConnection(char* str) {
	sdpConnInfo = new SDPConnectionInfo();
	sdpConnInfo->isAuto=true;
	if( sdpConnInfo->setConnectionInfo(str) < 0) {
		delete sdpConnInfo;
		sdpConnInfo = NULL;
		return -1;
	}
	nConnInfo++;
	return 0;
}

int SDPPacket::setConnectionInfo(SDPConnectionInfo* connInfo) {
	if(sdpConnInfo) {
		return -10;			//SDP_ERR_CONN_PRESENT
	}
	sdpConnInfo = connInfo;
	nConnInfo++;
	return 0;
}

char* SDPPacket::getConnectionString(void) {
	return NULL;
}

SDPConnectionInfo* SDPPacket::getConnectionInfo() {
	return sdpConnInfo;
}

int SDPPacket::getNumTotalConnections(void) {
	return nConnInfo;
}

/**
 * Add media desciption
 */
void SDPPacket::addMediaDescription(SDPMediaDescription *desc) {
	if(desc->getConnectionInfo()!=NULL)
		nConnInfo++;
	mediaDescList.push_back((void*)desc);
}

/**
 * Check number of media desciptions for this session
 */
int SDPPacket::getNumMediaDescriptions(void) {
	return mediaDescList.size();
}

/**
 * Get all media descriptions
 */
SDPMediaDescription* SDPPacket::getMediaDescription(unsigned int index) {
	if(index >= mediaDescList.size())
		return NULL;
	return (SDPMediaDescription*)mediaDescList[index];
}

/**
 * Add time desciption
 */
void SDPPacket::addTimeDescription(SDPTimeDescription *desc) {
	timeDescList.push_back(desc);
}

/**
 * Check number of time desciptions for this session
 */
int SDPPacket::getNumTimeDescriptions(void) {
	return timeDescList.size();
}

/**
 * Get all time descriptions
 */
SDPTimeDescription* SDPPacket::getTimeDescription(unsigned int index) {
	if(index >= timeDescList.size())
		return NULL;
	return (SDPTimeDescription*)timeDescList[index];
}

/**
 * Add session-level attributes
 */
void SDPPacket::addAttribute(char* str) {
	char *temp = (char*)malloc(strlen(str)+1);
	strcpy(temp,str);
	//temp[strlen(str)] = '\0';
	attribsList.push_back((void*)temp);
}

int	SDPPacket::getAttribute(char *buff, unsigned int len, unsigned int index) {
	if(index >= attribsList.size())
		return 0;
	if(len < strlen((char*)attribsList[index])+1)
		return -1;
	strcpy(buff,(char*)attribsList[index]);
	buff[strlen((char*)attribsList[index])]='\0';
	return 1;
}

/**
 * Return all session-level attributes
 */
int SDPPacket::getNumAttributes(void) {
	return attribsList.size();
}

/**
 * Set session field
 */
int SDPPacket::setField(char id, char* str) {

	switch(id) {
		case 'v':
			return setVersion(str);
		case 'o':
			setOwner(str);
			break;
		case 's':
			name = str;
			/* name = new char[strlen(str)+1];
			strcpy(name,str);
			name[strlen(str)]='\0';*/
			break;
		case 'i':
			info = str;
			break;
		case 'u':
			uri = str;
			break;
		case 'e':
			email = str;
			break;
		case 'p':
			phone = str;
			break;
		case 'c':
			return setConnection(str);
		case 'b':
			bandwidth = str;
			break;
		case 'z':
			timezoneAdj = str;
			break;
		case 'k':
			encryptionKey = str;
			break;
		case 'a':
			addAttribute(str);
			break;
		default:
			return -1;
	}

	return 0;
}

/**
 * Convenience method - for display
 */
int SDPPacket::toString(char* buffer, int len) {
	int ret;
	unsigned int itr;
	int posn=0;
	SDPTimeDescription* timeDesc;
	SDPMediaDescription* mediaDesc;

	if(len < 750)
		return -1;
	posn+=sprintf(buffer,
		"\nversion = %d \nowner   = %s \nname    = %s",
		version,
		(owner?owner:""),
		(name?name:""));

	if(sdpConnInfo!=NULL) {
		posn+=sprintf(buffer+posn,"\nConnection: ");
		ret = sdpConnInfo->toString(buffer+posn,len-posn);
		if(ret < 0)
			return -1;
		posn+=ret;
	}
	posn+=sprintf(buffer+posn,"\nTime: ");

	for(itr=0; itr<timeDescList.size(); itr++){
		timeDesc = (SDPTimeDescription*)timeDescList[itr];
		ret = timeDesc->toString(buffer+posn,len-posn);
		if(ret < 0)
			return -1;
		posn+=ret;
	}

	posn+=sprintf(buffer+posn,"\nMedia: ");

	for(itr=0; itr<mediaDescList.size(); itr++){
		mediaDesc = (SDPMediaDescription*)mediaDescList[itr];
		ret = mediaDesc->toString(buffer+posn,len-posn);
		if(ret < 0)
			return -1;
		posn+=ret;
	}

	buffer[posn++] = '\n';
	buffer[posn++] = '\0';
	return posn;
}

/********************************* PARSER *****************************/

int SDPHandler::read(char* bytes,int len, SDPPacket *packet) {
	int index;
	char ch;
	char prev;

	char* stkLine;
	char* stkField;
	char* strIp;
	char* line;

	char* strKey = NULL;
	char* strValue = NULL;

	if(packet->input) {
		free(packet->input);
		packet->input = NULL;
	}

	packet->input = (char*)malloc(len+1);
	printf("copying %d bytes\n\t", len); fflush(stdout);
	strncpy(packet->input, bytes, len);
	packet->input[len] = '\0';
	strIp = packet->input;

	SDPTimeDescription* timeDesc=NULL;
	SDPMediaDescription* mediaDesc=NULL;

	bool	bReqdDone = false;

	int		oldIndex=0;
	//int		indexMedia;
	//bool	bFound = false;
	bool	bMediaStarted=false;

	index = 0;
	prev = 'v';

	for(line = strtok_r(strIp,"\r\n",&stkLine);
		line;
		line = strtok_r(NULL,"\r\n",&stkLine)) {
		//debug(line);

		strKey = strtok_r(line,"=",&stkField);
		strValue = strtok_r(NULL,"=",&stkField);
		debug("\n%s : %s",strKey,strValue);

		if(strlen(strKey) > 1) {
			if(packet->input) {
				free(packet->input);
				packet->input = NULL;
			}
			return -1;			//SDP_ERR_UNKNOWN_KEY
		}
		ch = strKey[0];

		if(bMediaStarted) {
			index = lastIndexOf(SDPFields, ch);
			if(index < 0) {
				if(packet->input) {
					free(packet->input);
					packet->input = NULL;
				}
				return -2;		//SDP_ERR_INCORRECT_ORDER
			}
			if(index < oldIndex) {
				if(ch!='m') {
					if(packet->input) {
						free(packet->input);
						packet->input = NULL;
					}
					return -3;	//SDP_ERR_INCORRECT_ORDER
				}
			}
		}
		else {
			index = indexOf(SDPFields, ch);
			if((index < 0) || ( index < oldIndex)) {
				if(packet->input) {
					free(packet->input);
					packet->input = NULL;
				}
				return -4;		//SDP_ERR_INCORRECT_ORDER
			}
		}

		//Have we got all the required fields yet?
		if(!bReqdDone) {
			if(index > 2) {
				if(packet->input) {
					free(packet->input);
					packet->input = NULL;
				}
				debug("\nindex of %c =%d",ch,index);
				return -5;		//SDP_ERR_REQD_FIELD_MISSING
			}
			if(index==2)
				bReqdDone=true;
		}

		if(ch=='m') {
			bMediaStarted=true;
		}

		if((ch=='t')||(ch=='r')) {
			// Time-level info
			if(ch=='t') {
				timeDesc = new SDPTimeDescription();
				timeDesc->isAuto=true;
				packet->addTimeDescription(timeDesc);
				timeDesc->setField(ch, strValue);
				oldIndex=10;
			}
			if(ch=='r') {
				if (prev!='t') {
					if(packet->input) {
						free(packet->input);
						packet->input = NULL;
					}
					return -6;		//SDP_ERR_INCORRECT_TIME
				}
				timeDesc->setField(ch,strValue);
				oldIndex = 8;
			}
			prev=ch;
			continue;
		} else if((bMediaStarted)&&(indexOf(SDPMediaFields, ch) >= 0)) {
			//Media-level info
			if(ch=='m') {
				mediaDesc = new SDPMediaDescription();
				mediaDesc->isAuto=true;
				packet->addMediaDescription(mediaDesc);
			}
			mediaDesc->setField(ch,strValue);
		}
		else
			packet->setField(ch,strValue);	//Session-level info

		if(ch=='a')
			oldIndex = index-1;				//go back as multiple attribs
		else
			oldIndex = index;
		prev=ch;
	}

	if(packet->getNumTotalConnections() < 1) {
		return -7;							//SDP_ERR_NO_CONN_SPEC
	}
	return 0;
}

int writeTimeDesc(SDPConnectionInfo *sdpConnInfo, char* buffer, int len) {
		int posn=0;
		posn+=sprintf(buffer+posn,"c=%s %s %s\r\n",
			sdpConnInfo->networkType,
			sdpConnInfo->addressType,
			sdpConnInfo->address);
		if(sdpConnInfo->TTL)
			posn+=sprintf(buffer+posn,"/%d",sdpConnInfo->TTL);
		if(sdpConnInfo->numAddress)
			posn+=sprintf(buffer+posn,"/%d",sdpConnInfo->numAddress);
		return posn;
}

int SDPHandler::write(SDPPacket *pack, char* buffer, int len) {
	int	length;
	int ret;
	int itr;
	int itr2;
	char str[100];
	int posn = 0;
	SDPTimeDescription* sdpTimeDesc;
	SDPMediaDescription* sdpMediaDesc;
	SDPConnectionInfo* sdpConnInfo;

	if(!(pack->username
		 && pack->networkType
		 && pack->addressType
		 && pack->address
		 && pack->name))
		return -8;							//SDP_ERR_MALFORMED

	posn+=sprintf(buffer,
		"v=%d\r\no=%s %ld %ld %s %s %s\r\ns=%s\r\n",
		pack->getVersion(),
		pack->username,
		pack->sessionId,
		pack->ownerVersion,
		pack->networkType,
		pack->addressType,
		pack->address,
		pack->name);
	if(pack->info)
		posn+=sprintf(buffer+posn,"i=%s\r\n",pack->info);
	if(pack->uri)
		posn+=sprintf(buffer+posn,"u=%s\r\n",pack->uri);
	if(pack->email)
		posn+=sprintf(buffer+posn,"e=%s\r\n",pack->email);
	if(pack->phone)
		posn+=sprintf(buffer+posn,"p=%s\r\n",pack->phone);
	sdpConnInfo = pack->getConnectionInfo();

	if(sdpConnInfo) {
		posn+=writeTimeDesc(sdpConnInfo, buffer+posn, len-posn);
	}
	if(pack->bandwidth)
		posn+=sprintf(buffer+posn,"b=%s\r\n",pack->bandwidth);
	if(pack->timezoneAdj)
		posn+=sprintf(buffer+posn,"z=%s\r\n",pack->timezoneAdj);
	if(pack->encryptionKey)
		posn+=sprintf(buffer+posn,"k=%s\r\n",pack->encryptionKey);

	length = pack->getNumAttributes();
	for(itr=0;itr< length; itr++) {
		if(pack->getAttribute(str,100,itr) > 0);
			posn+=sprintf(buffer+posn,"a=%s\r\n",str);
	}

	length = pack->getNumTimeDescriptions();
	for(itr=0;itr< length; itr++) {
		sdpTimeDesc = pack->getTimeDescription(itr);
		if(sdpTimeDesc) {
			posn+=sprintf(buffer+posn,"t=%ld %ld\r\n",
				sdpTimeDesc->startTimeSec,
				sdpTimeDesc->stopTimeSec);
			posn+=sprintf(buffer+posn,"r=%ld %ld %ld",
				sdpTimeDesc->repeatIntervalSec,
				sdpTimeDesc->activeDurationSec,
				(long int)sdpTimeDesc->nOffsets);
			for(itr2=0; itr2 < sdpTimeDesc->nOffsets; itr2++) {
				posn+=sprintf(buffer+posn," %ld",
					sdpTimeDesc->offsets[itr2]);
			}
			if(sdpTimeDesc->nOffsets==0)
				posn+=sprintf(buffer+posn," %ld", (long int)0);
			posn+=sprintf(buffer+posn,"\r\n");
		}
	}

	length = pack->getNumMediaDescriptions();
	for(itr=0;itr< length; itr++) {
		sdpMediaDesc = pack->getMediaDescription(itr);
		if(sdpMediaDesc) {
			posn+=sprintf(buffer+posn,"m=%s %d",
				sdpMediaDesc->media,
				sdpMediaDesc->port);
			if(sdpMediaDesc->nPorts)
				posn+=sprintf(buffer+posn,"/%d",
					sdpMediaDesc->nPorts);
			posn+=sprintf(buffer+posn," %s ",
				sdpMediaDesc->transport);
			if(sdpMediaDesc->fmtListLen) {
				for(itr2=0;itr2 < sdpMediaDesc->fmtListLen; itr2++) {
					posn+=sprintf(buffer+posn," %d",
						sdpMediaDesc->fmtList[itr2]);
				}
			}
			else
				posn+=sprintf(buffer+posn," 0");
			posn+=sprintf(buffer+posn,"\r\n");

			if(sdpMediaDesc->title)
				posn+=sprintf(buffer+posn,"i=%s\r\n",sdpMediaDesc->title);

			sdpConnInfo = sdpMediaDesc->getConnectionInfo();

			if(sdpConnInfo) {
				posn+=writeTimeDesc(sdpConnInfo,
					buffer+posn, len-posn);
			}

			if(sdpMediaDesc->bandwidth)
				posn+=sprintf(buffer+posn,"b=%s\r\n",
					sdpMediaDesc->bandwidth);
			if(pack->encryptionKey)
				posn+=sprintf(buffer+posn,"k=%s\r\n",
					sdpMediaDesc->encryptionKey);

			ret = sdpMediaDesc->getNumAttributes();
			for(itr2=0;itr2 < ret; itr2++) {
				if(sdpMediaDesc->getAttribute(str,100, itr2) > 0)
					posn+=sprintf(buffer+posn,"a=%s\r\n",str);
			}
		}
	}
	return posn;
}
