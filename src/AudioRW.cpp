#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "AudioRW.h"


#if defined (__SVR4) && defined (__sun) && !defined(SOLARIS)
#define SOLARIS 1
#endif

/* ---- LINUX-specific implementation ---- */
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

int initAudio(Common* common) {
	//open i/p and o/p files
	if(openAudioInput(common) < 0) {
		perror ("\n\tError opening AUDIO_DEV for input");
		return -1;
	}

	if(openAudioOutput(common)  < 0) {
		perror ("\n\tError opening AUDIO_DEV for output");
		return -1;
	}
 
 	//close again, this was just a test
 	close(common->audio_ip);
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
#endif

/* OSX-specific implementation */
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

int openAudioInput(Common *common) {
    CoreAudioInput *coreai = ((CoreAudioInterface*)common->ai)->coreai;
    if ((coreai == NULL) || (coreai->Start() < 0)) {
        return -1;
    }
    return 0;
}

int closeAudioInput(Common *common) {
    CoreAudioInput *coreai = ((CoreAudioInterface*)common->ai)->coreai;
    if (coreai)
        delete coreai;
    ((CoreAudioInterface*)common->ai)->coreao  = NULL;
    close(common->audio_ip);
    return 0;
}

int initAudioInput(Common *common) {
    //open pipe for inter-thread communication
    //not the best, but easiest to integrate with the AudioRW code
    int fd[2];
    if(pipe(fd) < 0) {
        return -1;
    }
    common->audio_ip = fd[0];
    
    CoreAudioInput *coreai = new CoreAudioInput(fd[1]);
    ((CoreAudioInterface*)common->ai)->coreai = coreai;
    if(coreai->ConfigureAudio(common->nChannels, common->sampleRate, 
                            common->precision, common->nCoding) < 0) {
        //error
        closeAudioInput(common);
        return -1;
    }
    //need same for audio output
    return 0;
}

int openAudioOutput(Common* common) {
	//open audio o/p
    CoreAudioOutput *coreao = ((CoreAudioInterface*)common->ai)->coreao;
    if ((coreao == NULL) || (coreao->Start() < 0)) {
        return -1;
    }
    return 0;
}

int closeAudioOutput(Common* common) {
	//close audio o/p
    CoreAudioOutput *coreao = ((CoreAudioInterface*)common->ai)->coreao;
    if (coreao) {
        delete coreao;
    }
    ((CoreAudioInterface*)common->ai)->coreao  = NULL;
    close(common->audio_op);
    return 0;
}

int initAudioOutput(Common *common) {    
    //open pipe for inter-thread communication
    //not the best, but easiest to integrate with the AudioRW code
    int fd[2];
    if(pipe(fd) < 0) {
        return -1;
    }
    common->audio_op = fd[1];

    CoreAudioOutput *coreao = new CoreAudioOutput(fd[0]);
    ((CoreAudioInterface*)common->ai)->coreao = coreao;
    if(coreao->ConfigureAudio(common->nChannels, common->sampleRate, 
                            common->precision, common->nCoding) < 0) {
        //error
        closeAudioOutput(common);
        return -1;
    }

    //need same for audio output
    return 0;
}

int initAudio(Common *common) {
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

#endif 

/* SOLARIS-specific implementation */
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

int initAudio(Common* common) {
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
#endif


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

	struct timeval* timeout = new timeval();
	int 	max;
	int 	done=0;

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
				timeout->tv_sec	 = 0;			// time out = 50 msec
				timeout->tv_usec = 20000;

				nFound= select(max, &readSet, NULL, NULL, timeout);

				if(nFound==0) {
					//timed out
					continue;
				}
				if(nFound < 0) {
					perror("\n\tError in reading audio:");
					continue;
				}
				nFound = FD_ISSET(common->audio_ip, &readSet);
				if(nFound < 0) {
					//error !!
					perror("\n\tError in reading audio:");
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
	delete timeout;
	printf("AudioReader stopped. "); fflush(stdout);
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
    if (openAudioOutput(common) < 0) {
        perror ("\n\tError opening AUDIO_DEV for output");
        printf("AudioWriter breaking early\n"); fflush(stdout);
		return NULL;
    }
    
	setBlocking(common->audio_op, 1);

	while(common->go) {
		memo = (Memo*)common->playQ->waitForData(20);
		if(memo != NULL) {
			switch(memo->code) {
				case VIVOCE_MSG_PLAY:		//data from Q
					buf=0;
					len = memo->data1;
					while (1) {
						size = write(common->audio_op, memo->bytes+buf, len);
						
						if (size < len) 	// Success, but not done writing
						{
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
					//common->rmg->releaseMemo(memo);
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
	printf("AudioWriter stopped."); fflush(stdout);
	return NULL;
}
