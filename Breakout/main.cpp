#include "Game.h"

int main(int argc, char** argv) {
	Game game(760, 600, Vector2Int{ 12, 15 }, 3);
	if (game.initialize()) {
		game.runLoop();
	}
	game.shutdown();

	return 0;
}