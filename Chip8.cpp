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
	placeFontsInMemory();
	srand(time(NULL));	
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
	case 0: {
				if (y == 0xE0) {
					printf("CLR SCRN");
				}
				else if (y == 0xEE) {
					sp--;
					pc = stack[sp];
					printf("RET FROM SUBROUTINE");
				}
				else {
					printf("RCA 1802");
				}
				pc += 2;
	} break;

	case 1: {
				uint16_t newAddr = ((x & 0xF) << 8) | y;
				printf("NEW PC: 0x%x ", newAddr);
				pc = newAddr;
	} break;

	case 2: {
				uint16_t target = ((x & 0xF) << 8) | y;
				stack[sp] = pc;
				sp++;
				pc = target;
				printf("CALL SUBR AT %x", target);
	} break;

	case 3: {
				if (V[x & 0xF] == y) {
					pc += 4;
					printf("SKIP NEXT INSTR");
				}
				else {
					pc += 2;
					printf("INCR PC");
				}
	} break;

	case 4: {
				if (V[x & 0xF] != y) {
					pc += 4;
					printf("SKIP NEXT INSTR");
				}
				else {
					pc += 2;
					printf("INCR PC");
				}
	} break;

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
	} break;

	case 6: {
				V[x & 0xF] = y;
				printf("V[%x] = 0x%x", x & 0xF, y);
				pc += 2;
	} break;

	case 7: {
				V[x & 0xF] += y;
				printf("V[%x] += 0x%x", x & 0xF, y);
				pc += 2;
	} break;

	case 8: {
				switch (y & 0xF) {
				case 0: {
							V[x & 0xF] = V[y >> 4];
							printf("V[%x] = 0x%x", x & 0xF, V[y >> 4]);
				} break;
				case 1: {
							V[x & 0xF] |= V[y >> 4];
							printf("V[%x] |= 0x%x", x & 0xF, V[y >> 4]);
				} break;
				case 2: {
							V[x & 0xF] &= V[y >> 4];
							printf("V[%x] &= 0x%x", x & 0xF, V[y >> 4]);
				} break;
				case 3: {
							V[x & 0xF] ^= V[y >> 4];
							printf("V[%x] ^= 0x%x", x & 0xF, V[y >> 4]);
				} break;
				case 4: {
							uint8_t msb = V[x & 0xF] >> 7;
							V[x & 0xF] += V[y >> 4];
							V[0xF] = msb == 1 && ((V[x & 0xF] >> 7) == 0) ? 1 : 0;
							printf("V[%x] += %x ; VF set to %x", x & 0xF, V[y >> 4], V[0xF]);
				} break;
				case 5: {
							V[0xF] = V[x & 0xF] > V[y >> 4];
							V[x & 0xF] -= V[y >> 4];
							printf("V[%x] -= 0x%x; VF = %x", x & 0xF, V[y >> 4], V[0xF]);
				} break;
				case 6: {
							V[0xF] = V[x & 0xF] & 0x1;
							V[x & 0xF] >>= 1;
							printf("V[%x] >>= 1; VF = %x", x & 0xF, V[0xF]);
				} break;
				case 7: {
							V[0xF] = V[y >> 4] > V[x & 0xF];
							V[x & 0xF] = V[y >> 4] - V[x & 0xF];
							printf("V[%x] = V[%x] - V[%x] ; VF = %x", x & 0xF, y >> 4, x & 0xF, V[0xF]);
				} break;
				case 0xE: {
							  V[0xF] = (V[x & 0xF] >> 7) & 0x1;
							  V[x & 0xF] <<= 1;
							  printf("V[%x] <<= 1 ; VF = %x", x & 0xF, V[0xF]);
				} break;
				default:
					printf("UNEXPECTED BOTTOM NYBBLE OF Y. EXITING");
					exit(-4);
				}
				pc += 2;	

	} break;

	case 9: {
				if (V[x & 0xF] != V[y >> 4]) {
					pc += 4;
					printf("SKIP NEXT INSTR");
				}
				else {
					pc += 2;
					printf("INCR PC");
				}
	} break;

	case 0xA: {
				  I = ((x & 0xF) << 8) | y;
				  printf("I = 0x%x", I);
				  pc += 2;
	} break;

	case 0xB: {
				  pc = (((x & 0xF) << 8) | y) + V[0];
				  printf("NEW PC: 0x%x", pc);
	} break;

	case 0xC: {
				  V[x & 0xF] = (rand() % CH8_RAND_MAX) & y;
				  printf("V[%x] = 0x%x", x & 0xF, V[x & 0xF]);
				  pc += 2;
	} break;

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
	} break;

	case 0xE: {
				  if (y == 0x9E) {
					  printf("IMPLEMENT ME -- KEYBOARD LOCK");
					  //pc += 2;
				  }
				  else if (y == 0xA1) {
					  pc += 4;
					  printf("IMPLEMENT ME -- KEYBOARD LOCK");
				  } else {
					  printf("UNEXPECTED BOTTOM NYBBLE FOR 0xE, EXITING");
					  exit(-3);
				  }
	} break;

	case 0xF: {
				  switch (y) {
				  case 7: {
							  V[x & 0xF] = delayTimer;
							  printf("V[%x] = 0x%x", x & 0xF, delayTimer);
							  pc += 2;
				  } break;

				  case 0xA: {
								printf("IMPLEMENT ME");
								pc += 2;
								break;
				  }

				  case 0x15: {
								 delayTimer = V[x & 0xF];
								 printf("delayTimer = 0x%x", delayTimer);
								 pc += 2;
				  } break;

				  case 0x18: {
								 soundTimer = V[x & 0xF];
								 printf("soundTimer = 0x%x", soundTimer);
								 pc += 2;
				  } break;

				  case 0x1E: {
								 I += V[x & 0xF];
								 printf("I += V[0x%x]", x & 0xF);
								 pc += 2;
				  } break;

				  case 0x29: {
								 // each number/letter takes up 5 bytes in ram
								 I = V[x & 0xF] * 5;
								 printf("I = BIT DATA FOR V[%x]", x & 0xF);
								 pc += 2;
				  } break;

				  case 0x33: {
								 uint8_t numToStore = V[x & 0xF];
								 // store the hundreds place at i, tens at i + 1, and ones at i + 2
								 ram[I + 2] = numToStore % 10;
								 ram[I + 1] = (numToStore / 10) % 10;
								 ram[I] = (numToStore / 100) % 10;
								 printf("STORING BCD NUM : %d", numToStore);
								 pc += 2;
				  } break;

				  case 0x55: {
								 for (int i = 0; i < (x & 0xF); i++) {
									 ram[I + i] = V[i];
								 }
								 printf("STORE FROM V TO RAM");
								 pc += 2;
					  } break;

				  case 0x65: {
								 for (int i = 0; i <= (x & 0xF); i++) {
									 V[i] = ram[I + i];
								 }
								 printf("LOAD FROM RAM TO V");
								 pc += 2;
				  } break;

				  default:
					  printf("Bad opcode for bottom nybble for case 0xF! Fatal error.");
					  exit(-1);
				  }

	} break;

	default:
		printf("UNRECOGNIZED OPCODE. THIS SHOULD NOT HAPPEN. EXITING...");
		exit(-2);
	}

	if (delayTimer > 0) delayTimer--;

	printf("\n");
}

bool Chip8::readRom() {
	FILE* f;
	f = fopen("Pong.ch8", "rb");
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

// places the bitpatterns for blit-able numbers and text in memory
// since 0x0-0x1FF are unused, they will be placed from 0x0
void Chip8::placeFontsInMemory() {
	// zero
	ram[0] = 0xF0;
	ram[1] = 0x90;
	ram[2] = 0x90;
	ram[3] = 0x90;
	ram[4] = 0xF0;

	// one
	ram[5] = 0x20;
	ram[6] = 0x60;
	ram[7] = 0x20;
	ram[8] = 0x20;
	ram[9] = 0x70;

	// two
	ram[10] = 0xF0;
	ram[11] = 0x10;
	ram[12] = 0xF0;
	ram[13] = 0x80;
	ram[14] = 0xF0;

	// three
	ram[15] = 0xF0;
	ram[16] = 0x10;
	ram[17] = 0xF0;
	ram[18] = 0x10;
	ram[19] = 0xF0;

	// four
	ram[20] = 0x90;
	ram[21] = 0x90;
	ram[22] = 0xF0;
	ram[23] = 0x10;
	ram[24] = 0x10;

	// five
	ram[25] = 0xF0;
	ram[26] = 0x80;
	ram[27] = 0xF0;
	ram[28] = 0x10;
	ram[29] = 0xF0;

	// six
	ram[30] = 0xF0;
	ram[31] = 0x80;
	ram[32] = 0xF0;
	ram[33] = 0x90;
	ram[34] = 0xF0;

	// seven
	ram[35] = 0xF0;
	ram[36] = 0x10;
	ram[37] = 0x20;
	ram[38] = 0x40;
	ram[39] = 0x40;

	// eight
	ram[40] = 0xF0;
	ram[41] = 0x90;
	ram[42] = 0xF0;
	ram[43] = 0x90;
	ram[44] = 0xF0;

	// nine
	ram[45] = 0xF0;
	ram[46] = 0x90;
	ram[47] = 0xF0;
	ram[48] = 0x10;
	ram[49] = 0xF0;

	// A
	ram[50] = 0xF0;
	ram[51] = 0x90;
	ram[52] = 0xF0;
	ram[53] = 0x90;
	ram[54] = 0x90;
	
	// B
	ram[55] = 0xE0;
	ram[56] = 0x90;
	ram[57] = 0xE0;
	ram[58] = 0x90;
	ram[59] = 0xE0;

	// C
	ram[60] = 0xF0;
	ram[61] = 0x80;
	ram[62] = 0x80;
	ram[63] = 0x80;
	ram[64] = 0xF0;

	// D
	ram[65] = 0xE0;
	ram[66] = 0x90;
	ram[67] = 0x90;
	ram[68] = 0x90;
	ram[69] = 0xE0;

	// E
	ram[70] = 0xF0;
	ram[71] = 0x80;
	ram[72] = 0xF0;
	ram[73] = 0x80;
	ram[74] = 0xF0;

	// F
	ram[75] = 0xF0;
	ram[76] = 0x80;
	ram[77] = 0xF0;
	ram[78] = 0x80;
	ram[79] = 0x80;

}

