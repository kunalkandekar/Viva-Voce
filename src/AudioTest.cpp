#include <stdio.h>

#include "AudioRW.h"
#include "AdpcmCodec.h"
#include "Common.h"
#include "AudioTest.h"

#include <iostream>
#include <fstream>


AudioIOTest::AudioIOTest (  Common *common, AudioReader *reader, 
                            AudioWriter *writer, AdpcmCodec *codec, bool tc, int f) 
            :Thread(10, 0, common) {
    audioReader = reader;
    audioWriter = writer;
    audioCodec  = codec;
    running     = false;
    testCodec   = tc;
    flags       = f;
}

AudioIOTest::~AudioIOTest() { }

void* AudioIOTest::run(void *arg) {
    Common *common = (Common *)arg;
    running = true;
    int delaySec = 2;
    printf("Starting audio test: Replaying recorded audio with %d delay. Use microphones!\n", delaySec); fflush(stdout);
    
    int dump_bytes       = flags & 1;
    common->audioGenTone = flags & 2;
    
	std::ofstream *dumpf = NULL;
	if(dump_bytes) {
	   dumpf = new std::ofstream("./dump.dat", std::ios::out | std::ios::trunc | std::ios::binary);
	}

    //start reader and writer
    common->audioGo   = true;
    common->codecStop = true;
    audioCodec->join(NULL); //wait for codec thread to stop, if it hasn't already

    sleep(delaySec); //let it buffer for 2 seconds
    while(running) {
        //read from 
        Memo *memo = (Memo*)common->codecQ->waitForData(20);

        if (memo) {
            if (testCodec) {
                audioCodec->encode(memo, memo);
                audioCodec->decode(memo, memo);
            }

            //dump to file for acoustic echo cancellation test
            if(dump_bytes) { 
                dumpf->write(memo->bytes, memo->data1);
                dumpf->flush();
            }

            //queue for playback            
            memo->code = VIVOCE_MSG_PLAY;
            common->playQ->signalData(memo);
        }
    }
    printf("Stopping audio test...\n"); fflush(stdout);
    
    //stop reader and write
    common->audioGo      = 0;
    common->codecStop    = 0;
    common->audioGenTone = 0;
    
    if(dump_bytes) {
        dumpf->close();
        delete dumpf;
    }
    
    audioCodec->start();    //start it again
    return NULL;
}
