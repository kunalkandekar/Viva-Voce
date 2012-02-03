/*
Adapted from File:DCAudioFileRecorder.cpp
*/ 

#include "OSXCoreAudioIO.h"
#include <time.h>

#define OSX_COREAUDIO_DEBUG 0

//Base class impl
CoreAudioIOBase::CoreAudioIOBase() {
    bufferByteSize = -1;
    mCurrentPacket = 0;
    mCurrentByte   = 0;
    mIsRunning     = false;
    pipefd         = -1;
    signal(SIGPIPE, SIG_IGN); //we will have broken pipes
}

CoreAudioIOBase::~CoreAudioIOBase() {
}

int CoreAudioIOBase::ConfigureAudioFormat(int nChannels, int sampleRate, int bitsPerSample, const char *nCoding, int bufferSize) {
    // Twiddle the format to our liking
    mDataFormat.mSampleRate = sampleRate;
    mDataFormat.mChannelsPerFrame = nChannels;
    //mDataFormat.mFormatID = kAudioFormatLinearPCM;
    if(!strncmp(nCoding, "alaw", 4)) {
	    mDataFormat.mFormatID       = kAudioFormatALaw;
        mDataFormat.mBitsPerChannel = 8;
        mDataFormat.mFormatFlags    = 0;
        mDataFormat.mBytesPerFrame  = mDataFormat.mChannelsPerFrame;
        mDataFormat.mBytesPerPacket = mDataFormat.mChannelsPerFrame;
	}
	else if(!strncmp(nCoding, "pcm", 3)) {
	    mDataFormat.mFormatID       = kAudioFormatLinearPCM;
        mDataFormat.mFormatFlags    = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        mDataFormat.mBitsPerChannel = 16;
        mDataFormat.mFormatFlags    = 0;
        mDataFormat.mBytesPerFrame  = mDataFormat.mChannelsPerFrame * 2;
        mDataFormat.mBytesPerPacket = mDataFormat.mChannelsPerFrame * 2;
	}
	else if(!strncmp(nCoding, "u8", 2)) {
	    mDataFormat.mFormatID       = kAudioFormatLinearPCM;
        mDataFormat.mFormatFlags    = kAudioFormatFlagIsPacked;
        mDataFormat.mBitsPerChannel = 8;
        mDataFormat.mBytesPerFrame  = mDataFormat.mChannelsPerFrame;
        mDataFormat.mBytesPerPacket = mDataFormat.mChannelsPerFrame;
	}
	else if(!strncmp(nCoding, "adpcm", 5)) {
        mDataFormat.mFormatID       = kAudioFormatAppleIMA4;
        mDataFormat.mFormatFlags    = 0;            
        mDataFormat.mBitsPerChannel = 0;
        mDataFormat.mBytesPerFrame  = mDataFormat.mChannelsPerFrame;
        mDataFormat.mBytesPerPacket = mDataFormat.mChannelsPerFrame * 34;
	}
	else { //default is ULAW
	    mDataFormat.mFormatID       = kAudioFormatULaw;
        mDataFormat.mBitsPerChannel = 8;
        mDataFormat.mFormatFlags    = 0;
        mDataFormat.mBytesPerFrame  = mDataFormat.mChannelsPerFrame;
        mDataFormat.mBytesPerPacket = mDataFormat.mChannelsPerFrame;
	}
    
    //mDataFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
    if (mDataFormat.mFormatID == kAudioFormatLinearPCM && nChannels == 1)
        mDataFormat.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
//#if __BIG_ENDIAN__
    mDataFormat.mFormatFlags |= kAudioFormatFlagIsBigEndian;
//#endif
    if(mDataFormat.mFormatID  != kAudioFormatAppleIMA4) {
        //mDataFormat.mBitsPerChannel = sizeof(Float32) * 8;
        //mDataFormat.mBytesPerFrame = mDataFormat.mBitsPerChannel / 8;
        mDataFormat.mFramesPerPacket = 1;
        mDataFormat.mBytesPerPacket = mDataFormat.mBytesPerFrame;
    }
    bufferByteSize = bufferSize;
    return 0;
}

// Convenience function to dispose of our audio buffers
int CoreAudioIOBase::DestroyAudioBuffers() {
    //not currently needed with AudioQueuDispose
    return 0;
}
    
// Convenience function to allocate our audio buffers
int CoreAudioIOBase::AllocateAudioBuffers() {
    if (bufferByteSize < 0)
        return -1;

    for (int i = 0; i < kNumberBuffers; ++i) {
        AudioQueueAllocateBuffer ( mQueue,
                                bufferByteSize,
                                &mBuffers[i]);
    }
    return 0;
}

//End base class impl

CoreAudioInput::CoreAudioInput() : CoreAudioIOBase() {
}

CoreAudioInput::~CoreAudioInput() {
    // Stop pulling audio data
    Stop();
}

UInt32 CoreAudioInput::DeriveIPBufferSize ( AudioQueueRef                audioQueue,
                                            AudioStreamBasicDescription  &ASBDescription,
                                            Float64                      seconds,
                                            UInt32                       *outBufferSize) {
    static const int maxBufferSize = 0x400;
 
    int maxPacketSize = ASBDescription.mBytesPerPacket;
    if (maxPacketSize == 0) {
        UInt32 maxVBRPacketSize = sizeof(maxPacketSize);
        AudioQueueGetProperty (
                audioQueue,
                kAudioQueueProperty_MaximumOutputPacketSize,
                // in Mac OS X v10.5, instead use
                //   kAudioConverterPropertyMaximumOutputPacketSize
                &maxPacketSize,
                &maxVBRPacketSize
        );
    }
 
    Float64 numBytesForTime =
        ASBDescription.mSampleRate * maxPacketSize * seconds;
    *outBufferSize =
    UInt32 (numBytesForTime < maxBufferSize ?
        numBytesForTime : maxBufferSize);
    return 0;
}

void CoreAudioInput::HandleInputBuffer( void                                *aqData,
                                        AudioQueueRef                       inAQ,
                                        AudioQueueBufferRef                 inBuffer,
                                        const AudioTimeStamp                *inStartTime,
                                        UInt32                              inNumPackets,
                                        const AudioStreamPacketDescription  *inPacketDesc) {
    CoreAudioInput *pAqData = (CoreAudioInput *) aqData;
    //if (pAqData->mIsRunning == false) return;
 
    if (inNumPackets == 0 && 
          pAqData->mDataFormat.mBytesPerPacket != 0)
       inNumPackets =
           inBuffer->mAudioDataByteSize / pAqData->mDataFormat.mBytesPerPacket;

    int remaining = inBuffer->mAudioDataByteSize;
    int written = 0;
    while (remaining > 0) { 
        int bytesWrit = write(pAqData->pipefd, ((char*)inBuffer->mAudioData) + written, remaining);
        if (bytesWrit >= 0) {
		    if (OSX_COREAUDIO_DEBUG) { printf("\n\tHandleInputBuffer wrote %d bytes", bytesWrit); fflush(stdout); }
            pAqData->mCurrentPacket += inNumPackets;
            pAqData->mCurrentByte += bytesWrit;
        }
        else {
            //error, stop
            perror("CoreAudioInput::HandleInputBuffer write");
		    if (OSX_COREAUDIO_DEBUG) { printf("\n\tHandleInputBuffer wrote %d bytes", bytesWrit); fflush(stdout); }         
            pAqData->mIsRunning = false;
            break;
        }
        remaining -= bytesWrit;
        written   += bytesWrit;
    }
 
    AudioQueueEnqueueBuffer(pAqData->mQueue,
                            inBuffer,
                            0,
                            NULL);
}

int CoreAudioInput::EnqueueBuffers() {
    for (int i = 0; i < kNumberBuffers; ++i) {
        AudioQueueEnqueueBuffer (mQueue,
                                mBuffers[i],
                                0,
                                NULL);
    }
    return 0;
}

int CoreAudioInput::ConfigureAudio(int nChannels, int sampleRate, int bitsPerSample, const char *nCoding, int bufferSize) {
    ConfigureAudioFormat(nChannels, sampleRate, bitsPerSample, nCoding, bufferSize);
    return 0;
}

int CoreAudioInput::Start(int fd) {
    pipefd = fd;

    // Start pulling for audio data
    mCurrentPacket = 0;
    mIsRunning     = true;

    // Create audio queue
    AudioQueueNewInput( &mDataFormat,
                        CoreAudioInput::HandleInputBuffer,
                        this,
                        NULL,
                        kCFRunLoopCommonModes,
                        0,
                        &mQueue);
                        
    //Get buffer size for configured audio
    DeriveIPBufferSize( mQueue,
                        mDataFormat,
                        (bufferByteSize / (mDataFormat.mSampleRate * mDataFormat.mBytesPerFrame)),
                        &bufferByteSize);

    //allocate and enqueue
    if (AllocateAudioBuffers() < 0) {
        return -1;
    }

    EnqueueBuffers();

    AudioQueueStart (mQueue, NULL);
    return 0;
}
    
int CoreAudioInput::Stop() {
    if(mIsRunning) {
        mIsRunning     = false;

        // Stop pulling audio data
        AudioQueueStop (mQueue, true);
        
        AudioQueueDispose (mQueue, true);
        
        close(pipefd);
        pipefd = -1;

    }
    fprintf(stderr, "Recording stopped.\n");
    return 0;
}

//CoreAudioOutput impl
CoreAudioOutput::CoreAudioOutput() : CoreAudioIOBase(), Thread(12, 0, this)  {
    mNumPacketsToRead = 1;
    mPacketDescs = NULL;
}

CoreAudioOutput::~CoreAudioOutput() {
    // Stop pulling audio data
    Stop();
}

UInt32 CoreAudioOutput::DeriveOPBufferSize( AudioStreamBasicDescription &ASBDesc,
                                            UInt32                      maxPacketSize,
                                            Float64                     seconds,
                                            UInt32                      *outBufferSize,
                                            UInt32                      *outNumPacketsToRead) {
    static const UInt32 maxBufferSize = 0x400;  //keep it small for low latency
    static const UInt32 minBufferSize = 0x080;
 
    if (ASBDesc.mFramesPerPacket != 0) {
        Float64 numPacketsForTime = (ASBDesc.mSampleRate / ASBDesc.mFramesPerPacket) * seconds;
        *outBufferSize = numPacketsForTime * maxPacketSize;
    } 
    else {
        *outBufferSize = maxBufferSize > maxPacketSize ? maxBufferSize : maxPacketSize;
    }
 
    if ((*outBufferSize > maxBufferSize) && (*outBufferSize > maxPacketSize)) {
        *outBufferSize = maxBufferSize;
    }
    else if (*outBufferSize < minBufferSize) {
        *outBufferSize = minBufferSize;
    }
 
    *outNumPacketsToRead = *outBufferSize / maxPacketSize;

    return 0;
}

void CoreAudioOutput::HandleOutputBuffer(void                *aqData,
                                        AudioQueueRef        inAQ,
                                        AudioQueueBufferRef  inBuffer) {
    CoreAudioOutput *pAqData = (CoreAudioOutput *) aqData;
    //if (OSX_COREAUDIO_DEBUG) { printf("\n\tHandleOutputBuffer: isRunning  %d ", pAqData->mIsRunning); fflush(stdout); }

    UInt32 numBytesReadFromFile = 0;
    UInt32 remaining = inBuffer->mAudioDataBytesCapacity;    //pAqData->bufferByteSize;
    //if (pAqData->mIsRunning == false) return;

    //read from pipe into buffer
    while (remaining > 0) {
        if (OSX_COREAUDIO_DEBUG) { printf("\n\tHandleOutputBuffer reading %d bytes", remaining); fflush(stdout); }        
        int bytesRead = read(pAqData->pipefd, ((char*)inBuffer->mAudioData) + numBytesReadFromFile, remaining);
        if (bytesRead > 0) {
            pAqData->mCurrentByte += bytesRead;
        }
        else if (bytesRead == 0) {  //no data, something wrong?
		    if (OSX_COREAUDIO_DEBUG) { printf(" -  read 0 bytes"); fflush(stdout); }
            break;
        }
        else {
            //error, stop
		    if (OSX_COREAUDIO_DEBUG) { printf(" -  read %d bytes", bytesRead); fflush(stdout); }
            pAqData->mIsRunning = false;
            break;
        }
        remaining -= bytesRead;
        numBytesReadFromFile += bytesRead;
        if (OSX_COREAUDIO_DEBUG) { printf(" read=%u total=%u remaining=%u bytes", bytesRead, numBytesReadFromFile, remaining); fflush(stdout); }        
    }

    if (numBytesReadFromFile > 0) {
        inBuffer->mAudioDataByteSize = numBytesReadFromFile;
        AudioQueueEnqueueBuffer(pAqData->mQueue,
                                inBuffer,
                                NULL,
                                0);
    } else {
		if (OSX_COREAUDIO_DEBUG) { printf("\n\tCoreAudioOutput::HandleOutputBuffer stopping...\n"); fflush(stdout); }
        AudioQueueStop (pAqData->mQueue, false );
        pAqData->mIsRunning = false; 
    }
    return;
}

int CoreAudioOutput::PrimeBuffers() {
    for (int i = 0; i < kNumberBuffers; ++i) {
        HandleOutputBuffer( this, mQueue, mBuffers[i] );
    }
    return 0;
}


int CoreAudioOutput::Start(int fd) {    
    pipefd = fd;
    this->start();
    return 0;
}
    
int CoreAudioOutput::Stop() {
    if(mIsRunning) {
        mIsRunning = false;
        
        // Stop pulling audio data
        AudioQueueStop (mQueue, false);
        
        AudioQueueDispose (mQueue, false);
        
        close(pipefd);
        pipefd = -1;
    }
    fprintf(stderr, "Playback stopped.\n");
    return 0;
}

int CoreAudioOutput::ConfigureAudio(int nChannels, int sampleRate, int bitsPerSample, const char *nCoding, int bufferSize) {
    ConfigureAudioFormat(nChannels, sampleRate, bitsPerSample, nCoding, bufferSize);

    //Get buffer size for configured audio
    DeriveOPBufferSize( mDataFormat,
                        1,
                        (bufferByteSize / (mDataFormat.mSampleRate * mDataFormat.mBytesPerFrame)),
                        &bufferByteSize,
                        &mNumPacketsToRead);

    return 0;
}

void* CoreAudioOutput::run(void *arg) {
    int status = 0;
    // Create new queue
    if ((status = AudioQueueNewOutput(&mDataFormat,
                        CoreAudioOutput::HandleOutputBuffer,
                        this,
                        CFRunLoopGetCurrent (),
                        kCFRunLoopCommonModes,
                        0,
                        &mQueue)) != 0) {
        printf("AudioQueueNewOutput failed %d\n", status); fflush(stdout);
        return NULL;
    }

    //allocate and prime buffers
    if (AllocateAudioBuffers() < 0) {
        printf("AllocateAudioBuffers failed\n"); fflush(stdout);
        return NULL;
    }
 
    // Start pulling for audio data
    mCurrentPacket = 0;
    mIsRunning     = true;

    // Set gain
    Float32 gain = 1.0;
    // Optionally, allow user to override gain setting here
    AudioQueueSetParameter (mQueue,
                            kAudioQueueParam_Volume,
                            gain);

    PrimeBuffers();

    if ((status = AudioQueueStart(mQueue, NULL)) != 0) {
        printf("AudioQueueStart failed %d\n", status); fflush(stdout);
        return NULL;
    }
        
    do {
        CFRunLoopRunInMode (kCFRunLoopDefaultMode, 0.20, false);
    } while (mIsRunning);
     

    CFRunLoopRunInMode (kCFRunLoopDefaultMode, 0.2, false);
    
    return NULL;
}