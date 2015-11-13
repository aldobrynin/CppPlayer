#include "Player.h"

#undef main

int main(int argc, char* argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Missing input file.\nUsage: %s <filename>\n", argv[0]);
		return -1;
	}
	Player *player = new Player();
	player->Open(argv[1]);
	player->Play();
	delete player;
	return 0;
}