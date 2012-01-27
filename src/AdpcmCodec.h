#ifndef __ADPCMCODEC_H
#define __ADPCMCODEC_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <vector>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

//infra-structure
#include "Common.h"
#include "Thread.h"

class AdpcmCodec:public Thread {
	Common *common;
	long	msLastChecked;
	long	msCheckThreshold;
	int   	detectSilence(char *bytes, int offset, int len);
	unsigned short 	*pcmSamples;   //internal pcm buffer, shared for encoding and decoding

public:
	AdpcmCodec(Common* common);
	virtual ~AdpcmCodec();

    int encode(Memo *memoIn, Memo *memoOut);
    int decode(Memo *memoIn, Memo *memoOut);

	void* run(void* arg);
};
#endif
