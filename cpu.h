#ifndef CPU_H_
#define CPU_H_

/* GLOBALS */
unsigned short opcode; // opcode
unsigned char memory[4096]; // 4kb
unsigned char V[16]; // registers
unsigned short I; // index register
unsigned short pc; // program counter
unsigned char gfx[64][32]; // 2048 pixels
unsigned char drawFlag; // draw flag
unsigned char delay_timer; // timer
unsigned char sound_timer; // timer
unsigned short stack[16]; // stack
unsigned short sp; // stack pointer
unsigned char key[16]; // keypad
unsigned char chip8_fontset[80]; //fontset

/* FUNCTIONS */
void initGraphics();
void initInput();
void initSystem();
void loadGame(char *game);
void emulateCycle();
void drawGraphics();
void setKeys();
void keyDown(unsigned char k);
void keyUp(unsigned char k);
#endif
