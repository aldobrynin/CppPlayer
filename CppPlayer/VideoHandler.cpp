#include "VideoHandler.h"

VideoHandler::VideoHandler(AVStream* vStream): swsContext(nullptr)
{
	isQuit = false;
	packetQueue = new PacketQueue();

	if (SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	videoStream = vStream;
	codecContext = videoStream->codec;
	clock = 0;

	codec = avcodec_find_decoder(codecContext->codec_id);
	if (codec == nullptr) {
		fprintf(stderr, "Codec not found or unsupported.\n");
		exit(1);
	}
	if (avcodec_open2(codecContext, codec, nullptr) < 0) {
		fprintf(stderr, "Could not open codec.\n");
		exit(1);
	}

	screen = SDL_CreateWindow(
		"Player",
	    SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED,
	    codecContext->width, 
		codecContext->height,
	    SDL_WINDOW_OPENGL);

	if (!screen) {
		fprintf(stderr, "SDL: could not create window - exiting: %s\n", SDL_GetError());
		exit(1);
	}

	renderer = SDL_CreateRenderer(screen, -1, 0);
	if (!renderer) {
		fprintf(stderr, "SDL: could not create renderer: %s\n", SDL_GetError());
		exit(1);
	}

	texture = SDL_CreateTexture(
		renderer,
	    SDL_PIXELFORMAT_IYUV,
	    SDL_TEXTUREACCESS_STREAMING,
	    codecContext->width, 
		codecContext->height);
	if (!texture) {
		fprintf(stderr, "SDL: could not create texture: %s\n", SDL_GetError());
		exit(1);
	}

	pictureMutex = SDL_CreateMutex();
	pictureReadyCond = SDL_CreateCond();
	isPictureReady = false;
}

SDL_Thread* VideoHandler::Start()
{
	return SDL_CreateThread(DecodeVideoThread, "video", this);
}

int VideoHandler::DecodeVideoThread(void* arg)
{
	VideoHandler *v = (VideoHandler*)(arg);
	v->DecodeVideo();
	return 0;
}

int VideoHandler::DecodeVideo()
{
	AVPacket videoPacket;
	AVFrame  *frame = av_frame_alloc();
	int frameFinished;
	while (!isQuit)
	{
		packetQueue->Get(&videoPacket);
		if (avcodec_decode_video2(codecContext, frame, &frameFinished, &videoPacket) < 0)
		{
			fprintf(stderr, "Error in decoding video frame.\n");
			exit(1);
		}
		if (frameFinished)
		{
			UpdateClock(frame);
			PreparePicture(frame, codecContext);
			av_free_packet(&videoPacket);
		} 
	}
	av_frame_free(&frame);			
	return 0;
}

void VideoHandler::PutPacket(AVPacket* pkt) const
{
	packetQueue->Put(pkt);
}

double VideoHandler::VideoClock() const
{
	return clock;
}

void VideoHandler::PreparePicture(AVFrame* frame, AVCodecContext* codecContext)
{
	SDL_LockMutex(pictureMutex);
	while (isPictureReady)
		SDL_CondWait(pictureReadyCond, pictureMutex);
	SDL_UnlockMutex(pictureMutex);

	int pitch = codecContext->width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_IYUV);
	uint8_t * buffer = ConvertToYUV420Format(frame, codecContext);
	SDL_UpdateTexture(texture, nullptr, buffer, pitch);

	SDL_LockMutex(pictureMutex);
	isPictureReady = true;
	SDL_CondSignal(pictureReadyCond);
	SDL_UnlockMutex(pictureMutex);
}

void VideoHandler::RenderPicture()
{
	SDL_LockMutex(pictureMutex);
	while (!isPictureReady)
		SDL_CondWait(pictureReadyCond, pictureMutex);
	SDL_UnlockMutex(pictureMutex);

	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);

	SDL_LockMutex(pictureMutex);			
	isPictureReady = false;
	SDL_CondSignal(pictureReadyCond);
	SDL_UnlockMutex(pictureMutex);
}

uint8_t* VideoHandler::ConvertToYUV420Format(AVFrame* frame, AVCodecContext* codecContext)
{
	static int numPixels = avpicture_get_size (AV_PIX_FMT_YUV420P, codecContext->width, codecContext->height);
	static uint8_t * buffer = (uint8_t *)malloc(numPixels);

	swsContext = nullptr;
	swsContext = sws_getCachedContext(
		swsContext,
		codecContext->width,
		codecContext->height,
		codecContext->pix_fmt,
		codecContext->width,
		codecContext->height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		nullptr,
		nullptr,
		nullptr
	);

	AVFrame* frameYUV420P = av_frame_alloc();
	avpicture_fill(
		(AVPicture *)frameYUV420P,
		buffer,
		AV_PIX_FMT_YUV420P,
		codecContext->width,
		codecContext->height
		);

	sws_scale(
		swsContext,
		frame->data,
		frame->linesize,
		0,
		codecContext->height,
		frameYUV420P->data,
		frameYUV420P->linesize
	);
	av_frame_free(&frameYUV420P);
	return buffer;
}

void VideoHandler::Quit()
{
	isQuit = true;
	avcodec_close(codecContext);
	sws_freeContext(swsContext);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(screen);
	SDL_Quit();
}

double VideoHandler::UpdateClock(AVFrame* frame)
{
	if (frame->pkt_dts != AV_NOPTS_VALUE)
		clock = av_q2d(videoStream->time_base) * frame->pkt_dts;
	else if (frame->pkt_pts != AV_NOPTS_VALUE)
		clock = av_q2d(videoStream->time_base) * frame->pkt_pts;

	double frame_delay = av_q2d(codecContext->time_base);
	frame_delay += frame->repeat_pict * (frame_delay * 0.5);
	clock += frame_delay;

	return clock;
}


VideoHandler::~VideoHandler()
{
	delete packetQueue;
}