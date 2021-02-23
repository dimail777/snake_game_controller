/*
 * SnakeGameIR.c
 * Author : Dmitriy.Shisterov
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
#include <inttypes.h>

#define BYTE_MASK 0b00000111    // 8 - 1

// IRR DEFENITIONS START

#define IRR_PIN 2
#define IRR_PIN_PORT PIND
#define IRR_UP_COMMAND 0b01000110
#define IRR_DOWN_COMMAND 0b00010101
#define IRR_LEFT_COMMAND 0b01000100
#define IRR_RIGHT_COMMAND 0b01000011

// IRR DEFENITIONS END
// GAME DEFENITIONS START

#define MIN_SNAKE_SIZE 3
#define MAX_SNAKE_SIZE 32

#define MATRIX_ROWS PORTB
#define MATRIX_COLUMNS PORTD
#define MATRIX_COLUMN_2 PORTC
#define C_2 2
#define PING PORTC

#define UP_DIRECTION 0
#define DOWN_DIRECTION 1
#define RIGHT_DIRECTION 2
#define LEFT_DIRECTION 3

// GAME DEFENITIONS END
// IRR FIELDS START

static uint8_t impulse = 0;
static uint8_t up_count = 0;
static uint8_t down_count = 0;

static uint8_t starting = 0;
static uint8_t started = 0;
static uint8_t done = 0;

static int bit_position = 0;
static int var_position = 0;

static uint8_t irr_address_0 = 0;
static uint8_t irr_address_1 = 0;
static uint8_t irr_command_0 = 0;
static uint8_t irr_command_1 = 0;

// IRR FIELDS END
// GAME FIELDS START

const int ACTIVE_ROW [8] = { 0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b10000000 };

int matrix [8];
int* snake; 
int size, direction;
int foodX, foodY;

// GAME FIELDS END
// IRR INTERRUPTS & METHODS START

ISR (TIMER0_OVF_vect) {
	if (impulse) {
		up_count += 1;
	} else {
		down_count += 1;
	}
	if (up_count < 16 && down_count > 8) {
		TCCR0 &= ~1; // IR timer down
		TCNT0 = 0;
		starting = 0;
		started = 0;
		up_count = 0;
		down_count = 0;
	}
}

ISR (INT0_vect) {
	if (IRR_PIN_PORT & (1 << IRR_PIN)) {
		impulse = 0;
	} else {
		TCNT0 = 0;
		impulse = 1;
		if (starting == 0) {
			TCCR0 |= 1; // IR timer up
			up_count = 0;
			down_count = 0;
			starting = 1;
			return;
		}
		
		if (up_count > 16 && down_count > 8) {
			started = 1;
			up_count = 0;
			down_count = 0;
		} else if (started && up_count > 0 && up_count < 4) {
			if (down_count > 3 && down_count < 8) {
				if (irRegistries(1)) {
					TCCR0 &= ~1; // IR timer down
					TCNT0 = 0;
					started = 0;
					starting = 0;
					done = 1;
				}
			} else if (down_count > 0 && down_count < 4) {
				if (irRegistries(0)) {
					TCCR0 &= ~1; // IR timer down
					TCNT0 = 0;
					started = 0;
					starting = 0;
					done = 1;
				}
			}
		}
		up_count = 0;
		down_count = 0;
	}
}

int irRegistries(int signal) {
	switch(var_position) {
		case 0:
		if (signal) {
			irr_address_0 |= (1 << bit_position);
		}
		bit_position = (bit_position + 1) & BYTE_MASK;
		if (bit_position == 0) {
			var_position = 1;
		}
		break;
		
		case 1:
		if (signal == 0) {
			irr_address_1 |= (1 << bit_position);
		}
		bit_position = (bit_position + 1) & BYTE_MASK;
		if (bit_position == 0) {
			var_position = 2;
		}
		break;
		
		case 2:
		if (signal) {
			irr_command_0 |= (1 << bit_position);
		}
		bit_position = (bit_position + 1) & BYTE_MASK;
		if (bit_position == 0) {
			var_position = 3;
		}
		break;
		
		case 3:
		if (signal == 0) {
			irr_command_1 |= (1 << bit_position);
		}
		bit_position = (bit_position + 1) & BYTE_MASK;
		if (bit_position == 0) {
			var_position = 0;
			return 1;
		}
		break;
	}
	return 0;
}

uint8_t getNextCommand(void) {
	uint8_t command;
	if (done == 0) {
		command = 0;
		return command;
	}
	done = 0;
	command = irr_command_0;
	if (irr_address_0 != irr_address_1 || irr_command_0 != irr_command_1 || irr_command_0 == 0) {
		command = 0;
	}
	irr_address_0 = 0;
	irr_command_0 = 0;
	irr_address_1 = 0;
	irr_command_1 = 0;
	
	return command;
}

// IRR INTERRUPTS & METHODS END

// GAME INTERRUPTS START

ISR(TIMER1_OVF_vect) {
	TCCR1B = 0b00000000; // game timer down
	
	int headX = 0;
    int headY = 0;
	switch (direction) {
		case UP_DIRECTION:
		headX = *(snake) + 1;
		headY = *(snake + 1);
		break;
		case DOWN_DIRECTION:
		headX = *(snake) - 1;
		headY = *(snake + 1);
		break;
		case RIGHT_DIRECTION:
		headX = *(snake);
		headY = *(snake + 1) + 1;
		break;
		case LEFT_DIRECTION:
		headX = *(snake);
		headY = *(snake + 1) - 1;
		break;
	}
	headX &= BYTE_MASK;
	headY &= BYTE_MASK;

	int eaten = foodX == headX && foodY == headY;
	if (eaten) {
		initFood();
		size = size + 1;
		snake = realloc(snake, 2 * size * sizeof(int));
		signalExt(0);
	} else {
		int tailX = *(snake + 2 * (size - 1));
		int tailY = *(snake + 2 * (size - 1) + 1);
		matrix[tailX] |= (1 << tailY); // tail has been shifted
	}
	
	int i;
	int crashed = 0;
	for (i = size - 1; i > 0; i = i - 1) {
		
		*(snake + 2 * i) = *(snake + 2 * (i - 1));
		*(snake + 2 * i + 1) = *(snake + 2 * (i - 1) + 1);
		
		if (*(snake + 2 * i) == headX && *(snake + 2 * i + 1) == headY) {
			crashed = 1;
		}
		
	}
	*(snake) = headX;
	*(snake + 1) = headY;
	
	if (crashed || size == MAX_SNAKE_SIZE) {
		signalExt(1);
		clearGame();		
	} else {
		matrix[headX] &= ~(1 << headY); // head has been shifted
	}
	
	TCCR1B = 0b00000010; //  game timer up
	TCNT1 = 0;			 // overflow - 524.288 ms
}

void signalExt(int longTime) {
	PING |= 3;
	if (longTime) {
		_delay_ms(200);
	} else {
		_delay_ms(20);
	}
	PING &= ~3;
}

// GAME INTERRUPTS END

int main(void) {
	initGame();
	
	DDRB = 0b11111111;
	DDRD = 0b11111011;
	DDRC = 0b00000111;

	GICR |= (1 << 6);            // external interrupt enable
	MCUCR |= 1;					 // interrupt on PD2 (INT0) port
	
	TIMSK |= ((1 << 2) | 1);     // timer counter0 counter1, overflow interrupt enable
	TCCR1B = 0b00000010;		 // counter1 timer up
	TCNT1 = 0;					 // overflow - 524.288 ms
	SREG |= (1<<7);				 // enable global interrupts

    while (1) {
		matrixUpdate();
		updateSnakeDirection();
    }
}

void matrixUpdate(void) {
	int i;
	for (i = 0; i < 8; i = i + 1) {
		if (matrix[i] == 0b11111111) {
			continue;
		}
		
		MATRIX_COLUMNS = matrix[i] & 0b11111011;
		if (matrix[i] & (1 << C_2)) {
			MATRIX_COLUMN_2 |= (1 << C_2);
		} else {
			MATRIX_COLUMN_2 &= ~(1 << C_2);
		}
		MATRIX_ROWS = ACTIVE_ROW[i];
		
		MATRIX_COLUMNS = 0b11111011;
		MATRIX_COLUMN_2 |= (1 << C_2);
		MATRIX_ROWS = 0b00000000;
	}
}

void updateSnakeDirection(void) {
	uint8_t command = getNextCommand();
	if (command == 0) {
		return;
	}
	if (command == IRR_UP_COMMAND && direction != DOWN_DIRECTION) {
		direction = UP_DIRECTION;
	} else if (command == IRR_DOWN_COMMAND && direction != UP_DIRECTION) {
		direction = DOWN_DIRECTION;
	} else if (command == IRR_RIGHT_COMMAND && direction != LEFT_DIRECTION) {
		direction = RIGHT_DIRECTION;
	} else if (command == IRR_LEFT_COMMAND && direction != RIGHT_DIRECTION) {
		direction = LEFT_DIRECTION;
	}
}

void initGame(void) {
	direction = UP_DIRECTION;
	size = MIN_SNAKE_SIZE;
	snake = (int*)malloc(2 * size * sizeof(int));
	
	int i;
	for (i = 0; i < size; i = i + 1) {
		*(snake + 2 * i) = size - 1 - i; // snake[i][0]
		*(snake + 2 * i + 1) = 0;		 // snake[i][1]
	}
	matrix [0] = 0b11111110;
	matrix [1] = 0b11111110;
	matrix [2] = 0b11111110;
	matrix [3] = 0b11111111;
	matrix [4] = 0b11111111;
	matrix [5] = 0b11111111;
	matrix [6] = 0b11111111;
	matrix [7] = 0b11111111;
	
	initFood();
}

void initFood(void) {
	int intersection;
	do {
		foodX = rand() % 8;
		foodY = rand() % 8;
		intersection = ~matrix[foodX] & (1 << foodY);
	} while (intersection != 0);
	matrix[foodX] &= ~(1 << foodY);
}

void clearGame(void) {
	free(snake);
	initGame();
}