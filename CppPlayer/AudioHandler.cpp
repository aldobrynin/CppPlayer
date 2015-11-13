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
		dataSizeToCopy = dataLeftInBuffer();
		if (dataSizeToCopy > streamSize)
			dataSizeToCopy = streamSize;

		memcpy(stream, (uint8_t *)audioBuffer + audioBufferIndex, dataSizeToCopy);
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

		if (frameFinished)
		{
			dataSize = av_samples_get_buffer_size(NULL, codecContext->channels, frame->nb_samples, codecContext->sample_fmt, 1);
			memcpy(audioBuffer, frame->data[0], dataSize);
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
		clock += (double)dataSize /
			(codecContext->channels * codecContext->sample_rate *
				av_get_bytes_per_sample(codecContext->sample_fmt));
	}
}

AudioHandler::AudioHandler(AVStream* aStream)
{
	isQuit = false;
	packetQueue = new PacketQueue();
	SDL_Init(SDL_INIT_AUDIO);

	audioStream = aStream;

	codecContext = audioStream->codec;
	codec = avcodec_find_decoder(codecContext->codec_id);

	if (codecContext->sample_fmt == AV_SAMPLE_FMT_S16P)
		codecContext->request_sample_fmt = AV_SAMPLE_FMT_S16;

	avcodec_open2(codecContext, codec, nullptr);

	SDL_AudioSpec desiredSpecs;
	SDL_AudioSpec specs;
	desiredSpecs.freq = codecContext->sample_rate;
	desiredSpecs.format = AUDIO_S16SYS;
	desiredSpecs.channels = codecContext->channels;
	desiredSpecs.silence = 0;
	desiredSpecs.samples = 512;
	desiredSpecs.callback = PlaybackCallback;
	desiredSpecs.userdata = this;
	SDL_OpenAudio(&desiredSpecs, &specs);
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
}

AudioHandler::~AudioHandler()
{
	avcodec_close(audioStream->codec);
	avcodec_close(codecContext);

}
