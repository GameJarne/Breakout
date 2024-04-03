#pragma once

#include "SDL.h"
#include <vector>
#include "SDL_ttf.h"
#include <string>

struct Vector2 {
	float x, y;
};

struct Vector2Int {
	int x, y;
};

struct Block {
	Vector2 pos;
	bool isBroken = false;
};

struct Paddle {
	Vector2 pos;
	Vector2 initialPos;
	int moveDir; // -1 is left | +1 is right
	float speed;
};

struct Ball {
	Vector2 pos;
	Vector2 velocity;
	float speed;
};

class Game
{
public:
	enum GameState {
		STARTING,
		PLAYING,
		DEAD,
		GAME_OVER,
		WIN
	};

	Game(int window_width, int window_height, Vector2Int _blockAmount, int _lives);

	bool initialize();
	void runLoop();
	void shutdown();

private:
	void createBlocks();
	void resetGame();

	void processInput();
	void updateGame();
	void generateOutput();

	SDL_Window* window;
	const int WINDOW_WIDTH;
	const int WINDOW_HEIGHT;
	SDL_Renderer* renderer;

	bool isRunning;
	bool isPaused;
	
	SDL_Color bgColor = { 0, 0, 255, 255 };
	SDL_Color fgColor = { 255, 255, 255, 255 };
	SDL_Color textColor = { 255, 255, 255, 255 };

	TTF_Font* font;
	SDL_Texture* text;

	Uint32 ticksCount;

	GameState currentState;

	Vector2Int blockAmount;
	int totalBlockAmount;
	int blocksDestroyed;
	std::vector<std::vector<Block>> blocks;

	Paddle paddle;
	Ball ball;

	// GAME //
	int lives;
	int maxLives;
	// ---- //

	// CONFIG //
	const Vector2Int BLOCK_SEPARATION = { 3, 3 };
	const Vector2Int BLOCKS_MARGIN = { 50, 50 };
	const Vector2Int BLOCK_SIZE = { 50, 20 };

	const Vector2Int PADDLE_SIZE = { 70, 15 };
	// ------ //
};