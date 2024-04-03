#include "Game.h"

Game::Game(int window_width, int window_height, Vector2Int _blockAmount, int _lives) 
	: window(nullptr), WINDOW_WIDTH(window_width), WINDOW_HEIGHT(window_height),
	  renderer(nullptr), isRunning(false), isPaused(false), ticksCount(0), lives(_lives), maxLives(_lives),
	  blockAmount(_blockAmount)
{
	createBlocks();

	// CONFIG //
	paddle.initialPos = Vector2{ WINDOW_WIDTH / 2.0f - PADDLE_SIZE.x / 2.0f, static_cast<float>(WINDOW_HEIGHT - PADDLE_SIZE.y * 2.0f) };
	paddle.moveDir = 0;
	paddle.speed = 300.0f;
	paddle.pos = paddle.initialPos;

	float distancePaddleBlocks = paddle.pos.y - blocks[blockAmount.x - 1][blockAmount.y - 1].pos.y;

	ball.pos = Vector2{ WINDOW_WIDTH / 2.0f - 10.0f / 2.0f, WINDOW_HEIGHT - 10.0f / 2.0f - distancePaddleBlocks / 2.0f };
	ball.speed = 200.0f;
	ball.velocity = Vector2{ ball.speed, ball.speed };
	// ------ //

	currentState = STARTING;
}

bool Game::initialize() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return false;
	}

	window = SDL_CreateWindow("Breakout", 50, 50, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	if (!window) {
		SDL_Log("Failed to create window: %s", SDL_GetError());
		return false;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		SDL_Log("Failed to create renderer: %s", SDL_GetError());
		return false;
	}

	if (TTF_Init() < 0) {
		SDL_Log("Error initializing SDL_ttf: %s", TTF_GetError());
	}

	font = TTF_OpenFont("font.ttf", 25);
	if (!font) {
		SDL_Log("Error loading font: %s", TTF_GetError());
	}

	isRunning = true;
	currentState = STARTING;
	return true;
}

void Game::createBlocks() {
	blocks.clear();
	totalBlockAmount = 0;

	for (int x = 0; x < blockAmount.x; x++) {
		std::vector<Block> blocks_x = std::vector<Block>();
		for (int y = 0; y < blockAmount.y; y++) {
			Vector2 blockPos{
				BLOCKS_MARGIN.x + BLOCK_SEPARATION.x * x + BLOCK_SIZE.x * x,
				BLOCKS_MARGIN.y + BLOCK_SEPARATION.y * y + BLOCK_SIZE.y * y
			};

			Block block;
			block.pos = blockPos;

			totalBlockAmount++;
			blocks_x.push_back(block);
		}
		blocks.push_back(blocks_x);
	}
}

void Game::resetGame() {
	blocksDestroyed = 0;
	lives = maxLives;
	paddle.pos = paddle.initialPos;
	createBlocks();
	currentState = STARTING;
}

void Game::runLoop() {
	while (isRunning) {
		processInput();
		updateGame();
		generateOutput();
	}
}

void Game::processInput() {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			isRunning = false;
			break;

		case SDL_KEYDOWN:
			// check for space bar -> start game or r key -> reset game
			switch (event.key.keysym.sym) {
			case SDLK_SPACE:
				// check what the current state is
				switch (currentState) {
				// start game
				case STARTING: case DEAD:
					currentState = PLAYING;
					ball.velocity = Vector2{ 0.0f, -ball.speed };
					break;
				// reset game
				case GAME_OVER: case WIN:
					resetGame();
					break;
				}
				break;
			// reset game
			case SDLK_r:
				resetGame();
				break;
			// toggle pause
			case SDLK_p:
				if (currentState == PLAYING) {
					isPaused = !isPaused;
				}
				break;
			}
			break;
		}
	}

	const Uint8* kbState = SDL_GetKeyboardState(NULL);
	if (kbState[SDL_SCANCODE_ESCAPE]) {
		isRunning = false;
	}

	// paddle movement //
	paddle.moveDir = 0;
	if (currentState != GAME_OVER && currentState != WIN) {
		if (kbState[SDL_SCANCODE_A] || kbState[SDL_SCANCODE_LEFT]) {
			paddle.moveDir -= 1;
		}
		if (kbState[SDL_SCANCODE_D] || kbState[SDL_SCANCODE_RIGHT]) {
			paddle.moveDir += 1;
		}
	}
	// --- //
}

void Game::updateGame() {
	// -- delta time -- //
	while (!SDL_TICKS_PASSED(SDL_GetTicks(), ticksCount + 10));

	float deltaTime = (SDL_GetTicks() - ticksCount) / 1000.0f;

	if (deltaTime > 0.05f)
	{
		deltaTime = 0.05f;
	}

	ticksCount = SDL_GetTicks();
	// ------------------------- //

	if (isPaused) {
		return;
	}

	// move ball
	if (currentState == PLAYING) {
		ball.pos.x += ball.velocity.x * deltaTime;
		ball.pos.y += ball.velocity.y * deltaTime;
	}
	else {
		ball.pos.x = paddle.pos.x + PADDLE_SIZE.x / 2.0f - 10.0f / 2.0f;
		ball.pos.y = paddle.pos.y - PADDLE_SIZE.y * 2.0f;
	}

	// move paddle
	paddle.pos.x += paddle.moveDir * paddle.speed * deltaTime;
	if (paddle.pos.x < 0) {
		paddle.pos.x = 0;
	}
	else if (paddle.pos.x > WINDOW_WIDTH - PADDLE_SIZE.x) {
		paddle.pos.x = WINDOW_WIDTH - PADDLE_SIZE.x;
	}

	if (currentState != PLAYING) {
		return;
	}

	// BALL COLLISIONS
	SDL_Rect ballRect{
		ball.pos.x, ball.pos.y,
		10.0f, 10.0f
	};
	SDL_Rect paddleRect{
		paddle.pos.x, paddle.pos.y,
		PADDLE_SIZE.x, PADDLE_SIZE.y
	};

	// paddle
	if (SDL_HasIntersection(&ballRect, &paddleRect)) {
		ball.pos.y = paddle.pos.y - 10.0f;
		ball.velocity.y *= -1.0f;

		if (ball.pos.x < paddle.pos.x + PADDLE_SIZE.x / 3.0f) {
			ball.velocity.x = -ball.speed;
		}
		else if (ball.pos.x > paddle.pos.x + PADDLE_SIZE.x * (2.0f / 3.0f)) {
			ball.velocity.x = ball.speed;
		}
	}
	
	// left and right walls
	if (ball.pos.x < 0 || ball.pos.x >= WINDOW_WIDTH - 10.0f) {
		ball.pos.x = SDL_clamp(ball.pos.x, 0, WINDOW_WIDTH - 10.0f); // put ball back in map
		ball.velocity.x *= -1.0f; // bounce away from wall
	}

	// top and bottom walls
	if (ball.pos.y < 0) {
		ball.pos.y = 0;
		ball.velocity.y *= -1.0f;
	}
	// ball touched bottom -> DEAD
	else if (ball.pos.y >= WINDOW_HEIGHT - 10.0f) {
		ball.velocity = Vector2{ 0.0f, 0.0f };

		paddle.pos = paddle.initialPos;
		paddle.moveDir = 0;
		
		// check if GAME OVER
		lives -= 1;
		if (lives <= 0) {
			currentState = GAME_OVER;
			paddle.pos = paddle.initialPos;
		}
		else {
			currentState = DEAD;
		}
		return;
	}

	// with blocks
	for (auto &x : blocks) {
		for (auto &block : x) {
			if (block.isBroken) {
				continue; // block is already broken
			}

			SDL_Rect blockRect{
				block.pos.x, block.pos.y, BLOCK_SIZE.x, BLOCK_SIZE.y
			};
			if (SDL_HasIntersection(&ballRect, &blockRect)) {
				block.isBroken = true;

				// check if velocity should be altered
				if (ball.pos.x <= block.pos.x || ball.pos.x + 10.0f >= block.pos.x + BLOCK_SIZE.x) {
					ball.velocity.x *= -1.0f;
				}
				if (ball.pos.y <= block.pos.y || ball.pos.y + 10.0f >= block.pos.y + BLOCK_SIZE.y) {
					ball.velocity.y *= -1.0f;
				}

				// check win state
				blocksDestroyed++;
				if (blocksDestroyed >= totalBlockAmount) {
					currentState = WIN;
					paddle.pos = paddle.initialPos;
					return;
				}

				break;
			}
		}
	}
}

void Game::generateOutput() {
	SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	SDL_RenderClear(renderer);

	SDL_SetRenderDrawColor(renderer, fgColor.r, fgColor.g, fgColor.b, fgColor.a);	//	OBJECTS:	WHITE	//

	SDL_Rect walls = {
		0, 0,
		WINDOW_WIDTH, WINDOW_HEIGHT
	};
	SDL_RenderDrawRect(renderer, &walls);

	for (auto x : blocks) {
		for (auto block : x) {
			if (block.isBroken) {
				continue;
			}
			SDL_Rect blockRect{
				block.pos.x, block.pos.y,
				BLOCK_SIZE.x, BLOCK_SIZE.y
			};
			SDL_RenderFillRect(renderer, &blockRect);
		}
	}

	SDL_Rect paddleRect{
		(int)paddle.pos.x, (int)paddle.pos.y,
		PADDLE_SIZE.x, PADDLE_SIZE.y
	};
	SDL_RenderFillRect(renderer, &paddleRect);

	SDL_Rect ballRect{
		(int)ball.pos.x, (int)ball.pos.y,
		10.0f, 10.0f
	};
	SDL_RenderFillRect(renderer, &ballRect);

	std::string textString;
	SDL_Surface* textSurface = nullptr;
	SDL_Rect textDest;

	switch (currentState) {
	case STARTING:
		textString = "SPACE TO START";
		textSurface = TTF_RenderText_Solid(font, textString.c_str(), textColor);
		text = SDL_CreateTextureFromSurface(renderer, textSurface);
		textDest = {
			WINDOW_WIDTH / 2 - textSurface->w, WINDOW_HEIGHT / 2 - textSurface->h / 2,
			textSurface->w * 2, textSurface->h * 2
		};
		SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		SDL_RenderFillRect(renderer, &textDest);
		break;

	case PLAYING: case DEAD:
		textString = (isPaused) ? "PAUSED" : "LIVES:" + std::to_string(lives);
		textSurface = TTF_RenderText_Solid(font, textString.c_str(), textColor);
		text = SDL_CreateTextureFromSurface(renderer, textSurface);
		textDest = {
			5, 0,
			textSurface->w, textSurface->h
		};
		break;
	
	case GAME_OVER:
		textString = "GAME OVER";
		textSurface = TTF_RenderText_Solid(font, textString.c_str(), textColor);
		text = SDL_CreateTextureFromSurface(renderer, textSurface);
		textDest = {
			WINDOW_WIDTH / 2 - textSurface->w, WINDOW_HEIGHT / 2 - textSurface->h / 2,
			textSurface->w * 2, textSurface->h * 2
		};
		SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		SDL_RenderFillRect(renderer, &textDest);
		break;

	case WIN:
		textString = "YOU WIN";
		textSurface = TTF_RenderText_Solid(font, textString.c_str(), textColor);
		text = SDL_CreateTextureFromSurface(renderer, textSurface);
		textDest = {
			WINDOW_WIDTH / 2 - textSurface->w, WINDOW_HEIGHT / 2 - textSurface->h / 2,
			textSurface->w * 2, textSurface->h * 2
		};
		SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		SDL_RenderFillRect(renderer, &textDest);
		break;
	}

	SDL_RenderCopy(renderer, text, NULL, &textDest);
	SDL_DestroyTexture(text);
	SDL_FreeSurface(textSurface);

	SDL_RenderPresent(renderer);
}

void Game::shutdown() {
	TTF_CloseFont(font);
	TTF_Quit();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}