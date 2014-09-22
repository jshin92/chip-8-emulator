#include "Chip8.h"
#include <stdio.h>
#include <string.h>
#include <cstdlib>


Chip8::Chip8() {
	printf("Initializing Chip8 emulator...\n\n");

	sp = 0x0;
	// programs start at 0x200, since 0x0-0x1FF are reserved for the interpreter
	pc = 0x200;
	I = 0x0;
	fileSize = 0;
	memset(ram, 0, sizeof(ram));
	memset(V, 0, sizeof(V));
	memset(stack, 0, sizeof(stack));
	memset(screen, 0, sizeof(screen));
	delayTimer = 0;
	soundTimer = 0;

	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Chip 8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	// initialize rectangle used for displaying sprites
	r.x = r.y = r.w = r.h = PIXEL_SIZE;

}

void Chip8::run() {
	printf("Reading ROM into ram...\n\n");
	if (!readRom()) {
		printf("ERROR: Could not open ROM file!\n\n");
		return;
	}

	//printRom();
	uint32_t end = RAM_OFFSET + fileSize;

	bool done = false;
	SDL_Event e;

		while (!done) {
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				done = true;
			}
		}
		if (pc < end)
			interp(ram[pc], ram[pc + 1]);

		SDL_RenderPresent(renderer);
		SDL_UpdateWindowSurface(window);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

}

void Chip8::interp(uint8_t x, uint8_t y) {
	printf("< 0x%x > | ", pc);
	printf("[%2x %2x] -> ", x, y);

	uint8_t firstNib = x >> 4;
	switch (firstNib) {
		// TODO: clearing screen and sub routine return
	case 0: {
				if (y == 0xE0) {
					printf("CLR SCRN");
				}
				else if (y == 0xEE) {
					printf("return from sub routine");
				}
				else {
					printf("RCA 1802");
				}
				pc += 2;
				break;
	} 

	case 1: {
				uint16_t newAddr = ((x & 0xF) << 8) | y;
				printf("NEW PC: 0x%x ", newAddr);
				pc = newAddr;
				break;
	} 

		// TODO, finish calling subroutine
	case 2: {
				uint16_t target = ((x & 0xF) << 8) | y;
				printf("CALL SUBR AT %x\n", target);
				// change pc, push addr to stack?
				break;
	} 

	case 3: {
				if (V[x & 0xF] == y) {
					pc += 4;
					printf("SKIP NEXT INSTR");
				}
				else {
					pc += 2;
					printf("INCR PC");
				}
				break;
	} 

	case 4: {
				if (V[x & 0xF] != y) {
					pc += 4;
					printf("SKIP NEXT INSTR");
				}
				else {
					pc += 2;
					printf("INCR PC");
				}
				break;
	} 

	case 5: {
				uint8_t xVal = x & 0xF;
				uint8_t yVal = (y & 0xF0) >> 4;
				if (V[xVal] == V[yVal]) {
					pc += 4;
					printf("SKIP NEXT INSTR");
				}
				else {
					pc += 2;
					printf("INCR PC");
				}
				break;
	} 

	case 6: {
				V[x & 0xF] = y;
				printf("V[%x] = 0x%x", x & 0xF, y);
				pc += 2;
				break;
	} 

	case 7: {
				V[x & 0xF] += y;
				printf("V[%x] += 0x%x", x & 0xF, y);
				pc += 2;
				break;
	} 

	case 8: {
				printf("IMPLEMENT ME!");
				pc += 2;
				break;
	} 

	case 9: {
				if (V[x & 0xF] != V[y >> 4]) {
					pc += 4;
					printf("SKIP NEXT INSTR");
				}
				else {
					pc += 2;
					printf("INCR PC");
				}
				break;
	} 

	case 0xA: {
				  I = ((x & 0xF) << 8) | y;
				  printf("I = 0x%x", I);
				  pc += 2;
				  break;
	} 

	case 0xB: {
				  pc = (((x & 0xF) << 8) | y) + V[0];
				  printf("NEW PC: 0x%x", pc);
				  break;
	} 

	case 0xC: {
				  printf("IMPLEMENT ME");
				  pc += 2;
				  break;
	} 

	case 0xD: {
				  // Draws a sprite at location {Vx, Vy}
				  uint8_t xCoord = V[x & 0xF];
				  uint8_t yCoord = V[y >> 4];
				  uint8_t numBytes = y & 0xF;
				  for (int i = 0; i < numBytes; i++) {
				      uint8_t spriteLineInfo = ram[I + i];
					  r.y = yCoord * PIXEL_SIZE + i * PIXEL_SIZE;
					  for (int j = 0; j < SPRITE_LINE_SIZE; j++) {
						  if ((spriteLineInfo >> (SPRITE_LINE_SIZE - j - 1)) & 0x1) {
							  r.x = xCoord * PIXEL_SIZE + j * PIXEL_SIZE;
							  SDL_RenderFillRect(renderer, &r);
						  }
					  }
				  }

				  printf("DRAWING SPRITE");
				  pc += 2;
				  break;
	} 

	case 0xE: {
				  printf("IMPLEMENT ME");
				  pc += 2;
				  break;
	} 

	case 0xF: {
				  switch (y) {
				  case 7: {
							  V[x & 0xF] = delayTimer;
							  printf("V[%x] = 0x%x", x & 0xF, delayTimer);
							  pc += 2;
							  break;
				  } 

				  case 0xA: {
								printf("IMPLEMENT ME");
								pc += 2;
								break;
				  } 

				  case 15: {
							   delayTimer = V[x & 0xF];
							   printf("delayTimer = 0x%x", delayTimer);
							   pc += 2;
							   break;
				  } 

				  case 18: {
							   soundTimer = V[x & 0xF];
							   printf("soundTimer = 0x%x", soundTimer);
							   pc += 2;
							   break;
				  } 

				  case 0x1E: {
								 I += V[x & 0xF];
								 printf("I += V[0x%x]", x & 0xF);
								 pc += 2;
								 break;
				  } 

				  case 29: {
							   printf("IMPLEMENT ME");
							   pc += 2;
							   break;
				  } 

				  case 33: {
							   printf("IMPLEMENT ME");
							   pc += 2;
							   break;
				  } 

				  case 55: {
							   printf("IMPLEMENT ME");
							   pc += 2;
							   break;
				  } 

				  case 65: {
							   printf("IMPLEMENT ME");
							   pc += 2;
							   break;
				  } 

				  default:
					  printf("Bad opcode for bottom nybble for case 0xF! Fatal error.");
					  exit(-1);
				  }

				  break;
	} 

	default:
		printf("UNRECOGNIZED OPCODE. THIS SHOULD NOT HAPPEN. EXITING...");
		exit(-2);
	}

	printf("\n");
}

bool Chip8::readRom() {
	FILE* f;
	f = fopen("Fishie.ch8", "rb");
	if (!f) return false;

	fseek(f, 0, SEEK_END);
	fileSize = ftell(f);
	printf("Size of Rom: %d bytes\n\n", fileSize);
	fseek(f, 0, SEEK_SET);
	fread(ram + RAM_OFFSET, sizeof(uint8_t), fileSize, f);
	fclose(f);

	return true;
}

void Chip8::printRom() {
	for (unsigned int i = RAM_OFFSET; i < RAM_OFFSET + fileSize; i += 8) {
		printf("0x%x | ", i);
		printf("%2x %2x %2x %2x %2x %2x %2x %2x\n\n", ram[i], ram[i + 1],
			ram[i + 2], ram[i + 3], ram[i + 4], ram[i + 5], ram[i + 6], ram[i + 7]);
	}
}

