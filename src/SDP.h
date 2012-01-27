#ifndef __SDP_H
#define __SDP_H

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

typedef std::vector<void*> Vector;

#define indexOf(STR,CH) 	(int)(strchr(STR,CH) - STR)
#define lastIndexOf(STR,CH) (int)(strrchr(STR,CH) - STR)

class SDPConnectionInfo {
public:
	bool		isAuto;
	char*		inputString;
	char*		networkType;
	char*		addressType;
	char*		address;
	int			addressLen;
	unsigned long	ipAddress;

	int			TTL;			//for multicast address
	int			numAddress;

	SDPConnectionInfo(void);
	~SDPConnectionInfo(void);
	int 		setConnectionInfo(char *str);
	int 		toString(char* buffer, int len);
};


class SDPMediaDescription {
//Media level info
private:
	Vector	attribsList;
	SDPConnectionInfo *sdpConnInfo;

public:
	bool	isAuto;
	char*	media;
	int		port;
	int		nPorts;
	char* 	transport;
	int*	fmtList;
	int		fmtListLen;

	char*	title;
	char*	connInfo;
	char*	bandwidth;
	char* 	encryptionKey;


	SDPMediaDescription();
	~SDPMediaDescription();

	int 	setField(char id, char* str);
	void 	addAttribute(char* str);
	int		getNumAttributes(void);
	int		getAttribute(char *buff, unsigned int len, unsigned int index);
	int 	setConnectionInfo(char* str);
	char* 	getConnectionString(void);
	SDPConnectionInfo* getConnectionInfo(void);

	int toString(char* buffer, int len);
};

class SDPTimeDescription {
public:
	bool		isAuto;
	//public static final double NTP_2_UNIX_DIFF = -22089888;
//Time information
	char*	time;
	char*	reps;

	long	startTimeSec;
	long	stopTimeSec;

	long	ntpStartTimeSec;
	long	ntpStopTimeSec;

	long	repeatIntervalSec;
	long 	activeDurationSec;
	long*	offsets;
	int		nOffsets;

	SDPTimeDescription();
	~SDPTimeDescription();

	static int isTimeField(char c);
	static long parseTime(char* str);
	int setField(char id, char* str);
	int setFields(char* strTime, char* strReps);

	int toString(char* buffer, int len);
};


class SDPPacket {
private:
	int 	length;
	int 	nConnInfo;

//Session level info
	int 	version;
	char* 	owner;

//connection info
	char*	connInfo;
	SDPConnectionInfo* 	sdpConnInfo;

	Vector	attribsList;		//Attribs list
	Vector	timeDescList; 	//Time description list
	Vector	mediaDescList;	//Media description list

//owner info
public:
	char*	username;
	char*	sessionIdStr;
	long 	sessionId;
	char*	ownerVersionStr;
	long	ownerVersion;
	char*	networkType;
	char*	address;
	char*	addressType;
	int		addressLen;
	unsigned long	ipAddress;

	//session info
	char*	name;
	char* 	info;
	char*	uri;
	char*	email;
	char* 	phone;

	char*	bandwidth;

	char* 	timezoneAdj;
	char* 	encryptionKey;


	char*	input;

	SDPPacket();
	~SDPPacket();

	void 	reset(void);
	int		getLength(void);

	int 	setVersion(char* str);
	int 	setVersion(int ver);
	int 	getVersion(void);

	void 	setOwner(char* str);
	char* 	getOwner(void);

	int 	setField(char id, char* str);

	int 	setConnection(char* str);
	int 	setConnectionInfo(SDPConnectionInfo* sdpConnInfo);
	char* 	getConnectionString(void);
	SDPConnectionInfo* getConnectionInfo(void);
	int 	getNumTotalConnections(void);

	void 	addMediaDescription(SDPMediaDescription* desc);
	int 	getNumMediaDescriptions(void);
	SDPMediaDescription* getMediaDescription(unsigned int index);

	void 	addTimeDescription(SDPTimeDescription *desc);
	int 	getNumTimeDescriptions();
	SDPTimeDescription* getTimeDescription(unsigned int index);

	void 	addAttribute(char* str);
	int  	getNumAttributes(void);
	int  	getAttribute(char *buff, unsigned int len, unsigned int index);
	//char* getAttribute(int index);

	int toString(char* buffer, int len);
};

class SDPHandler {
public:
	static SDPPacket* parse(char* bytes, int len);
	static int read(char* bytes,int len, SDPPacket* packet);
	static int write(SDPPacket* packet, char* bytes,int len);
};

#endif