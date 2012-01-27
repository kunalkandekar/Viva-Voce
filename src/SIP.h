#ifndef __SIP_H
#define __SIP_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <vector>
#include <ctype.h>
#include <stdio.h>

#define SIP_USERAGENT_PORT 5060

#define SIP_TIMEOUT_I_T1_MS 1000
#define SIP_TIMEOUT_I_LIMIT 7

#define SIP_TIMEOUT_O_T1_MS 500
#define SIP_TIMEOUT_O_T2_MS 4000
#define SIP_TIMEOUT_O_LIMIT 11

#define SIP_METHOD_INV			10
#define SIP_METHOD_ACK			11
#define SIP_METHOD_BYE			12

#define SIP_METHOD_OPT			13
#define SIP_METHOD_REG			14
#define SIP_METHOD_CNL			15

#define SIP_RESPONSE_OK			200
#define SIP_RESPONSE_RING		180
#define SIP_RESPONSE_BUSY		486
#define SIP_RESPONSE_UNIMPL		501
#define SIP_RESPONSE_DECLINE	603

#define SIP_ERR_UNSUPPORTED		-25
#define SIP_ERR_UNKNOWN_METHOD	-26
#define SIP_ERR_UNKNOWN_FIELD	-27
#define SIP_ERR_UNKNOWN_URI		-28
#define SIP_ERR_MALFORMED_PACK  -29

#define SIP_BRANCH_START "z9hG4bK"

#define SIP_TIMEOUT_I_T1_MS 1000
#define SIP_TIMEOUT_I_LIMIT 7

#define SIP_TIMEOUT_O_T1_MS 500
#define SIP_TIMEOUT_O_T2_MS 4000
#define SIP_TIMEOUT_O_LIMIT 11

class URI {
public:
	char* id;
	char* host;
};

class VIA {
public:
	VIA();
	bool	isAuto;
	char*	transport;
	char*	host;
	int		port;
	char*	branch;
};

typedef std::vector<VIA*> VIAList;

class SIPPacket {
public:
	int		method;
	float	version;

	bool	isRequest;
	int		requestCode;
	char*	requestString;
	int		responseCode;
	char*	responseString;

	VIAList	viaList;


	URI		requestUri;

	char*	fromName;
	URI		fromUri;
	char*	fromTag;

	char*	toName;
	URI		toUri;
	char*	toTag;

	double	callId;
	char*	callIdHost;

	int		cSeqCode;
	char*	cSeqMethod;

	URI		contactUri;
	char*	contactTransport;

	char*	contentType;
	int		contentLength;
	char*	contentData;

	char* 	subject;

	char*	input;

	SIPPacket();
	~SIPPacket();

	void 	reset(void);

	int		     addViaField(VIA* via);
	unsigned int getNumViaFields(void);
	VIA*         getViaField(unsigned int index);
};

class SIPHandler {
public:
	static SIPPacket* parse(char* bytes, int len);
	static int read(char* bytes,int len, SIPPacket* packet);
	static int write(SIPPacket* packet, char* bytes,int len);
};

#endif