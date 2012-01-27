#include "kodek.h"
#include <stdio.h>

/* First table lookup for Ima-ADPCM quantizer */
static char indexadjust[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* Second table lookup for Ima-ADPCM quantizer */
static short stepsize[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};


int adpcm_init(adpcm_state_t *state, int isNwBound) {
	*state = (adpcm_state_t)malloc(sizeof(struct _adpcm_state));
	(*state)->nwBound 	= isNwBound;
	(*state)->sample	= 0;
	(*state)->delta		= 0;
	(*state)->itr		= 0;
	(*state)->previous	= 0;
	(*state)->index		= 0;
	(*state)->isNew		= 1;
	return 0;
}

int adpcm_getEncodedBufferSize(int nSamples,int isNew) {
	if(isNew)
		return ((nSamples-1)/2 +2);
	else
		return (nSamples/2);
}

int adpcm_getDecodedBufferSize(int nBytes, int isNew) {
	if(isNew)
		return ((nBytes-2)*2 +1);
	else
		return (nBytes*2);
}

void adpcm_set(adpcm_state_t state, int first, int index) {
	state->previous=first;
	state->index=index;
	state->isNew=1;
}

void adpcm_reset(adpcm_state_t state) {
	state->previous=0;
	state->index=0;
	state->isNew=1;
}

//private functions
void quantize(adpcm_state_t state) {
	state->max=0;
	state->smpl = (float)state->delta;
	for(state->qVal=0; state->qVal<7; state->qVal++) {
		state->min=state->max;
		state->max=0.25*(state->qVal+1);
		if((state->min * state->step <= state->smpl)
			&&(state->smpl < state->max * state->step)) {
			state->mul=state->min;
			return;
		}
	}
	state->qVal=7;
	state->mul=1.75;
	return;
}
;
int adpcm_encode(adpcm_state_t state,
				 short pcmSamples[],
				 int nSamples,
				 char* adpcmData,
				 int nBytes) {
	state->bytePtr = (signed char *) adpcmData;
	state->isBuffered = 1;
    state->count=0;

    for (state->itr=0; state->itr < nSamples ; state->itr++ ) {
		state->sample = pcmSamples[state->itr];

		state->delta = state->sample - state->previous;

		state->sign = (state->delta < 0) ? -1 : 1;
		if ( state->sign < 0)
			state->delta = (-state->delta);

    	state->step = stepsize[state->index];
    	quantize(state);

		//predicted val
    	state->previous = (short)roundOff((float)state->previous
							+ (float)(state->sign*state->step*state->mul));
    	state->index+=indexadjust[state->qVal];

		//clamp
		if ( state->index < 0 )
			state->index = 0;
		else if ( state->index > 88 )
			state->index = 88;

		state->qOut = state->qVal;

		if(state->sign<0)
			state->qOut = state->qOut | 0x08;

		if ( state->isBuffered ) {
			state->outputBuffer = (state->qOut << 4) & 0xf0;
			state->isBuffered = 0;
		} else {
			state->bytePtr[state->count]
				= (state->qOut & 0x0f) | state->outputBuffer;
			state->count++;
			state->isBuffered = 1;
		}
    }

    /* Output last step, if needed */
    if ( !state->isBuffered )
      *state->bytePtr++ = state->outputBuffer;
    return state->count;
}

int adpcm_decode(adpcm_state_t state,
				 char adpcmData[],
				 int nBytes,
				 short samples[],
				 int nSamples) {

    state->count = nBytes*2;
    state->shortPtr = samples;
    state->bytePtr = (signed char *)adpcmData;
    state->isBuffered = 0;
	state->qVal=0;

    for (state->itr=0 ; state->itr < state->count; state->itr++) {
		//get the delta value
		if ( state->isBuffered ) {
			state->qOut = state->bytePtr[state->qVal] & 0x0f;
			state->qVal++;
			state->isBuffered = 0;
		} else {
			state->qOut = (state->bytePtr[state->qVal] >> 4) & 0x0f;
			state->isBuffered = 1;
		}

		if(state->qOut >= 8) {
			state->sign=-1;
			state->qOut = state->qOut & 0x07;
		}
		else
			state->sign=1;

		if(state->qOut > 7) {		//error
			return -1;
		}
		state->step = stepsize[state->index];
		state->mul = 0.25 * (float)state->qOut;

		state->previous = (short)roundOff((float)state->previous
							+(float)(state->sign * state->mul * state->step));

		state->shortPtr[state->itr] = state->previous;

		state->index+= indexadjust[state->qOut];
		if ( state->index < 0 )
			state->index = 0;
		else if ( state->index > 88 )
			state->index = 88;
    }
    return state->count;
}

void adpcm_close(adpcm_state_t state) {
    if(state)
	   free(state);
	return;
}

/*
void get_quant(signed short delta, int step_size, int * q_out, float * step_mul)
{
		int mul;
		float sample;

		if(delta<0) mul=-1;
		else mul = 1;

		sample = (float)mul*(float)delta;

		if(sample < (float)step_size/(float)4)
		{
			*q_out=0;
			*step_mul=0;
		}


		else if(sample >= (float)step_size/(float)4  && sample < (float)step_size/(float)2)
		{
			*q_out=1;
			*step_mul=0.25;
		}


		else if(sample >= (float)step_size/(float)2  && sample < (float)3*(float)step_size/(float)4)
		{
			*q_out=2;
			*step_mul=0.5;
		}


		else if(sample >= (float)3*(float)step_size/(float)4  && sample < (float)step_size)
		{
			*q_out=3;
			*step_mul=0.75;
		}


		else if(sample >= (float)step_size  && sample < (float)5*(float)step_size/(float)4)
		{
			*q_out=4;
			*step_mul=1;
		}


		else if(sample >= (float)5*(float)step_size/(float)4  && sample < (float)3*(float)step_size/(float)2)
		{
			*q_out=5;
			*step_mul=1.25;
		}


		else if(sample >= (float)3*(float)step_size/(float)2  && sample < (float)7*(float)step_size/(float)4)
		{
			*q_out=6;
			*step_mul=1.5;
		}

		else
		{
			*q_out=7;
			*step_mul=1.75;
		}
}

void getBuf(int type, void *data) {
	switch(type) {
	case 0:
		memcpy(data,RD_BUFFER,MAX_BUF);
		break;
	case 1:
		memcpy(data,TX_BUFFER,MAX_TX_BUF);
		break;
	case 2:
		memcpy(data,RCV_BUFFER,MAX_TX_BUF);
		break;
	default:
		memcpy(data,WR_BUFFER,MAX_BUF);
	}
}

void setBuf(int type, void *data) {
	switch(type) {
	case 0:
		memcpy(RD_BUFFER,data,MAX_BUF);
		break;
	case 1:
		memcpy(TX_BUFFER,data,MAX_TX_BUF);
		break;
	case 2:
		memcpy(RCV_BUFFER,data,MAX_TX_BUF);
		break;
	default:
		memcpy(WR_BUFFER,data,MAX_BUF);
	}
}

void ADPCM_ENCODE(void)
{

	int i, q_out, t2index=0, step_size, index_adj, sign;
	float step_mul, new_delta;

	signed short first_pcm_sample, prev_pcm_sample, current_pcm_sample, delta  ,t2;
	unsigned char ulaw_sample;

	char adpcm_sample1, adpcm_sample2;

	signed short* current = RD_BUFFER+1;

	// this is the first sample... send it as it is
	first_pcm_sample = RD_BUFFER[0];
	memcpy(TX_BUFFER,&first_pcm_sample,2);

	memcpy(&t2,TX_BUFFER,2);

	i=2;
	prev_pcm_sample = first_pcm_sample;

	while(i<MAX_TX_BUF)
	{

		current_pcm_sample = *current;
		current++;


		delta = current_pcm_sample - prev_pcm_sample;

		if(delta>=0) sign=1;
		else sign=-1;


		step_size = stepsize[t2index];
		get_quant(delta, step_size, &q_out, &step_mul);
		index_adj = indexadjust[q_out];
		new_delta = step_size*step_mul;

		prev_pcm_sample = roundOff(sign*new_delta+prev_pcm_sample);

		t2index+=index_adj;
		if(t2index<0) t2index=0;
		if(t2index>88) t2index=88;

		adpcm_sample1 = q_out;

		if(sign==-1) adpcm_sample1 = adpcm_sample1 | 0x08;




		current_pcm_sample = *current;
		current++;

		delta = current_pcm_sample - prev_pcm_sample;
		if(delta>0) sign=1;
		else sign=-1;


		step_size = stepsize[t2index];
		get_quant(delta, step_size, &q_out, &step_mul);
		index_adj = indexadjust[q_out];
		new_delta = step_size*step_mul;

		prev_pcm_sample = roundOff(sign*new_delta+prev_pcm_sample);

		t2index+=index_adj;
		if(t2index<0) t2index=0;
		if(t2index>88) t2index=88;

		adpcm_sample2 = q_out;

		if(sign==-1) adpcm_sample2 = adpcm_sample2 | 0x08;



		TX_BUFFER[i]= adpcm_sample1 << 4;
		TX_BUFFER[i] = TX_BUFFER[i] | (0x0f & adpcm_sample2);

		i++;
	}



}


void ADPCM_DECODE(void)
{

	int i,t2index=0, step_size, sign, index_adj;
	float step_mul;

	signed short first_pcm_sample, prev_pcm_sample, current_pcm_sample;


	char adpcm_sample1, adpcm_sample2;

	unsigned char* current = RCV_BUFFER+2;

		memcpy(&first_pcm_sample, TX_BUFFER, 2);
	WR_BUFFER[0] = first_pcm_sample;


	i=1;
	prev_pcm_sample = first_pcm_sample;

	while(i<MAX_BUF)
	{
		adpcm_sample2 = 0x000f & (*current);
		adpcm_sample1 = 0x000f & (*current >> 4);

		current++;


		if(adpcm_sample1>=8)
		{
			sign=-1;
			adpcm_sample1 = adpcm_sample1 & 0x07;
		}

		else sign=1;

		if(adpcm_sample1>7) {
			return;
			}

		step_size = stepsize[t2index];
		step_mul = (float) 0.25 * (float) adpcm_sample1;

		current_pcm_sample = roundOff((float)sign * (float) step_mul * (float) step_size + (float) prev_pcm_sample);

		prev_pcm_sample = current_pcm_sample;

		index_adj = indexadjust[adpcm_sample1];

		t2index+=index_adj;
		if(t2index<0) t2index=0;
		if(t2index>88) t2index=88;


		WR_BUFFER[i] = current_pcm_sample;

		i++;




		if(adpcm_sample2>=8)
		{
			sign=-1;
			adpcm_sample2 = adpcm_sample2 & 0x07;

		}

		else sign=1;

		step_size = stepsize[t2index];
		step_mul = 0.25 * (float) adpcm_sample2;

		current_pcm_sample = roundOff((float)sign * (float) step_mul * (float) step_size + (float) prev_pcm_sample);
		prev_pcm_sample = current_pcm_sample;

		index_adj = indexadjust[adpcm_sample2];
		t2index+=index_adj;
		if(t2index<0) t2index=0;
		if(t2index>88) t2index=88;


		WR_BUFFER[i] = current_pcm_sample;

		i++;
	}

}
*/


/*
int adpcm_encode(adpcm_state_t state,
				 short pcmSamples[],
				 int nSamples,
				 char* adpcmData,
				 int nBytes) {
    int itr;				// Iterator
    int count;				// Count of output bytes
    int step;				// Stepsize
    int currIndex;			// Current step change index
    int sign;				// Sign
    int outputBuffer;		// Buffers previous 4-bit output
    int isBuffered;			// flag for outputBuffer

	signed char *outPtr;	// Output buffer pointer

	outPtr = (signed char *) adpcmData;

    state->predicted = state->previous;
    currIndex = state->index;
    step = stepsize[currIndex];

    isBuffered = 1;
    count=0;

    for (itr=0; itr < nSamples ; itr++ ) {
		state->sample = *pcmSamples++;

		//Get difference
		state->diff = state->sample - state->predicted;
		sign = (state->diff < 0) ? 8 : 0;
		if ( sign )
			state->diff = (-state->diff);

		//Divide & Clamp
		state->delta = 0;
		state->predDiff = (step >> 3);

		if ( state->diff >= step ) {
			state->delta = 4;
			state->diff-=step;
			state->predDiff += step;
		}
		step >>= 1;
		if ( state->diff >= step  ) {
			state->delta |= 2;
			state->diff -= step;
			state->predDiff += step;
		}
		step >>= 1;
		if ( state->diff >= step ) {
			state->delta |= 1;
			state->predDiff += step;
		}

		//Update previous value
		if ( sign )
		  state->predicted -= state->predDiff;
		else
		  state->predicted += state->predDiff;

		//Clamp previous value to 16 bits
		if ( state->predicted > 32767 )
		  state->predicted = 32767;
		else if ( state->predicted < -32768 )
		  state->predicted = -32768;

		//Save final value, update state
		state->delta |= sign;

		currIndex += indexadjust[state->delta];
		if ( currIndex < 0 )
			currIndex = 0;
		else if ( currIndex > 88 )
			currIndex = 88;
		step = stepsize[currIndex];

		//Write to output buffer
		if ( isBuffered ) {
			outputBuffer = (state->delta << 4) & 0xf0;
		} else {
			*outPtr++ = (state->delta & 0x0f) | outputBuffer;
			count++;
		}
		isBuffered = !isBuffered;
    }

    // Output last step, if needed
    if ( !isBuffered )
      *outPtr++ = outputBuffer;

    state->previous = state->predicted;
    state->index = currIndex;
    return count;
}

int adpcm_decode(adpcm_state_t state,
				 char adpcmData[],
				 int nBytes,
				 short samples[],
				 int nSamples) {
    int itr;				// Iterator
    int count;				// Count of output bytes
	signed char *inPtr;		// Input buffer pointer
    short *outPtr;			// Output buffer pointer
    int sign;				// Current adpcm sign bit
    int step;				// Stepsize
    int currIndex;			// Current step change index
    int ipBuffer;			// place to keep next 4-bit value
    int isBuffered;			// toggle between ipBuffer/input

    int reqd;				// Number of output samples for these nBytes bytes

    reqd = nBytes*2;

    outPtr = samples;
    inPtr = (signed char *)adpcmData;

    state->predicted = state->previous;
    currIndex = state->index;
    step = stepsize[currIndex];

    isBuffered = 0;
    count=0;

    for (itr=0 ; itr < reqd; itr++) {
		//get the delta value
		if ( isBuffered ) {
			state->delta = ipBuffer & 0xf;
		} else {
			ipBuffer = *inPtr++;
			state->delta = (ipBuffer >> 4) & 0xf;
		}
		isBuffered = !isBuffered;

		//Find new currIndex value
		currIndex += indexadjust[state->delta];
		if ( currIndex < 0 )
			currIndex = 0;
		else if ( currIndex > 88 )
			currIndex = 88;

		//Extract the sign and magnitude
		sign = state->delta & 8;
		state->delta = state->delta & 7;

		//find difference and new predicted value
		state->predDiff = step >> 3;
		if ( state->delta & 4 )
			state->predDiff += step;
		if ( state->delta & 2 )
			state->predDiff += step>>1;
		if ( state->delta & 1 )
			state->predDiff += step>>2;

		if ( sign )
		  state->predicted -= state->predDiff;
		else
		  state->predicted += state->predDiff;

		//clamp output value
		if ( state->predicted > 32767 )
		  state->predicted = 32767;
		else if ( state->predicted < -32768 )
		  state->predicted = -32768;

		//Update step value
		step = stepsize[currIndex];

		//Output value
		*outPtr++ = state->predicted;
		count++;
    }

    state->previous = state->predicted;
    state->index = currIndex;

    return count;
}


*/