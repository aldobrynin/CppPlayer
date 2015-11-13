#include "AudioHandler.h"

void AudioHandler::PlaybackCallback(void* userdata, Uint8* stream, int streamSize)
{
	AudioHandler *a = (AudioHandler*)userdata;
	a->Playback(stream, streamSize);
}

unsigned AudioHandler::dataLeftInBuffer() const
{
	return audioBufferSize - audioBufferIndex;
}

void AudioHandler::Playback(Uint8* stream, int streamSize)
{
	int dataSizeToCopy;

	while (streamSize > 0)
	{
		unsigned data_left_in_buffer = dataLeftInBuffer();
		dataSizeToCopy = data_left_in_buffer > streamSize ? streamSize : data_left_in_buffer;
		unsigned char *src = (uint8_t *) (audioBuffer + audioBufferIndex);
		//memcpy(stream, src, dataSizeToCopy);
		SDL_MixAudio(stream, src, dataSizeToCopy, SDL_MIX_MAXVOLUME);

		streamSize -= dataSizeToCopy;
		stream += dataSizeToCopy;
		audioBufferIndex += dataSizeToCopy;

		if (audioBufferIndex >= audioBufferSize)
		{
			audioBufferSize = DecodeAudio(audioBuffer);
			audioBufferIndex = 0;
		}
	}
}

int AudioHandler::DecodeAudio(uint8_t* audioBuffer)
{
	AVPacket audioPacket;
	AVFrame  *frame = av_frame_alloc();
	int frameFinished = 0;

	int audioDecodedSize, dataSize = 0;

	while (!isQuit)
	{
		packetQueue->Get(&audioPacket);

		audioDecodedSize = avcodec_decode_audio4(codecContext, frame, &frameFinished, &audioPacket);
		if(audioDecodedSize < 0)
		{
			av_free_packet(&audioPacket);
			fprintf(stderr, "Failed to decode audio frame\n");
			break;
		}
		if (frameFinished)
		{
			dataSize = av_samples_get_buffer_size(NULL, codecContext->channels, frame->nb_samples, codecContext->sample_fmt, 1);
			//memcpy(audioBuffer, frame->data[0], dataSize);
			SDL_MixAudio(audioBuffer, frame->data[0], dataSize, SDL_MIX_MAXVOLUME);

			UpdateClock(&audioPacket, dataSize);
			av_free_packet(&audioPacket);
		}
		return dataSize;
	}
	av_free(frame);
	return 0;
}

void AudioHandler::UpdateClock(AVPacket* packet, int dataSize)
{
	if (packet->dts != AV_NOPTS_VALUE)
		clock = av_q2d(audioStream->time_base) * packet->dts;
	else if (frame->pkt_pts != AV_NOPTS_VALUE)
		clock = av_q2d(audioStream->time_base) * frame->pkt_pts;
	else
	{
		clock += (double) dataSize /
			(codecContext->channels * codecContext->sample_rate *
				av_get_bytes_per_sample(codecContext->sample_fmt));
	}
}

AudioHandler::AudioHandler(AVStream* aStream)
{
	isQuit = false;
	packetQueue = new PacketQueue();
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(0);
	}
	audioStream = aStream;
	codecContext = audioStream->codec;
	codec = avcodec_find_decoder(codecContext->codec_id);
	if (codec == nullptr) {
		fprintf(stderr, "Audio codec not found or unsupported.\n");
		exit(1);
	}	if (codecContext->sample_fmt == AV_SAMPLE_FMT_S16P)
		codecContext->request_sample_fmt = AV_SAMPLE_FMT_S16;

	if (avcodec_open2(codecContext, codec, nullptr) < 0) {
		fprintf(stderr, "Could not open audiocodec.\n");
		exit(1);
	}
	SDL_AudioSpec desiredSpecs;
	SDL_AudioSpec specs;
	desiredSpecs.freq = codecContext->sample_rate;
	desiredSpecs.format = AUDIO_S16SYS;
	desiredSpecs.channels = codecContext->channels;
	desiredSpecs.silence = 0;
	desiredSpecs.samples = SDL_AUDIO_BUFFER_SIZE;
	desiredSpecs.callback = PlaybackCallback;
	desiredSpecs.userdata = this;
	if (SDL_OpenAudio(&desiredSpecs, &specs) < 0) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		exit(1);
	}
}

void AudioHandler::Start()
{
	SDL_PauseAudio(0);
}

void AudioHandler::PutPacket(AVPacket* pkt) const
{
	packetQueue->Put(pkt);
}

double AudioHandler::AudioClock() const
{
	int bytes_per_sec = codecContext->sample_rate * 
		codecContext->channels *
		av_get_bytes_per_sample(codecContext->sample_fmt);

	int audioBufferDataSize = audioBufferSize - audioBufferIndex;

	if (bytes_per_sec != 0)
		return clock - (double)audioBufferDataSize / bytes_per_sec;
	return clock;
}

void AudioHandler::Quit()
{
	isQuit = true;
	SDL_CloseAudio();
}

AudioHandler::~AudioHandler()
{
	avcodec_close(audioStream->codec);
	avcodec_close(codecContext);
	delete packetQueue;
}
