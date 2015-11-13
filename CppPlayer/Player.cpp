#include "Player.h"

Player::Player(): 
	video(nullptr), audio(nullptr), sync(nullptr)
{
	isQuit = false;
	pFormatCtx = nullptr;
	videoStreamIndex=-1;
	audioStreamIndex=-1;
	if (SDL_Init(SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL\n");
		exit(1);
	}
	av_register_all();
}

Player::~Player()
{
	avformat_close_input(&pFormatCtx);
	SDL_Quit();
	delete audio;
	delete video;
	delete sync;
}

void Player::Open(char *filename)
{
	if (avformat_open_input(&pFormatCtx, filename, nullptr, nullptr) != 0){
		fprintf(stderr, "Couldn't open input file: %s.\n", filename);
		exit(1);
	}
	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0){
		fprintf(stderr, "Couldn't find stream information.\n");
		exit(1);
	}
	av_dump_format(pFormatCtx, 0, filename, 0);

	videoStreamIndex = getStreamID(AVMEDIA_TYPE_VIDEO);
	audioStreamIndex = getStreamID(AVMEDIA_TYPE_AUDIO);

	if (videoStreamIndex == -1 || audioStreamIndex == -1){
		fprintf(stderr, "Couldn't find audio or video stream.\n");
		exit(1);
	}

	video = new VideoHandler(pFormatCtx->streams[videoStreamIndex]);
	audio = new AudioHandler(pFormatCtx->streams[audioStreamIndex]);
	sync = new Sync(audio, video);
}

void Player::Play()
{
	SDL_AddTimer(10, TimerCallback, video);
	SDL_Thread *demuxThread = SDL_CreateThread(DemuxThread, "demux", this);
	SDL_Thread *videoThread = video->Start();
	audio->Start();
	EventListener();
	SDL_WaitThread(demuxThread, nullptr);
	SDL_WaitThread(videoThread, nullptr);
}

int Player::Quit()
{
	isQuit = true;
	audio->isQuit=true;
	video->isQuit=true;
	audio->Quit();
	video->Quit();
	return 0;
}

int Player::getStreamID(AVMediaType type) const
{
	unsigned int i;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == type)
			return i;

	return -1;
}

int Player::DemuxThread(void* arg)
{
	Player *p = static_cast<Player*>(arg);
	p->Demux();
	return 0;
}

int Player::Demux() const
{
	AVPacket packet;
	while (av_read_frame(pFormatCtx, &packet) >= 0 && !isQuit)
	{
		if (packet.stream_index == videoStreamIndex)
			video->PutPacket(&packet);
		else if (packet.stream_index == audioStreamIndex)
			audio->PutPacket(&packet);
	}
	return 0;
}

int Player::EventListener()
{
	SDL_Event event;
	while (!isQuit)
	{
		if (SDL_WaitEvent(&event)) {
			if (event.type == SDL_QUIT) {
				Quit();
			}
			if (event.type == SDL_USEREVENT)
			{
				Uint32 delay = (int)(sync->computeFrameDelay() * 500);
				SDL_AddTimer(delay, TimerCallback, nullptr);
				video->RenderPicture();
			}
		}
	}
	return 0;
}

Uint32 Player::TimerCallback(Uint32 interval, void* opaque)
{	
	SDL_Event event;
	event.type = SDL_USEREVENT;
	SDL_PushEvent(&event);
	return 0;
}