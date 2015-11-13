#ifndef AUDIO_HANDLER
#define AUDIO_HANDLER

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <SDL.h>
#include "PacketQueue.h"
#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

class AudioHandler
{
	AVCodecContext* codecContext;
	AVCodec* codec;
	AVStream* audioStream;
	PacketQueue* packetQueue;
	AVPacket* audioPacket;
	AVFrame* frame;
	SDL_AudioDeviceID device;
	double clock;

	uint8_t audioBuffer[MAX_AUDIO_FRAME_SIZE];
	unsigned int audioBufferSize;
	unsigned int audioBufferIndex;
	static void PlaybackCallback(void* userdata, Uint8* stream, int streamSize);
	unsigned int dataLeftInBuffer() const;
	void Playback(Uint8* stream, int streamSize);
	int DecodeAudio(uint8_t* audioBuffer);
	void UpdateClock(AVPacket* packet, int dataSize);
public:
	bool isQuit;

	AudioHandler(AVStream* aStream);
	void Start();
	void PutPacket(AVPacket* pkt) const;
	double AudioClock() const;
	void Quit();
	~AudioHandler();
};
#endif
