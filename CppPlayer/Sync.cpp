#include "Sync.h"

double Sync::computeFrameDelay()
{
	double delay = video->VideoClock() - previousClock;
	if (delay <= 0.0 || delay >= 1.0)
		delay = previousDelay;
	previousClock = video->VideoClock();
	previousDelay = delay;

	double diff = video->VideoClock() - audio->AudioClock();

	if (diff <= -delay)		delay = 0;				
	if (diff >= delay)		delay = 2 * delay;

	return delay;
}
