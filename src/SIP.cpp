#include "SIP.h"


#define CMP strcasecmp
#define debug //printf

/*********************  CONVENIENCE METHODS *************************/
void parseURI(URI* uri, char* line) {
	char *ptr;
	uri->id = strtok_r(line,"@",&ptr);
	uri->host = strtok_r(NULL,"@",&ptr);
}

const char* getRequestString(int code) {
	switch(code) {
		case SIP_METHOD_INV:
			return "INVITE";
		case SIP_METHOD_ACK:
			return "ACK";
		case SIP_METHOD_BYE:
			return "BYE";
		case SIP_METHOD_OPT:
			return "OPTIONS";
		case SIP_METHOD_REG:
			return "REGISTER";
		case SIP_METHOD_CNL:
			return "CANCEL";
		default:
			return "";
	}
}


const char* getResponseString(int code) {
	switch(code) {
		case 180:
			return "RINGING";
		case 200:
			return "OK";
		case 486:
			return "BUSY";
		case 501:
			return "UNIMPLEMENTED";
		default:
			return "";
	}
}

/******************************** VIA *******************************/
VIA::VIA() {
	isAuto=false;
	transport=NULL;
	host=NULL;
	port=0;
	branch=NULL;
}

/****************************** PACKET ******************************/
SIPPacket::SIPPacket() {
	method = 0;
	version = 2.0;

	isRequest=false;
	requestCode = 0;
	requestString = NULL;
	responseCode = 0;
	responseString = NULL;

	fromName = NULL;
	fromTag = NULL;

	toName = NULL;
	toTag = NULL;

	callId=0;
	callIdHost = NULL;

	cSeqCode = 0;
	cSeqMethod = NULL;

	contactTransport = NULL;

	contentType = NULL;
	contentLength = 0;
	contentData = NULL;

	subject  = NULL;

	input = NULL;
}

/**
 * Destructor
 */
SIPPacket::~SIPPacket() {
	reset();
}

void SIPPacket::reset(void) {
	VIA* via;

	for(unsigned int itr=0; itr < viaList.size(); itr++){
		via = (VIA*)viaList[itr];
		if(via->isAuto)
			delete via;
	}
	viaList.clear();

	if(input)
		free(input);
	input = NULL;

	isRequest=false;
}

int	 SIPPacket::addViaField(VIA* via) {
	viaList.push_back(via);
	return 0;
}

unsigned int SIPPacket::getNumViaFields(void) {
	return viaList.size();
}

VIA* SIPPacket::getViaField(unsigned int index) {
	if(index >= viaList.size())
		return NULL;
	return (VIA*)viaList[index];
}

int SIPHandler::read(char* bytes,int len, SIPPacket* pack) {
	char* stkLine = NULL;
	char* stkKey = NULL;
	char* stkField = NULL;
	char* stkSubField = NULL;
	char* strIp = NULL;
	char* line = NULL;

	char* strKey = NULL;
	char* str1 = NULL;
	char* str2 = NULL;
	VIA*  via  = NULL;

	pack->isRequest=false;

	if(pack->input) {
		free(pack->input);
		pack->input = NULL;
	}

	pack->input = (char*)malloc(len+1);
	strcpy(pack->input,bytes);
	pack->input[len]='\0';
	strIp = pack->input;

	//first line
	line = strtok_r(strIp,"\r\n",&stkLine);
	pack->isRequest = strncmp("SIP/2.0",line,7);
	if(pack->isRequest) {
		pack->requestString = strtok_r(line," ",&stkField);
		if(!CMP("INVITE",pack->requestString)) {
			pack->requestCode = SIP_METHOD_INV;
		}
		else if(!CMP("ACK",pack->requestString)) {
			pack->requestCode = SIP_METHOD_ACK;
		}
		else if(!CMP("BYE",pack->requestString)) {
			pack->requestCode = SIP_METHOD_BYE;
		}
		else if(!CMP("OPTIONS",pack->requestString)) {
			pack->requestCode = SIP_METHOD_OPT;
		}
		else if(!CMP("REGISTER",pack->requestString)) {
			pack->requestCode = SIP_METHOD_REG;
		}
		else if(!CMP("CANCEL",pack->requestString)) {
			pack->requestCode = SIP_METHOD_CNL;
		}
		else {
			pack->requestCode = 0;
			return SIP_ERR_UNKNOWN_METHOD;
		}

		str1 = strtok_r(NULL," ",&stkField);
		if(str1) {
			str2 = strtok_r(NULL," ",&stkField);
			if(strncasecmp("sip:",str1,4) || strncmp("SIP/2.0",str2,7))
				return SIP_ERR_MALFORMED_PACK;

			str1 = strtok_r(str1,":",&stkField);
			str1 = strtok_r(NULL,":",&stkField);
			parseURI(&pack->requestUri, str1);
		}
		else
			return SIP_ERR_MALFORMED_PACK;
	}
	else {
		strKey = strtok_r(line," ",&stkField);
		str1 = strtok_r(NULL," ",&stkField);
		pack->responseCode = atoi(str1);
		pack->responseString = strtok_r(NULL," ",&stkField);
	}

	//further lines
	for(;line; line = strtok_r(NULL,"\r\n",&stkLine)) {
		debug("\ndbg: %s",line);
		strKey = strtok_r(line,":",&stkKey);
		if(!CMP("Via",strKey)) {
			//Via: SIP/2.0/TCP client.atlanta.example.com:5060;branch=z9hG4bK74bf9
			str1 = strtok_r(NULL,":",&stkKey);
			if(!str1)
				continue;
			via = new VIA();
			via->isAuto=true;
			str1 = strtok_r(str1," ",&stkSubField);
			if(strncmp(str1,"SIP/2.0",7))
				return SIP_ERR_MALFORMED_PACK;
			via->host = strtok_r(NULL," ",&stkSubField);

			str1 = strtok_r(str1,"/",&stkSubField);
			str1 = strtok_r(NULL,"/",&stkSubField);
			via->transport = strtok_r(NULL,"/",&stkSubField);
			str2 = strtok_r(NULL,":",&stkKey);
			str1 = strtok_r(str2,";",&stkSubField);
			via->port = atoi(str1);

			str1 = strtok_r(NULL,";",&stkSubField);
			str1 = strtok_r(str1,"=",&stkField);
			if(CMP("branch",str1))
				return SIP_ERR_UNKNOWN_FIELD;
			via->branch = strtok_r(NULL,"=",&stkField);
			pack->addViaField(via);
		}
		else if(!CMP("From",strKey)) {
			//From: Alice <sip:alice@atlanta.example.com>;tag=9fxced76sl
			pack->fromName = strtok_r(stkKey," ",&stkField);
			if(!pack->fromName)
				continue;
			if(pack->fromName[0]=='<') {
				str1 = strtok_r(pack->fromName," ",&stkField);
				stkField=pack->fromName;
				pack->fromName=NULL;
			}
			else {
				str1 = strtok_r(NULL," ",&stkField);
			}
			str2 = strtok_r(str1,";",&stkField);
			str2 = strtok_r(str2,"<>",&stkSubField);
			str2 = strtok_r(str2,":",&stkSubField);
			if(CMP("sip",str2))
				return SIP_ERR_UNKNOWN_URI;
			str2 = strtok_r(NULL,":",&stkSubField);
			parseURI(&pack->fromUri, str2);

			str1 = strtok_r(NULL,";",&stkField);
			if(str1) {
				str2 = strtok_r(str1,"=",&stkSubField);
				if(CMP("tag",str2))
					return SIP_ERR_UNKNOWN_FIELD;
				pack->fromTag = strtok_r(NULL,"=",&stkSubField);
			}
		}
		else if(!CMP("To",strKey)) {
			//To: Bob <sip:bob@biloxi.example.com>;tag=8321234356
			pack->toName = strtok_r(stkKey," ",&stkField);
			if(!pack->toName)
				continue;
			if(pack->toName[0]=='<') {
				str1 = strtok_r(pack->toName," ",&stkField);
				stkField=pack->toName;
				pack->toName=NULL;
			}
			else {
				str1 = strtok_r(NULL," ",&stkField);
			}
			str2 = strtok_r(str1,";",&stkField);
			str2 = strtok_r(str2,"<>",&stkSubField);
			str2 = strtok_r(str2,":",&stkSubField);
			if(CMP("sip",str2))
				return SIP_ERR_UNKNOWN_URI;
			str2 = strtok_r(NULL,":",&stkSubField);
			parseURI(&pack->toUri, str2);

			str1 = strtok_r(NULL,";",&stkField);
			if(str1) {
				str2 = strtok_r(str1,"=",&stkSubField);
				if(CMP("tag",str2))
					return SIP_ERR_UNKNOWN_FIELD;
				pack->toTag = strtok_r(NULL,"=",&stkSubField);
			}
		}
		else if(!CMP("CALL-ID",strKey)) {
		    //Call-ID: 3848276298220188511@atlanta.example.com
			str1 = strtok_r(NULL,":",&stkKey);
			if(!str1)
				continue;
		    URI uri;
		    parseURI(&uri, str1);
		    pack->callId = strtod(uri.id, NULL);
		    pack->callIdHost = uri.host;
		}
		else if(!CMP("CSeq",strKey)) {
		    //CSeq: 1 INVITE
			str1 = strtok_r(NULL,":",&stkKey);
			if(!str1)
				continue;
		    str1 = strtok_r(str1," ",&stkField);
		    pack->cSeqCode = atoi(str1);
		    pack->cSeqMethod = strtok_r(NULL," ",&stkField);
		}
		else if(!CMP("Contact",strKey)) {
		    //Contact: <sip:alice@client.atlanta.example.com;transport=tcp>
			str1 = strtok_r(stkKey,"<>",&stkField);
			str1 = strtok_r(NULL,";",&stkField);
			str2 = strtok_r(str1,":",&stkSubField);
			if(CMP("sip",str2))
				return SIP_ERR_UNKNOWN_URI;
			str2 = strtok_r(NULL,":",&stkSubField);
			parseURI(&pack->contactUri, str2);

			str2 = strtok_r(NULL,";>",&stkField);
			if(str2) {
				str2 = strtok_r(str2,"=",&stkSubField);
				if(CMP("transport",str2))
					return SIP_ERR_UNKNOWN_FIELD;
				pack->contactTransport = strtok_r(NULL,"=",&stkSubField);
			}
		}
		else if(!CMP("Content-Type",strKey)) {
			//Content-Type: application/sdp
			str1 = strtok_r(NULL,":",&stkKey);
			if(!str1)
				continue;
			pack->contentType = str1;
		}
		else if(!CMP("Content-Length",strKey)) {
			//Content-Length: 151
			str1 = strtok_r(NULL,":",&stkKey);
			if(!str1)
				continue;
			pack->contentLength = atoi(str1);
			if(pack->contentLength) {
				pack->contentData = stkLine;
				break;
			}
		}
		else {
			//unsupported field
			continue;
						//return SIP_ERR_MALFORMED_PACK;
		}
	}
	return 0;
}

int SIPHandler::write(SIPPacket *pack, char* buffer, int len) {
	unsigned int	length;
	unsigned int itr;
	int posn = 0;
	VIA* via = NULL;

	if(len < 1000)
		return -1;

	if(pack->isRequest) {
		if( !pack->requestUri.id || !pack->requestUri.host) {
			pack->requestUri.id = pack->toUri.id;
			pack->requestUri.host = pack->toUri.host;
		}
		posn+=sprintf(buffer,"%s sip:%s@%s SIP/2.0\r\n",
			getRequestString(pack->requestCode),
			pack->requestUri.id,
			pack->requestUri.host);
	}
	else {
		posn+=sprintf(buffer,"SIP/2.0 %d %s\r\n",
			pack->responseCode,
			getResponseString(pack->responseCode));
	}

	length = pack->getNumViaFields();
	for(itr=0;itr< length; itr++) {
		via = pack->getViaField(itr);
		if(via) {
			posn+=sprintf(buffer+posn,"Via: SIP/2.0/%s %s:%d",
				via->transport,
				via->host,
				via->port);
			if(via->branch)
				posn+=sprintf(buffer+posn,";branch=%s%s",
					SIP_BRANCH_START,
					via->branch);
			posn+=sprintf(buffer+posn,"\r\n");
		}
	}
	posn+=sprintf(buffer+posn,"From: %s <sip:%s@%s>",
		(pack->fromName?pack->fromName:""),
		pack->fromUri.id,
		pack->fromUri.host);
	if(pack->fromTag)
		posn+=sprintf(buffer+posn,";tag=%s",
			pack->fromTag);
	posn+=sprintf(buffer+posn,"\r\n");
	posn+=sprintf(buffer+posn,"To: %s <sip:%s@%s>",
		(pack->toName?pack->toName:""),
		pack->toUri.id,
		pack->toUri.host);
	if(pack->toTag)
		posn+=sprintf(buffer+posn,";tag=%s",
			pack->toTag);
	posn+=sprintf(buffer+posn,"\r\n");

	posn+=sprintf(buffer+posn,"CALL-ID: %.0f@%s\r\nCSeq: %d %s\r\n",
		pack->callId,
		pack->callIdHost,
		pack->cSeqCode,
		pack->cSeqMethod);
	posn+=sprintf(buffer+posn,"Contact: <sip:%s@%s;transport=%s>\r\n",
		pack->contactUri.id,
		pack->contactUri.host,
		pack->contactTransport);

	if(pack->contentType)
		posn+=sprintf(buffer+posn,"Content-Type: %s\r\n",
			pack->contentType);
	posn+=sprintf(buffer+posn,"Content-Length: %d\r\n\r\n",
		pack->contentLength);
	if(pack->contentLength) {
		memcpy(buffer+posn,pack->contentData,pack->contentLength);
		posn+=pack->contentLength;
	}
	buffer[posn++] = '\0';
	return posn;
}