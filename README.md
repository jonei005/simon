SIMON
======

Overview
--------
This is a recreation of the game Simon on a breadboard with an ATmega1284 microcontroller, running C code written in Atmel Studio 7. Simon is a memory game that tests your ability to remember sequences by asking you watch and repeat sequences of increasing length. Our implementation of Simon stops after you correctly input a sequence of length 9.


Setup
-----
- ATmega1284 microcontroller
- LCD display (connected to PORT D6...D7 and PORT C0...C7)
- 4 LEDs (connected to PORT B0...B3)
- 5 buttons
  - 1 start button (connected to PORT A7)
  - 4 game buttons, each corresponding to its adjacent LED (connected to PORT A0...A3)
- Speaker (connected to PORT B6)


How To Play
-----------
After implementing the hardware and programming the microcontroller, you may use these instructions to play Simon:
```
1. Turn on the power source of the breadboard.
2. When the LCD screen displays "Welcome to Simon", press the Start Button to start the game.
3. The LCD screen will display your current score, the length of the last sequence you inputted correctly.
4. After a short delay, a randomly chosen LED will blink. You must press the button that adjacent to that LED. 
5. If you pressed the correct button, Simon will add another light to the sequence.
   - The sequence builds from the previous sequence. You must input the new sequence in order.
6. This process repeats until you input a sequence of length 9, when the game tells you that you have won. 
7. If your input is incorrect at any time during the game, the LCD will tell you that you have lost.
8. After a win or loss, you may press Start to go back to the Welcome Screen and play again. 
```

Demo video: https://youtu.be/zdUH_CReJn4

Relevant Files
--------------
- Main C code: `simon/simon/main.c`
- Hex file to program ATmega1284 directly: `simon/simon/debug/simon.hex`
- 'Includes' folder, containing relevant functions for LCD controls: `simon/simon/includes`
- Atmel Studio 7 Project File: `simon/simon.atsln`

Authors
-------
Jeremy O'Neill

Gabriel Cortez

Benjamin Li
