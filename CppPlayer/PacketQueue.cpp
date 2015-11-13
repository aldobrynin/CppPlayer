#include "PacketQueue.h"

PacketQueue::PacketQueue()
{
	mutex = SDL_CreateMutex();
	cond = SDL_CreateCond();
	first_pkt = nullptr;
	last_pkt = nullptr;
	nb_packets = 0;
	size = 0;
}

int PacketQueue::Put(AVPacket* pkt)
{
	AVPacketList *newP;
	if (av_dup_packet(pkt) < 0)
		return -1;

	newP = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	newP->pkt = *pkt;
	newP->next = nullptr;
  
	SDL_LockMutex(mutex);

	if (!last_pkt)
		first_pkt = newP;
	else
		last_pkt->next = newP;

	last_pkt = newP;
	nb_packets++;
	size += newP->pkt.size;
	SDL_CondSignal(cond);
	SDL_UnlockMutex(mutex);
	return 0;
}

int PacketQueue::Get(AVPacket* pkt)
{
	AVPacketList *pkt1;
	int ret;

	SDL_LockMutex(mutex);

	for(;;)
	{
		pkt1 = first_pkt;
		if (pkt1)
		{
			first_pkt = pkt1->next;
			if (!first_pkt)
				last_pkt = nullptr;
			nb_packets--;
			size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		}
		SDL_CondWait(cond, mutex);
	}
	SDL_UnlockMutex(mutex);
	return ret;
}