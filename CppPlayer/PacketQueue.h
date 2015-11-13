#ifndef PACKET_QUEUE
#define PACKET_QUEUE

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#include <SDL.h>

class PacketQueue
{
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;
	SDL_cond *cond;

public:
	PacketQueue();
	int Put(AVPacket* pkt);
	int Get(AVPacket* pkt);
	int getSize() const
	{return size;}
};

#endif