#pragma once
#include "VideoHandler.h"
#include "AudioHandler.h"

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0
class Sync
{
	AudioHandler *audio;
	VideoHandler *video;

	double previousClock;
	double previousDelay;
public:
	Sync(AudioHandler *a, VideoHandler *v) : audio(a), video(v), previousClock(0), previousDelay(0) {}
	double computeFrameDelay();
};
