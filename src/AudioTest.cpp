#include <stdio.h>

#include "AudioRW.h"
#include "AdpcmCodec.h"
#include "Common.h"
#include "AudioTest.h"

AudioIOTest::AudioIOTest (  Common *common, AudioReader *reader, 
                            AudioWriter *writer, AdpcmCodec *codec, bool tc) 
            :Thread(10, 0, common) {
    audioReader = reader;
    audioWriter = writer;
    audioCodec  = codec;
    running     = false;
    testCodec   = tc;
}

AudioIOTest::~AudioIOTest() { }

void* AudioIOTest::run(void *arg) {
    Common *common = (Common *)arg;
    running = true;
    printf("Starting audio test...\n"); fflush(stdout);

    //start reader and writer
    common->audioGo   = true;
    common->codecStop = true;
    audioCodec->join(NULL); //wait for codec thread to stop, if it hasn't already
    
    sleep(2); //let it buffer for 2 seconds
    while(running) {
        //read from 
        Memo *memo = (Memo*)common->codecQ->waitForData(20);
        if (memo) {
            if (testCodec) {
                audioCodec->encode(memo, memo);
                audioCodec->decode(memo, memo);
            }
            memo->code = VIVOCE_MSG_PLAY;
            common->playQ->signalData(memo);
        }
        else {
            //printf("\rno memo, queue size %d...", common->codecQ->count()); fflush(stdout);
        }
    }
    printf("Stopping audio test...\n"); fflush(stdout);
    
    //stop reader and write
    common->audioGo   = false;
    common->codecStop = false;
    audioCodec->start();    //start it again    
    return NULL;
}
