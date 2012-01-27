#ifndef __AUDIORW_H
#define __AUDIORW_H

#include "Common.h"

class AudioReader : public Thread {
	Common *common;
public:
	AudioReader();
	AudioReader(Common* common);
	virtual ~AudioReader();

	void* run(void* arg);
};

class AudioWriter : public Thread {
	Common *common;
public:
	AudioWriter();
	AudioWriter(Common* common);
	virtual ~AudioWriter();

	void* run(void* arg);
};
#endif
