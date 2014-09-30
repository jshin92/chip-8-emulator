#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>


#define SCREEN_SIZE 2048
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define RAM_SIZE 4096
#define NUM_REGS 16
#define STACK_SIZE 48
#define RAM_OFFSET 0x200
#define SPRITE_LINE_SIZE 8
#define PIXEL_SIZE 10
#define CH8_RAND_MAX 256

class Chip8	{
public:
	Chip8();
	void run();
	bool readRom();
	void interp(uint8_t x, uint8_t y);
	void printRom();
	void clearScreen();
	void drawActivePixels();

private:
	void placeFontsInMemory();
	SDL_Scancode getKeyMapping(uint8_t k);
	uint8_t getInverseKeyMapping(SDL_Scancode s);
	bool validKey(SDL_Scancode s);
	

	uint8_t ram[RAM_SIZE];
	// registers V0-VF
	uint8_t V[NUM_REGS];
	uint16_t stack[STACK_SIZE];
	uint8_t sp;
	uint16_t pc;
	uint16_t I;
	// screen is 64 x 32 = 2048; {w x h}
	uint8_t screen[SCREEN_SIZE];
	
	size_t fileSize;
	uint8_t delayTimer;
	uint8_t soundTimer;

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Rect r;



};
