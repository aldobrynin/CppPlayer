#ifndef VIDEO_HANDLER
#define VIDEO_HANDLER

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
}
#include <SDL.h>
#include "PacketQueue.h"

class VideoHandler
{
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	SDL_Window* screen;
	AVCodecContext* codecContext;
	AVCodec* codec;
	AVStream* videoStream;
	PacketQueue* packetQueue;
	SDL_mutex* pictureMutex;
	SDL_cond* pictureReadyCond;
	int isPictureReady;
	double clock;
	struct SwsContext* swsContext;

	double UpdateClock(AVFrame* frame);
	static int DecodeVideoThread(void* arg);
	int DecodeVideo();
	uint8_t* ConvertToYUV420Format(AVFrame* frame, AVCodecContext* codecContext);
	void PreparePicture(AVFrame* frame, AVCodecContext* codecContext);

public:
	bool isQuit;

	VideoHandler(AVStream* vStream);
	~VideoHandler();
	SDL_Thread* Start();
	void PutPacket(AVPacket* pkt) const;
	double VideoClock() const;
	void RenderPicture();
	void Quit();
};
#endif