#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

#ifndef __KODEK_H
#define __KODEK_H

#define ERR_INSUFFICIENT_BUFFER			-10

//Round off macro
#define roundOff(f) ((f - (int)f)>=0.5? (int)f+1:(int)f)

/*IM-ADPCM CODEC */

#define ADPCM_STATE_INIT NULL

typedef struct _adpcm_state {
	int				nwBound;		/* Is network bound */
    signed  short	previous;		/* Previous output value */
    int 	 		index;			/* Index into stepsize table */

    signed   short 	sample;			/* Current input sample value */
    signed   short 	delta;			/* Difference between samples */
    signed 	 char 	qOut;			/* Current ADPCM output value */
	int		 		isNew;			/* Re-using previous state ? */

	//temp variables
    int 			itr;			/* Iterator */
    int 			count;			/* Count of output bytes */
    int 			step;			/* Stepsize */
    int 			sign;			/* Sign */
    char			outputBuffer;	/* Buffers previous 4-bit output */
    bool			isBuffered;		/* Flag for outputBuffer */
    int 			qVal;			/* New Q */

    float 			mul;			/* Multiplier */
	signed   char 	*bytePtr;		/* Byte buffer pointer */
	signed   short 	*shortPtr;		/* Short buffer pointer */

	//quantization variables
	float 			smpl;
	float 			min;
	float 			max;
} *adpcm_state_t;

//public functions
int adpcm_init(adpcm_state_t *state, int isNwBound);

void adpcm_set(adpcm_state_t state, int first, int index);
void adpcm_reset(adpcm_state_t state);

int adpcm_encode(adpcm_state_t state,
				 short pcmSamples[],
				 int nBytesIn,
				 char adpcmData[],
				 int nBytes);

int adpcm_decode(adpcm_state_t state,
				 char adpcmData[],
				 int nBytes,
				 short samples[],
				 int nSamples);

void adpcm_close(adpcm_state_t state);

#endif