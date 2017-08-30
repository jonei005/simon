/*
 * Partner 1: Jeremy O'Neill, jonei005@ucr.edu
 * Partner 2: Gabriel Cortez, gcort002@ucr.edu
 * Partner 3: Benjamin Li, bli020@ucr.edu
 * Class: CS/EE 120B Summer 2017
 * Lab Section: 021
 * Assignment: Final Lab (Simon game)
 * I acknowledge all content contained herein, excluding template or example code, is my own original work.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <time.h>
#include "io.c"

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	TCCR1B = 0x0B;		// bit3 = 0: CTC mode (clear timer on compare)
	OCR1A = 125;		// Timer interrupt will be generated when TCNT1==OCR1A
	TIMSK1 = 0x02;		// bit1: OCIE1A -- enables compare match interrupt
	TCNT1=0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;		// 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00;		// bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void set_PWM(double frequency) {
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; } //stops timer/counter
		else { TCCR3B |= 0x03; } // resumes/continues timer/counter
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		// prevents OCR3A from underflowing, using prescaler 64
		// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR3A = 0x0000; }
		// set OCR3A based on desired frequency
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }
		TCNT3 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR0A = 0x00;
	TCCR0B = 0x00;
}

// LCD welcome screen
// Wait for button press to begin
// Random sequence shown on led
// Player enters sequence on button presses
// Report score on LCD screen
// Winning sequence is 9 button presses
// Report win/lose on LCD screen
// Bonus:  Add sounds with the LED/Button press

enum gameStates { GameInit, Welcome, GenerateSequence, GetReady, LightSequence, ShowNext, LightSequenceOff, WaitForPlayer, WaitRelease, CheckInput, Correct, YouWin, YouWinRelease, Wrong, WrongRelease} gameState;
enum signalStates { SignalInit, Off, Signal1, Signal2, Signal3, Signal4, SignalAll, SignalWrong } signalState;

const unsigned TimerPeriod = 100;		// Refresh rate of system is set to 100 ms	

const unsigned char max_length = 6;		// This global stores the game length. Once the player enters this many correct sequences, they win.

unsigned int timer = 0;					// "timer" controls all pauses and delays for user feedback and event sequencing. It ensures that 
										// individual LED blinks are distinguishable from one another.

unsigned int signalTimer = 0;			// "signalTimer" 

unsigned char i = 0;					// i serves as a loop counter for outputting the current number of blinks.

unsigned char j = 0;					// j serves as a loop counter for filling the sequence array.

unsigned char curr_length = 1;			// curr_length represents the player's current difficulty level (i.e. how many blinks they must repeat).

unsigned char correct = 0;				// "correct" is a flag for determining if the player entered a correct or incorrect response sequence.

unsigned char done = 0;					// "done" is a flag that is raised when the player is finished entering their response sequence.

unsigned char signal = 0;				// "signal" is a variable that indicates which LED should be lit.

unsigned char sequence[9] = {0};		// "sequence" stores the current randomly-generated simon sequence.
	
unsigned char userInput = 0;			// "userInput" stores the button that the player last pressed.

unsigned char buttonPressed = 0;		// "buttonPressed" is a flag that remembers the first button that was pressed. Along with userInput, this
										// prevents the player from exploiting the game and pressing all buttons simultaneously to achieve the
										// correct sequence.
	
unsigned char currValue = 0;

void tickGame() {
	userInput = ~PINA;	
	
	// Transitions
	switch(gameState) {
		case GameInit:
			LCD_DisplayString(1, (const unsigned char*)("Welcome to Simon  Press Start"));
			gameState = Welcome;
			break;
			
		case Welcome:
			if (userInput & 0x80) {
				gameState = GenerateSequence;
			}
			break;
			
		case GenerateSequence:
			gameState = GetReady;
		
		case GetReady:
			if (timer >= 2000) {
				gameState = LightSequence;
			}
			break;
		
		case LightSequence:
			if (timer >= 500) {
				gameState = ShowNext;
			}
			break;
			
		case ShowNext:
			gameState = LightSequenceOff;
			break;
			
		case LightSequenceOff:
			if (timer >= 500 && i < curr_length) {
				gameState = LightSequence; 
			}
			else if (timer >= 500 && i >= curr_length) {
				i = 0;
				
				// print game score to lcd
				LCD_DisplayString(1, (const unsigned char*)("Score: "));
				LCD_WriteData(curr_length - 1 + '0');
				
				// print current array to lcd (cheat mode)
				//LCD_ClearScreen();
				//LCD_Cursor(1);
				//for (j = 0; j < curr_length; j++) {
				//	LCD_WriteData(sequence[j] + '0');
				//}
				
				gameState = WaitForPlayer;
			}
			break;
		
		case WaitForPlayer:
			if (buttonPressed == 1 || buttonPressed == 2 || buttonPressed == 3 || buttonPressed == 4) {
				gameState = WaitRelease;
			}
			break;
		
		case WaitRelease:
			if (userInput == 0x00) {
				gameState = CheckInput;
			}
			break;
		
		case CheckInput:
			if (buttonPressed == sequence[i] && i < curr_length - 1) {
				buttonPressed = 0;
				i++;
				
				// print game score to lcd
				LCD_DisplayString(1, (const unsigned char*)("Score: "));
				LCD_WriteData(curr_length - 1 + '0');
				
				// print current array to lcd (cheat mode)
				//LCD_ClearScreen();
				//LCD_Cursor(1);
				//for (j = 0; j < curr_length; j++) {
				//	LCD_WriteData(sequence[j] + '0');
				//}
				
				gameState = WaitForPlayer;
			}
			else if (buttonPressed == sequence[i] && i >= curr_length - 1) {
				buttonPressed = 0;
				i = 0; // FIXME: New add
				gameState = Correct;
			}
			else if (buttonPressed!= sequence[i]) {
				buttonPressed = 0;
				LCD_DisplayString(1, (const unsigned char*)("   Wrong! :(      Press Start"));
				gameState = Wrong;
			}
			break;
		
		case Correct:
			if (curr_length < max_length) {
				curr_length++;
				i = 0;
				gameState = GetReady;
			}
			else {
				LCD_DisplayString(1, (const unsigned char*)("    You win!      Press Start"));
				gameState = YouWin;
			}
			break;
		
		case YouWin:
		
			if ( userInput & 0x80 ) {
				
				gameState = YouWinRelease;				
			}
			break;
		
		case YouWinRelease:
		
			if ( userInput == 0x00 ) {
				LCD_DisplayString(1, (const unsigned char*)("Welcome to Simon  Press Start"));
				gameState = Welcome;
			}
			break;
			
		case Wrong:
		
			if ( userInput & 0x80 ) {
			
				gameState = WrongRelease;
			}
			break;
			
		case WrongRelease:
		
			if ( userInput == 0x00 ) {
				LCD_DisplayString(1, (const unsigned char*)("Welcome to Simon  Press Start"));
				gameState = Welcome;
			}
			break;
			
		default:
			gameState = GameInit;
			break;
	}
	
	// Actions
	switch(gameState) {
		case GameInit:
			i = 0;
			curr_length = 1; // CHANGE TO 1
			correct = 0;
			done = 0;
			signal = 0;
			for (j = 0; j < max_length; j++) {
				sequence[j] = 0;
			}
			break;
		
		case Welcome:
			i = 0;
			curr_length = 1; // CHANGE TO 1
			correct = 0;
			done = 0;
			signal = 0;
			for (j = 0; j < max_length; j++) {
				sequence[j] = 0;
			}
			break;
		
		case GenerateSequence:
			for (j = 0; j < max_length; j++) {
				sequence[j] = (rand() % 4) + 1;
				//sequence[j] = ((j % 4) + 1);
			}
			timer = 0;
			break;
		
		case GetReady:
			timer += TimerPeriod;
			signal = 0; // Print to LCD "Get Ready"
			// RE ADD THIS
			//LCD_DisplayString(1, (const unsigned char*)("Get Ready!"));
			break;
		
		case LightSequence:
			signal = sequence[i];
			timer += TimerPeriod;
			LCD_ClearScreen();
			break;
			
		case ShowNext:
			i++;
			timer = 0;
			signal = 0;
			break;
			
		case LightSequenceOff:
			signal = 0;
			timer += TimerPeriod;
			break;
		
		case WaitForPlayer:			
			signal = 0;
			if (userInput == 0x01) {
				 signal = 1;
				 buttonPressed = 1;
			}
			else if (userInput == 0x02) {
				signal = 2;
				buttonPressed = 2;
			}
			else if (userInput == 0x04) { 
				signal = 3;
				buttonPressed = 3;
			}
			else if (userInput == 0x08) { 
				signal = 4;
				buttonPressed = 4;
			}
			break;
		
		case WaitRelease:
			break;
		
		case CheckInput:
			signal = 0;
			break;
		
		case Correct:
			//signal = 8;	// Light up all
			
			break;
		
		case YouWin:
			signal = 8;
			break;
		
		case YouWinRelease:
			signal = 0;
			break;
		
		case Wrong:
			//signal = 2;	// Light up 2
			signal = 6;
			break;
		
		case WrongRelease:
			signal = 0;
		break;
		
		default:
			gameState = GameInit;
			break;
	
	}
}

void tickSignal() {
	// Transitions
	switch (signalState)
	{
		case SignalInit:
			signalState = Off;
			break;
		
		case Off:
			if ( signal == 1 )
				signalState = Signal1;
			else if ( signal == 2 )
				signalState = Signal2;
			else if ( signal == 3 )
				signalState = Signal3;
			else if ( signal == 4 )
				signalState = Signal4;
			else if ( signal == 8 )
				signalState = SignalAll;
			else if ( signal == 6 )
				signalState = SignalWrong;
			break;
		
		case Signal1:
			if ( signal == 0 )
				signalState = Off;
			else if ( signal == 2 )
				signalState = Signal2;
			else if ( signal == 3 )
				signalState = Signal3;
			else if ( signal == 4 )
				signalState = Signal4;
			break;
		
		case Signal2:
			if ( signal == 0 )
				signalState = Off;
			else if ( signal == 1 )
				signalState = Signal1;
			else if ( signal == 3 )
				signalState = Signal3;
			else if ( signal == 4 )
				signalState = Signal4;
			break;
		
		case Signal3:
			if ( signal == 0 )
				signalState = Off;
			else if ( signal == 1 )
				signalState = Signal1;
			else if ( signal == 2 )
				signalState = Signal2;
			else if ( signal == 4 )
				signalState = Signal4;
			break;
		
		case Signal4:
			if ( signal == 0 )
				signalState = Off;
			else if ( signal == 1 )
				signalState = Signal1;
			else if ( signal == 2 )
				signalState = Signal2;
			else if ( signal == 3 )
				signalState = Signal3;
			break;
			
		case SignalAll:
			if (signal == 0) {
				signalState = Off;
			}
			break;
			
		case SignalWrong:
			if (signal == 0) {
				signalState = Off;
			}
			break;
		
		default:
			signalState = Off;
			break;
	}

	// Actions
	switch (signalState)
	{
		case SignalInit:
			signalTimer = 0;
			PORTB = 0x00;
			set_PWM(0.00);
			break;
		
		case Off:
			signalTimer = 0;
			PORTB = 0x00;
			set_PWM(0.00);
			break;
		
		case Signal1:
			++signalTimer;
			PORTB = 0x01;
			set_PWM(391.995);
			break;
		
		case Signal2:
			++signalTimer;
			PORTB = 0x02;
			set_PWM(261.626);
			break;
		
		case Signal3:
			++signalTimer;
			PORTB = 0x04;
			set_PWM(293.665);
			break;
		
		case Signal4:
			++signalTimer;
			PORTB = 0x08;
			set_PWM(369.994);
			break;
			
		case SignalAll:
			++signalTimer;
			PORTB = 0xFF;
			break;
			
		case SignalWrong:
			++signalTimer;
			PORTB = 0xFF;
			set_PWM(116.54);
			break;
		
		default:
			break;
	}
}


int main(void) {
	
	DDRA = 0x00;	// input
	PORTA = 0xFF;
	
	DDRB = 0xFF;	// output
	PORTB = 0x00;
	
	DDRC = 0xFF;	// output
	PORTC = 0x00;
	
	DDRD = 0xFF;	// output
	PORTD = 0x00;
	
	
	TimerSet(TimerPeriod);
	TimerOn();
	
	LCD_init();
	
	PWM_on();
	
	srand(time(NULL));
	
	gameState = -1;
	signalState = SignalInit;
	
    while (1) {
		tickGame();
		tickSignal();
		
		while(!TimerFlag);
		TimerFlag = 0;
    }
}

