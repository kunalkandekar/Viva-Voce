#include "AdpcmCodec.h"
#include "ResourceManager.h"
#include "ulaw.h"

AdpcmCodec::AdpcmCodec(Common* common):Thread(0, 3, common) {
	msCheckThreshold = 2000;
	//init
	pcmSamples = new unsigned short[common->samplesPerCapture + 10];

	this->common = common;
	adpcm_init(&common->stateEnc,0);
	adpcm_init(&common->stateDec,0);
}

AdpcmCodec::~AdpcmCodec() {
	delete [] pcmSamples;
	adpcm_close(common->stateDec);
	adpcm_close(common->stateEnc);
}

int AdpcmCodec::encode(Memo *memoIn, Memo *memoOut) {
	unsigned char *uLawData = (unsigned char*) memoIn->bytes;
	for(int itr=0;itr<memoIn->data1; itr++) {
		pcmSamples[itr] = audio_u2s(uLawData[itr]);
	}

	rtpAudioPayload *payload = (rtpAudioPayload*)memoOut->bytes;
	payload->first = pcmSamples[0];
	adpcm_set(common->stateEnc, pcmSamples[0] ,0);
	int ret = adpcm_encode( common->stateEnc,
    						(short*) pcmSamples+1,
    						memoIn->data1-1,
    						payload->data,
    						memoOut->size-4);
    memoOut->data1 = ret+4;
    
	payload->stepsizeIndex = 0;
	payload->reserved=0;
	return ret;
}

int AdpcmCodec::decode(Memo *memoIn, Memo *memoOut) {
	rtpAudioPayload *payload = (rtpAudioPayload*)memoIn->bytes;
	pcmSamples[0] = payload->first;
	adpcm_set(common->stateDec,payload->first,
		      payload->stepsizeIndex);

	int ret = adpcm_decode( common->stateDec,
        					payload->data,
        					memoIn->data1-4,
        					(short*)pcmSamples+1,
        					common->samplesPerCapture);

	memoOut->data1 = ret+1;
	unsigned char *uLawData = (unsigned char*)memoOut->bytes;
	for(int itr=0;itr<memoOut->data1; itr++) {
		uLawData[itr] = audio_s2u(pcmSamples[itr]);
	}

    return ret;
}

void* AdpcmCodec::run(void* arg){
	common = (Common *)arg;
	Memo	*memoIn;
	Memo	*memoOut;

	while(common->go) {
		memoIn = (Memo*)common->codecQ->waitForData(VIVOCE_WAIT_MSEC);
		if(memoIn!=NULL) {
			switch(memoIn->code) {
				case VIVOCE_MSG_ENC:
					memoOut = memoIn;			//re-use incoming memos;

				    encode(memoIn, memoOut);

					memoOut->code = VIVOCE_MSG_SEND;
					common->sendQ->signalData(memoOut);

					common->unitsEnc++;        //book-keeping
					break;

				case VIVOCE_MSG_DEC:
					memoOut = memoIn;			//re-use incoming memo;

                    decode(memoIn, memoOut);

					memoOut->code = VIVOCE_MSG_PLAY;
					common->playQ->signalData(memoOut);

					common->unitsDec++;        //book-keeping
					break;

				case VIVOCE_RESPOND:
					printf("\n\tADPCM Codec active");
					common->rmg->releaseMemo(memoIn);
					break;

				default:
					break;
			}
		}
		else if(common->codecStop) {
		  //stop codec only for some reason... probably audio testing
		  break;
		}
	}
	printf("AdpcmCodec stopped. ");
	return NULL;
}