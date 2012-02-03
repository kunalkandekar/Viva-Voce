#ifndef __OSXCOREAUDIOIO_H
#define __OSXCOREAUDIOIO_H
/*
Adapted from File:DCAudioFileRecorder.h
*/ 

#include <Carbon/Carbon.h>
#include <AudioToolbox/AudioToolbox.h>

#include "Thread.h"

//Adapted from examples on:
//http://developer.apple.com/library/ios/#documentation/MusicAudio/Conceptual/AudioQueueProgrammingGuide/AQRecord/RecordingAudio.html#//apple_ref/doc/uid/TP40005343-CH4-SW1

#define kNumberBuffers 3

class CoreAudioIOBase {
public:
    CoreAudioIOBase();
    virtual ~CoreAudioIOBase();

    // Convenience function to set data format
    int ConfigureAudioFormat(int nChannels, int sampleRate, int bitsPerSample, const char *nCoding, int bufferSize);

    // Convenience function to allocate our audio buffers
    int AllocateAudioBuffers();

    // Convenience function to dispose of our audio buffers
    int DestroyAudioBuffers();        

    virtual int ConfigureAudio(int nChannels, int sampleRate, int bitsPerSample, const char *nCoding, int bufferSize) = 0;
    
    virtual int Start(int fd) = 0;
    virtual int Stop() = 0;

//member variables
    AudioStreamBasicDescription  mDataFormat;
    AudioQueueRef                mQueue;
    AudioQueueBufferRef          mBuffers[kNumberBuffers];
    UInt32                       bufferByteSize;
    SInt64                       mCurrentPacket;
    UInt32                       mCurrentByte;
    bool                         mIsRunning;
    int pipefd;

};

class CoreAudioInput : public CoreAudioIOBase {
public:
    CoreAudioInput();
    
    ~CoreAudioInput();
    
    UInt32 DeriveIPBufferSize ( AudioQueueRef                audioQueue,
                                AudioStreamBasicDescription  &ASBDescription,
                                Float64                      seconds,
                                UInt32                       *outBufferSize);

    static void HandleInputBuffer ( void                                *aqData,
                                    AudioQueueRef                       inAQ,
                                    AudioQueueBufferRef                 inBuffer,
                                    const AudioTimeStamp                *inStartTime,
                                    UInt32                              inNumPackets,
                                    const AudioStreamPacketDescription  *inPacketDesc);

    int EnqueueBuffers();
    
    int ConfigureAudio(int nChannels, int sampleRate, int bitsPerSample, const char *nCoding, int bufferSize);
    int Start(int fd);
    int Stop();
};

class CoreAudioOutput : public CoreAudioIOBase, Thread {
public:
    CoreAudioOutput();
    
    ~CoreAudioOutput();
    
    UInt32 DeriveOPBufferSize ( AudioStreamBasicDescription &ASBDesc,
                                UInt32                      maxPacketSize,
                                Float64                     seconds,
                                UInt32                      *outBufferSize,
                                UInt32                      *outNumPacketsToRead);

    static void HandleOutputBuffer (void                 *aqData,
                                    AudioQueueRef        inAQ,
                                    AudioQueueBufferRef  inBuffer);
    int PrimeBuffers();

    int ConfigureAudio(int nChannels, int sampleRate, int bitsPerSample, const char *nCoding, int bufferSize);
    int Start(int fd);
    int Stop();

    void *run(void *arg);
//member variables
    UInt32 mNumPacketsToRead;
    AudioStreamPacketDescription  *mPacketDescs;
};

#endif