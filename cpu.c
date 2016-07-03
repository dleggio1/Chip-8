#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Sys/time.h>
#include "cpu.h"


void initSystem(){
    /* Initialize registers and memory */
    int i,j;

    pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;

    //Clear display
    for(i = 0; i < 32; i++){
        for(j = 0; j < 64; j++){
            gfx[i][j] = 0;
        }
    }

	drawFlag = 0;

    //Clear stack
    for(i = 0; i < 16; i++) stack[i] = 0;

    //Clear registers
    for(i = 0; i < 16; i++) V[i] = 0;

    //Clear memory
    for(i = 0; i < 4096; i++) memory[i] = 0;

    //Load fontset
	unsigned char chip8_fontset[80] =
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
    for(i = 0; i < 80; i++) memory[i] = chip8_fontset[i];

    //Reset timers
    delay_timer = 0;
    sound_timer = 0;

	//Seed rand()
	srand(time(NULL));

}

void loadGame(char *game){
    /* Load game into memory */
    FILE *in = fopen(game,"rb");
    int i = 512; // start point in memory
    while(!feof(in)){
		fread(&memory[i++],1,1,in);
	}
    fclose(in);

}

void emulateCycle(){
    /*Fetch, Decode, Execute opcode and update timer */
    int i,j;

    //Fetch
    opcode = (memory[pc] << 8) | memory[pc+1]; // merge upper & lower opcodes

    //Decode & Execute
	switch(opcode & 0xF000){

		case 0x0000: // two opcodes start with 0
			switch(opcode & 0x000F){

				case 0x0000: // 0x00E0 - clear screen
                    for(i = 0; i < 32; i++){
                        for(j = 0; j < 64; j++){
                            gfx[i][j] = 0;
                        }
                    }
                    pc += 2;
					//drawFlag = 1;
					break;

				case 0x000E: // 0x00EE - return from subroutine
					pc = stack[--sp];
					break;

				default:
					fprintf(stderr,"ERROR: Unknown opcode [0x0000]: 0x%X\n",opcode);
			}
			break;

		case 0x1000: // 0x1NNN - jumps to address NNN
			pc = opcode & 0x0FFF;
			break;

		case 0x2000: // 0x2NNN - jumps to subroutine at NNN
			stack[sp++] = pc;
			pc = opcode & 0x0FFF;
			break;

		case 0x3000: // 0x3XNN - skips next instruction if VX == NN
			if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) pc += 4;
			else pc += 2;
			break;

		case 0x4000: // 0x4XNN - skips next instruction if VX != NN
			if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) pc += 4;
			else pc += 2;
			break;

		case 0x5000: // 0x5XY0 - skips next instruction if VX == VY
			if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) pc += 4;
			else pc += 2;
			break;

		case 0x6000: // 0x6XNN - sets VX to NN
			V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
			pc += 2;
			break;

		case 0x7000: // 0x7XNN - adds NN to VX
			V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
			pc += 2;
			break;

		case 0x8000: // nine opcodes start with 8
			switch(opcode & 0x000F){
				case 0x0000: // 0x8XY0 - sets VX to the value of VY
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0001: // 0x8XY1 - sets VX to VX | VY
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] | V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0002: // 0x8XY2 - sets VX to VX & VY
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] & V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0003: // 0x8XY3 - sets VX to VX ^ VY
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] ^ V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0004: // 0x8XY4 - adds VY to VX. VF is set to 1 if carry, 0 if no carry
                    if(V[(opcode & 0x00F0) >> 4] > (255 - V[(opcode & 0x0F00) >> 8])) V[0xF] = 1;
					else V[0xF] = 0; // no carry
					V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;

				case 0x0005: // 0x8XY5 - VY is subtracted from VX. VF is set to 0 if borrow, 1 if no borrow
                    if(V[(opcode & 0X00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) V[0xF] = 0;
					else V[0xF] = 1;
					V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
					pc += 2;

				case 0x0006: // 0x8XY6 - shifts VX right by one. VF is set to lsb of VX before shift
					V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
					V[(opcode & 0x0F00) >> 8] >>= 1;
					pc += 2;

				case 0x0007: // 0x8XY7 - sets VX to VY - VX. VF is set to 0 if borrow, 1 if no borrow
                    if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) V[0xF] = 0;
					else V[0xF] = 1;
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
					pc += 2;

				case 0x000E: // 0x8XYE - shift VX left by one. VF is set to msb of VX before shift
					V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
					V[(opcode & 0x0F00) >> 8] <<= 1;
					pc += 2;
					break;

				default:
					 fprintf(stderr,"ERROR: Unknown opcode [0x8000]: 0x%X\n",opcode);

			}
			break;

		case 0x9000: // 0x9XY0 - skips the next instruction if VX != VY
			if(V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) pc += 4;
			else pc += 2;
			break;

		case 0xA000: // 0xANNN - sets I to the address of N
			I = opcode & 0x0FFF;
			pc += 2;
			break;

		case 0xB000: // 0xBNNN - jumps to the address NNN plus V0
			pc = V[0x0] + (opcode & 0x0FFF);
			break;

		case 0xC000: // 0xCXNN - sets VX = NN & rand_number
			V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF) & (rand() % 256);
			pc += 2;
			break;

		case 0xD000: // 0xDXYN - draw sprite
		{
			unsigned short x_loc = V[(opcode & 0x0F00) >> 8];
			unsigned short y_loc = V[(opcode & 0x00F0) >> 4];
			unsigned short height = opcode & 0x000F;
			unsigned short pixel;
            unsigned short row;
            unsigned short col;
			int yline;
			int xline;

			V[0xF] = 0;

            for (yline = 0; yline < height; yline++) {
                row = yline + V[y_loc];

                for (xline = 0; xline < 8; xline++) {
                    // each 'pixel' is one bit. each sprite is 8 pixels wide (1 byte). So we step through each bit checking if it is set.
                    pixel = memory[I + yline] & (0x80 >> xline);

                    if (pixel != 0) {
                        col = xline + V[x_loc];
                        if (gfx[col][row] == 1) {
                            V[0xF] = 1;
                        }
                        gfx[col][row] = gfx[col][row] ^ 1;
                    }
                }
            }
			drawFlag = 1;
			pc += 2;
		}
			break;

		case 0xE000:
			switch(opcode & 0x000F){

				case 0x000E: // 0xEX9E - skips next instruction if key stored in VX is pressed
					if(key[V[(opcode & 0x0F00) >> 8]] != 0) pc += 4;
					else pc += 2;
					break;

				case 0x0001: // 0xEXA1 - skips next instruction if key stored in VX isnt pressed
					if(key[V[(opcode & 0x0F00) >> 8]] == 0) pc += 4;
					else pc += 2;
					break;

				default:
					fprintf(stderr,"ERROR: Unknown opcode [0xE000]: 0x%X\n",opcode);

			}
			break;

		case 0xF000:
			switch(opcode & 0x000F){
				case 0x0007: // 0xFX07 - sets VX to value of delay timer
					V[(opcode & 0x0F00) >> 8] = delay_timer;
					pc += 2;
					break;

				case 0x000A: // 0xFX0A - key press is awaited, stored in VX
                {
                    int keyPress = 0;
                    for(i = 0; i < 16; i++){
                        if(key[i] != 0){
                            V[(opcode & 0x0F00) >> 8] = i;
                            keyPress = 1;
                           //break;
                        }
                    }
                    if(!keyPress) return;
                    pc += 2;
					break;
                }
				case 0x0005:
					switch(opcode & 0x00F0){
						case 0x0010: // 0xFX15 - sets delay timer to VX
							delay_timer = V[(opcode & 0x0F00) >> 8];
							pc += 2;
							break;

						case 0x0050: // 0xFX55 - Stores V0 to VX (including VX) in mem starting at address I
						{
							int i;
							for(i = 0; i <= ((opcode & 0x0F00) >> 8); i++) memory[I+i] = V[i];
							I += ((opcode & 0x0F00) >> 8) + 1;
							pc += 2;
							break;
						}
						case 0x0060: // 0xFX65 - Fills V0 to VX (including VX) with values starting at address I
						{
							int i;
							for(i = 0; i <= ((opcode & 0x0F00) >> 8); i++) V[i] = memory[I+i];
							I += ((opcode & 0x0F00) >> 8) + 1;
							pc += 2;
							break;
						}
						default:
							fprintf(stderr,"ERROR: Unknown opcode [0xF005]: 0x%X\n",opcode);
					}

				case 0x0008: // 0xFX18 - sets sound timer to VX
					sound_timer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;

				case 0x000E: // 0xFX1E - adds VX to I
					I += V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;

				case 0x0009: // 0xFX29 - sets I to the location of the sprite for the character in VX
					I = V[(opcode & 0x0F00) >> 8] * 0x5;
					pc += 2;
					break;

				case 0x0003: // Stores binary-coded decimal representation of VX in I, I+1, I+2
					memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
					memory[I+1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
					memory[I+2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
					pc += 2;
					break;

				default:
					fprintf(stderr,"ERROR: Unknown opcode [0xF000]: 0x%X\n",opcode);

			}
			break;

		default:
				fprintf(stderr,"ERROR: Unknown opcode : 0x%X\n",opcode);


	}

	//Update timers
    static struct timeval lastFireTime = {.tv_sec = 0, .tv_usec = 0};
    struct timeval currentTime;
    struct timeval timeDiff;
    
    gettimeofday(&currentTime, NULL);
    timersub(&currentTime, &lastFireTime, &timeDiff);
    double totalTime = (timeDiff.tv_sec * 1000000.0 + timeDiff.tv_usec) / 1000000.0;
    
    if (totalTime >= 1.0/60.0f) {
        
        lastFireTime = currentTime;
        
        if (delay_timer > 0) {
            delay_timer--;
        }
        
        if (sound_timer > 0) {
            printf("BEEP!\n");
            sound_timer--;
        }
    }
}

void keyDown(unsigned char k){

	switch(k){

		case '1':
			key[0x1] = 1;
			break;
		case '2':
			key[0x2] = 1;
			break;
		case '3':
			key[0x3] = 1;
			break;
		case '4':
			key[0x4] = 1;
			break;
		case '5':
			key[0x5] = 1;
			break;
		case '6':
			key[0x6] = 1;
			break;
		case '7':
			key[0x7] = 1;
			break;
		case '8':
			key[0x8] = 1;
			break;
		case '9':
			key[0x9] = 1;
			break;
		case '0':
			key[0x0] = 1;
			break;
		case 'A':
			key[0xA] = 1;
			break;
		case 'B':
			key[0xB] = 1;
			break;
		case 'C':
			key[0xC] = 1;
			break;
		case 'D':
			key[0xD] = 1;
			break;
		case 'E':
			key[0xE] = 1;
			break;
		case 'F':
			key[0xF] = 1;
			break;
		default:
			fprintf(stderr,"ERROR: Unrecognized key\n");

	}


}

void keyUp(unsigned char k){

	switch(k){

		case '1':
			key[0x1] = 0;
			break;
		case '2':
			key[0x2] = 0;
			break;
		case '3':
			key[0x3] = 0;
			break;
		case '4':
			key[0x4] = 0;
			break;
		case '5':
			key[0x5] = 0;
			break;
		case '6':
			key[0x6] = 0;
			break;
		case '7':
			key[0x7] = 0;
			break;
		case '8':
			key[0x8] = 0;
			break;
		case '9':
			key[0x9] = 0;
			break;
		case '0':
			key[0x0] = 0;
			break;
		case 'A':
			key[0xA] = 0;
			break;
		case 'B':
			key[0xB] = 0;
			break;
		case 'C':
			key[0xC] = 0;
			break;
		case 'D':
			key[0xD] = 0;
			break;
		case 'E':
			key[0xE] = 0;
			break;
		case 'F':
			key[0xF] = 0;
			break;
		default:
			fprintf(stderr,"ERROR: Unrecognized key\n");

	}


}



void initGraphics(){

}

void initInput(){

}

void drawGraphics(){

}

void setKeys(){

}
