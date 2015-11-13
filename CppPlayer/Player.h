#include "VideoHandler.h"
#include "AudioHandler.h"
#include "Sync.h"

extern "C"
{
#include <libavformat/avformat.h>
}

#include <SDL.h>

class Player
{
	AVFormatContext *pFormatCtx;
	VideoHandler *video;
	AudioHandler *audio;
	Sync *sync;

	int videoStreamIndex;
	int audioStreamIndex;
	bool isQuit;

	int getStreamID(AVMediaType type) const;
	static int DemuxThread(void* arg);
	int Demux() const;
	int EventListener();
	static Uint32 TimerCallback(Uint32 interval, void* userdata);

public:
	Player();
	~Player();
	void Open(char *filename);
	void Play();
	int Quit();
};