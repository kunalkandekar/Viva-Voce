#ifndef __ULAW_H
#define __ULAW_H

/*
    Sun compatible u-law to linear conversion
*/

extern unsigned short   u2s[];
extern unsigned char    s2u[];

#define audio_u2s(x)  (u2s[  (unsigned  char)(x)       ])
#define audio_s2u(x)  (s2u[ ((unsigned short)(x)) >> 3 ])

#define ZEROTRAP    /* turn on the trap as per the MIL-STD */
#define BIAS 0x84   /* define the add-in bias for 16 bit samples */
#define CLIP 32635

int linear2ulaw(short* samples, unsigned char* ulawbytes, int nSamples);

int ulaw2linear( unsigned char* ulawbytes, short* samples, int nSamples);

#endif