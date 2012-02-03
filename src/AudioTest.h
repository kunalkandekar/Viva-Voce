#ifndef __AUDIOTEST_H
#define __AUDIOTEST_H

#include "Thread.h"
//forward declarations
class Common;
class AudioReader;
class AudioWriter;
class AdpcmCodec;

class AudioIOTest : public Thread {
public:
    AudioIOTest(Common *common, AudioReader *reader, AudioWriter *writer, AdpcmCodec *codec, bool testCodec, int f);
    
    virtual ~AudioIOTest();
    
    AudioReader *audioReader;
    AudioWriter *audioWriter;
    AdpcmCodec  *audioCodec;
    bool running;
    bool testCodec;
    int  flags;
    
    void* run(void *arg);
};

#endif
