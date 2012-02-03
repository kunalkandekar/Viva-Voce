#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <math.h>

#define PI 3.14159265

#include "AudioRW.h"
#include "ulaw.h"

#if defined (__SVR4) && defined (__sun) && !defined(SOLARIS)
#define SOLARIS 1
#endif

//forward declarations
int initAudioInput(Common *common);
int openAudioInput(Common *common);
int closeAudioInput(Common *common);
int cleanupAudioInput(Common *common);

int initAudioOutput(Common *common);
int openAudioOutput(Common *common);
int closeAudioOutput(Common *common);
int cleanupAudioOutput(Common *common);

/**************************** LINUX-specific implementation ****************************/
#ifdef __linux__
#include <stropts.h>
#include <sys/soundcard.h>

#define AUDIO_DEV "/dev/dsp"

int openAudioOutput(Common* common) {
	//open i/p files
	if ((common->audio_op = open(AUDIO_DEV, O_WRONLY | O_NDELAY)) < 0 ) {
		return -1;
	}	
	
    int arg = 0;
    int status = 0;

    //set encoding
    int encoding = 0;
	if(!strncmp(common->nCoding, "ulaw", 4)) {
        encoding = arg = AFMT_MU_LAW;
	}
	else if(!strncmp(common->nCoding, "alaw", 4)) {
	   encoding = arg = AFMT_A_LAW;
	}
	else if(!strncmp(common->nCoding, "pcm", 3)) {
	   encoding = arg = AFMT_S16_LE;
	}
	else if(!strncmp(common->nCoding, "u8", 2)) {
	   encoding = arg = AFMT_U8;
	}
	else if(!strncmp(common->nCoding, "adpcm", 5)) {
	   encoding = arg = AFMT_IMA_ADPCM;
	}
	else { //default is ULAW
        encoding = arg = AFMT_MU_LAW;
	}
    status = ioctl(common->audio_op, SNDCTL_DSP_SETFMT, &arg);
    if ((status < 0) || (arg != encoding)) {
        perror("SNDCTL_DSP_SETFMT ioctl failed for audio output");
        return -1;
    }

	arg = common->precision;
	status = ioctl(common->audio_op, SOUND_PCM_WRITE_BITS, &arg);
    if ((status < 0) || (arg != common->precision)) {
        perror("SOUND_PCM_WRITE_BITS ioctl failed for audio output");
        return -1;
    }
	arg = common->nChannels;
	status = ioctl(common->audio_op, SOUND_PCM_WRITE_CHANNELS, &arg);
    if ((status < 0) || (arg != common->nChannels)) {
        perror("SOUND_PCM_WRITE_CHANNELS ioctl failed for audio output");
        return -1;
    }
	arg = common->sampleRate;
	status = ioctl(common->audio_op, SOUND_PCM_WRITE_RATE, &arg);
    if ((status < 0) || (arg != common->sampleRate)) {
        perror("SOUND_PCM_WRITE_RATE ioctl failed for audio output");
        return -1;
    }
	//common->ai->play.buffer_size = 2048;
	return 0;
}

int openAudioInput(Common* common) {
	//open i/p files    
	if ((common->audio_ip = open(AUDIO_DEV, O_RDONLY | O_NDELAY)) < 0) {
        return -1;
	}
	
    int arg = 0;
    int status = 0;

    //set encoding
    int encoding = 0;
	if(!strncmp(common->nCoding, "ulaw", 4)) {
        encoding = arg = AFMT_MU_LAW;
	}
	else if(!strncmp(common->nCoding, "alaw", 4)) {
	   encoding = arg = AFMT_A_LAW;
	}
	else if(!strncmp(common->nCoding, "pcm", 3)) {
	   encoding = arg = AFMT_S16_LE;
	}
	else if(!strncmp(common->nCoding, "u8", 2)) {
	   encoding = arg = AFMT_U8;
	}
	else if(!strncmp(common->nCoding, "adpcm", 5)) {
	   encoding = arg = AFMT_IMA_ADPCM;
	}
	else { //default is ULAW
        encoding = arg = AFMT_MU_LAW;
	}
    status = ioctl(common->audio_ip, SNDCTL_DSP_SETFMT, &arg);
    if ((status < 0) || (arg != encoding)) {
        perror("SNDCTL_DSP_SETFMT ioctl failed for audio input");
        return -1;
    }

	arg = common->precision;
//	status = ioctl(common->audio_ip, SOUND_PCM_READ_BITS, &arg);
//    if ((status < 0) || (arg != common->precision)) {
//        perror("SOUND_PCM_READ_BITS ioctl failed for audio input");
//        printf("status=%d arg=%d prec=%d\n", status, arg, common->precision);
//        return -1;
//    }
	arg = common->nChannels;
	status = ioctl(common->audio_ip, SOUND_PCM_READ_CHANNELS, &arg);
    if ((status < 0) || (arg != common->nChannels)) {
        perror("SOUND_PCM_READ_CHANNELS ioctl failed for audio input");
        return -1;
    }
	arg = common->sampleRate;
	status = ioctl(common->audio_ip, SOUND_PCM_READ_RATE, &arg);
    if ((status < 0) || (arg != common->sampleRate)) {
        perror("SOUND_PCM_READ_RATE ioctl failed for audio input");
        return -1;
    }
	//common->ai->record.buffer_size = 2048;
	//common->ai->record.gain = 30;//common->gain;
    return 0;
}

int initAudioLinux(Common* common) {
	//open i/p and o/p files
	if(openAudioInput(common) < 0) {
		perror ("\n\tError opening AUDIO_DEV for input");
		return -1;
	}
 	//close again, this was just a test
 	close(common->audio_ip);

	if(openAudioOutput(common)  < 0) {
		perror ("\n\tError opening AUDIO_DEV for output");
		return -1;
	}
  	//close again, this was just a test
 	close(common->audio_op);
	return 0;
}


int closeAudioInput(Common* common) {
	//close i/p files
	close(common->audio_ip);
	return 0;
}

int closeAudioOutput(Common* common) {
    close(common->audio_op);
    return 0;
}

int cleanupAudioInput(Common *common) {
    return 0;
}

int cleanupAudioOutput(Common *common) {
    return 0;
}

#endif

/**************************** OSX-specific implementation ****************************/
#ifdef __APPLE__
#include "OSXCoreAudioIO.h"

class CoreAudioInterface {
public:
    CoreAudioInterface() {
        coreai = NULL;
        coreao = NULL;
    }
    CoreAudioInput  *coreai;
    CoreAudioOutput *coreao;
};

int closeAudioInput(Common *common) {
    CoreAudioInput *coreai = ((CoreAudioInterface*)common->ai)->coreai;
    coreai->Stop();
    close(common->audio_ip);
    //common->audio_ip = -1;
    return 0;
}

int openAudioInput(Common *common) {
    //open pipe for inter-thread communication... not the best, but easiest to 
    //integrate with the other AudioRW code that treats Audio devices as files
    int fd[2];
    if(pipe(fd) < 0) {
        return -1;
    }
    common->audio_ip = fd[0];
    
    CoreAudioInput *coreai = ((CoreAudioInterface*)common->ai)->coreai;
    if(coreai->Start(fd[1]) < 0) {
        closeAudioInput(common);
        return -1;
    }
    return 0;
}

int initAudioInput(Common *common) {
    //need same for audio output
    CoreAudioInput *coreai = new CoreAudioInput();
    ((CoreAudioInterface*)common->ai)->coreai = coreai;
    if(coreai->ConfigureAudio(common->nChannels, common->sampleRate, 
                            common->precision, common->nCoding, common->bytesPerCapture) < 0) {
        //error
        cleanupAudioInput(common);
        return -1;
    }    
    return 0;
}

int closeAudioOutput(Common* common) {
	//close audio o/p
    CoreAudioOutput *coreao = ((CoreAudioInterface*)common->ai)->coreao;
    coreao->Stop();
    close(common->audio_op);
    //common->audio_op = -1;    
    return 0;
}

int openAudioOutput(Common* common) {
    //open pipe for inter-thread communication
    //not the best, but easiest to integrate with the AudioRW code
    int fd[2];
    if(pipe(fd) < 0) {
        return -1;
    }
    common->audio_op = fd[1];

	//open audio o/p
    CoreAudioOutput *coreao = ((CoreAudioInterface*)common->ai)->coreao;
    if (coreao->Start(fd[0]) < 0) {
        closeAudioOutput(common);
        return -1;
    }
    return 0;
}

int initAudioOutput(Common *common) {
    CoreAudioOutput *coreao = new CoreAudioOutput();
    ((CoreAudioInterface*)common->ai)->coreao = coreao;
    if(coreao->ConfigureAudio(common->nChannels, common->sampleRate, 
                            common->precision, common->nCoding, common->bytesPerCapture) < 0) {
        //error
        cleanupAudioOutput(common);
        return -1;
    }
    return 0;
}

int initAudioOSX(Common *common) {
    if (common->ai == NULL) {
        common->ai = new CoreAudioInterface();
    }
    if(initAudioInput(common) < 0) {
		perror ("\n\tError opening audio device for input");        
        return -1;
    }
    if(initAudioOutput(common) < 0) {
		perror ("\n\tError opening audio device for output");        
        return -1;
    }
    //need same for audio output
    return 0;
}

int cleanupAudioInput(Common *common) {
    CoreAudioInput *coreai = ((CoreAudioInterface*)common->ai)->coreai;
    if(coreai)
        delete coreai;
    ((CoreAudioInterface*)common->ai)->coreai  = NULL;
    return 0;
}

int cleanupAudioOutput(Common *common) {
    CoreAudioOutput *coreao = ((CoreAudioInterface*)common->ai)->coreao;
    if (coreao) {
        delete coreao;
    }
    ((CoreAudioInterface*)common->ai)->coreao  = NULL;
    return 0;
}

#endif 

/**************************** SOLARIS-specific implementation ****************************/
#ifdef SOLARIS
#define AUDIO_DEV "/dev/audio"
#define AUDIO_CTL "/dev/audioctl"

int openAudioCtl(Common* common) {
	if ((common->audio_ctl = open(AUDIO_CTL, O_RDWR)) < 0) {
		perror(AUDIO_CTL);
		return -1;
	}
	return 0;
}

int closeAudioCtl(Common* common) {
	if (close(common->audio_ctl ) < 0) {
		perror("CloseOutputAudio(): audio_ctldev");
		return -1;
	}
	return 0;
}

int initAudioSolaris(Common* common) {
	common->ai = (struct audio_info *) malloc (sizeof (struct audio_info));
	struct audio_info *common_ai = (struct audio_info *)common->ai;

	openAudioCtl(common);
	if (ioctl (common->audio_ctl, AUDIO_GETINFO, common_ai) == -1) {
		perror ("\n\tError getting info from /dev/audioctl");
		close (common->audio_ctl);
		return 1;
	}
	common->audio_port = common_ai->play.port;

	AUDIO_INITINFO(common_ai);
	common_ai->play.sample_rate = common->sampleRate;
	common_ai->play.channels = common->nChannels;
	common_ai->play.precision = common->precision;
	//common_ai->play.buffer_size = 2048;

	common_ai->record.sample_rate = common->sampleRate;
	common_ai->record.channels = common->nChannels;
	common_ai->record.precision = common->precision;
	//common_ai->record.buffer_size = 2048;
	//common_ai->record.gain = 30;//common->gain;
	
    //set encoding
	if(!strncmp(common->nCoding, "ulaw", 4)) {
	   common_ai->record.encoding = common_ai->play.encoding = AUDIO_ENCODING_ULAW;	   
	}
	else if(!strncmp(common->nCoding, "alaw", 4)) {
	   common_ai->record.encoding = common_ai->play.encoding = AUDIO_ENCODING_ALAW;
	}
	else if(!strncmp(common->nCoding, "pcm", 3)) {
	   common_ai->record.encoding = common_ai->play.encoding = AUDIO_ENCODING_LINEAR;
	}
	else if(!strncmp(common->nCoding, "u8", 2)) {
	   common_ai->record.encoding = common_ai->play.encoding = AUDIO_ENCODING_LINEAR8;
	}
	else if(!strncmp(common->nCoding, "adpcm", 5)) {
	   common_ai->record.encoding = common_ai->play.encoding = AUDIO_ENCODING_DVI;
	}
	else { //default is ULAW
        common_ai->record.encoding = common_ai->play.encoding = AUDIO_ENCODING_ULAW;
	}	

	ioctl(common->audio_ctl, AUDIO_SETINFO, common->ai);
	closeAudioCtl(common);
	
	return 0;
}

int openAudioInput(Common* common) {
	//open i/p files
	if ((common->audio_ip = open(AUDIO_DEV, O_RDONLY | O_NDELAY)) < 0) {
        return -1;
	}
	return 0;
}

int openAudioOutput(Common* common) {
	//open o/p files    
	if ((common->audio_op = open(AUDIO_DEV, O_WRONLY | O_NDELAY)) < 0 ) {
		return -1;
	}	
    return 0;
}

int closeAudioInput(Common* common) {
	//close i/p files
	close(common->audio_ip);
	return 0;
}

int closeAudioOutput(Common* common) {
    close(common->audio_op);
    return 0;
}

int cleanupAudioInput(Common *common) {
    return 0;
}

int cleanupAudioOutput(Common *common) {
    return 0;
}

#endif

//TODO: Separate out OS-specific stuff into separate files, maybe refactor into class hierarchy
//E.g. audiorw_linux.cpp, audiorw_osx.cpp etc. or AudioIO -> LinuxAudioIO, OSXCoreAudioIO, etc. 
int initAudio(Common *common) {
    int ret = 0;
#ifdef SOLARIS
    ret = initAudioSolaris(common);
#elif defined __linux__
    ret = initAudioLinux(common);
#elif defined __APPLE__
    ret = initAudioOSX(common);
#elif defined _WIN32 || defined _WIN64
    //initAudioWin(common);
    #error "Windows is currently not a supported platform"
#else
    #error "Unsupported platform"
#endif
    return ret;
}

/* OS-neutral stuff */
AudioReader::AudioReader(Common* common):Thread(1, 0, common) {

}

AudioReader::~AudioReader() {

}

void* AudioReader::run(void* arg) {
	common = (Common *)arg;

	int 	nFound;

	fd_set	readSet;
	int 	ret;
	Memo	*memo;
	long	len = common->samplesPerCapture;		//u-Law

	struct timeval timeout;
	int 	max;
	int 	done=0;

    static unsigned char sinewave[161];    //for a given sample rate of 8K
    static short sinepcm[161];
    float r = 8000;
    float a = 256 * 64;
    float f = (8000/23);
    for (unsigned int n = 0; n < sizeof(sinewave); n++) {
        sinepcm[n] = (int)(a * sin((2 * PI * n * f) / r));
    }
    linear2ulaw(sinepcm, sinewave, sizeof(sinewave));

	while(common->go) {
		wait(VIVOCE_WAIT_MSEC);
		if(common->isConnected || common->audioGo) {
		    if(openAudioInput(common) < 0) {
		      	perror ("\n\tError opening AUDIO_DEV for input");
		        return NULL;
		    }

			setBlocking(common->audio_ip,1);
			max = common->audio_ip+1;
			while(common->isConnected  || common->audioGo) {
				FD_ZERO(&readSet);
				FD_SET(common->audio_ip, &readSet);
				timeout.tv_sec	= 0;			// time out = 50 msec
				timeout.tv_usec = 20000;

				nFound = select(max, &readSet, NULL, NULL, &timeout);

				if(nFound==0) {
					//timed out
					continue;
				}
				if(nFound < 0) {
					perror("\n\tError in reading audio - select");
					continue;
				}
				nFound = FD_ISSET(common->audio_ip, &readSet);
				if(nFound < 0) {
					//error !!
					perror("\n\tError in reading audio - FD_ISSET");
					continue;
				}
				if(nFound>0) {
					//available to read
					memo = common->rmg->allocMemo();
					memo->code = VIVOCE_MSG_ENC;
					done = 0;
				    //if (VIVAVOCE_DEBUG) { printf("\n\treading %d bytes", common->bytesPerCapture); fflush(stdout); }
					while(done < common->bytesPerCapture) {
						ret = read(	common->audio_ip,
									memo->bytes+done,
									len-done);
						done+=ret;
					}
					
					if(common->audioGenTone) { //replace with sine
                        memcpy(memo->bytes, sinewave, sizeof(sinewave));
                        done = sizeof(sinewave);
					}
					
				    //if (VIVAVOCE_DEBUG) { printf("\n\tread %d bytes", done); fflush(stdout); }
					memo->data1=done;
					common->unitsRec++;
					common->codecQ->signalData(memo);
				}
				if(common->audrSig) {
					printf("\n\tAudio Reader active"); fflush(stdout);
					common->audrSig=0;
				}
			}
			closeAudioInput(common);
		}
		if(common->audrSig) {
			printf("\n\tAudio Reader active"); fflush(stdout);
			common->audrSig=0;
		}
	}
	printf("AudioReader stopped. "); fflush(stdout);
	cleanupAudioInput(common);	
	return NULL;
}


AudioWriter::AudioWriter(Common* common):Thread(2, 0, common) {

}

AudioWriter::~AudioWriter() {

}

void* AudioWriter::run(void* arg) {
	common = (Common *)arg;
	Memo* memo;
	int size;
	int buf;
	int len;

	static unsigned char golden[161];  //ooh that's deep
	static short goldenpcm[161];
    linear2ulaw(goldenpcm, golden, sizeof(golden));

	while(common->go) {
        if(!common->isConnected && !common->audioGo) {
		    //drop any memos if we're not playing audio
		    memo = (Memo*)common->playQ->waitForData(20);
		    if(memo && (memo->code == VIVOCE_RESPOND)) {
                printf("\n\tAudio Writer active"); fflush(stdout);
		    }
            common->rmg->releaseMemo(memo);
            continue;
        }
        
	    if(openAudioOutput(common) < 0) {
	      	perror ("\n\tError opening AUDIO_DEV for output");
            printf("AudioWriter breaking early\n"); fflush(stdout);
	        return NULL;
	    }

		setBlocking(common->audio_op, 1);

		while(common->isConnected  || common->audioGo) {
    		memo = (Memo*)common->playQ->waitForData(20);
    		if(memo != NULL) {
    			switch(memo->code) {
    				case VIVOCE_MSG_PLAY:		//data from Q
                        buf=0;
    					len = memo->data1;
    					if(common->audioMute) {
    					    size = write(common->audio_op, golden, sizeof(golden));
    					}
    					else while (1) {
    						size = write(common->audio_op, memo->bytes+buf, len);
    
    						if (size < 0) { 
                                //error, break
                                perror("AudioWriter::run write");
                                break;
    						}
    						else if(size == 0) {
                                //maybe some problem
                                //printf("\n\tAudio Writer wrote 0...");
                                break;
    						}
    						else if(size < len) {
    							buf += size;	// Move write pointer up
    							len -= size;	// Decrement count
    						}
    						else
    							break;		// All written, exit
    					}
    					common->unitsPld++;
    					break;

    				case VIVOCE_RESPOND:
    					printf("\n\tAudio Writer active"); fflush(stdout);
    					break;
    				default:
    					break;
    			}
    			common->rmg->releaseMemo(memo);
    		}
    		else {
    		    //printf("\nno memo, qsize = %d", common->playQ->count()); fflush(stdout);
    		}
		}
		closeAudioOutput(common);
	}
	printf("AudioWriter stopped."); fflush(stdout);
	cleanupAudioOutput(common);
	return NULL;
}
