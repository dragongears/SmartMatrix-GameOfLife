/*
 * SmartMatrix Game of Life - Conway's Game of Life for the Teensy 3.1 and SmartMatrix Shield.
 * Version 1.1.2
 * Copyright (c) 2014 Art Dahm (art@dahm.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// The array containing the cells expands beyond the display by one cell in each direction.
// The cells on the edge of the display are copied to the opposite side before the next
// generation is calculated to allow the field to wrap (Instead of doing a test for each
// cell's calculation).
//
// 9|789|7
// -+---+-
// 3|123|1
// 6|456|4
// 9|789|7
// -+---+-
// 3|123|1

#include <Time.h>
#include <SmartMatrix_32x32.h>
#include <IRremote.h>

int RECV_PIN = 18;
IRrecv irrecv(RECV_PIN);
decode_results results;

// IR Raw Key Codes for NEC remote
#define IRCODE_NEC_HELD     0xFFFFFFFF
#define IRCODE_NEC_ANSWER   0x00FFA25D
#define IRCODE_NEC_PHONE    0x00FF629D
#define IRCODE_NEC_HANGUP   0x00FFE21D
#define IRCODE_NEC_CH_DOWN  0x00FF22DD
#define IRCODE_NEC_CH_UP    0x00FF02FD
#define IRCODE_NEC_EQ       0x00FFC23D
#define IRCODE_NEC_REW      0x00FFE01F
#define IRCODE_NEC_FF       0x00FFA857
#define IRCODE_NEC_PLAY     0x00FF906F
#define IRCODE_NEC_MINUS    0x00FF9867
#define IRCODE_NEC_PLUS     0x00FFB04F
#define IRCODE_NEC_0    0x00FF6897
#define IRCODE_NEC_1    0x00FF30CF
#define IRCODE_NEC_2    0x00FF18E7
#define IRCODE_NEC_3    0x00FF7A85
#define IRCODE_NEC_4    0x00FF10EF
#define IRCODE_NEC_5    0x00FF38C7
#define IRCODE_NEC_6    0x00FF5AA5
#define IRCODE_NEC_7    0x00FF42BD
#define IRCODE_NEC_8    0x00FF4AB5
#define IRCODE_NEC_9    0x00FF52AD

SmartMatrix matrix;

int brightness = 60;
const rgb24 black = {0, 0, 0};
const rgb24 white = {0xff, 0xff, 0xff};
const rgb24 textColor = {0xff, 0, 0};
const rgb24 valueColor = {0xff, 0xff, 0};
const rgb24 cursorColor = {0xff, 0x00, 0x00};
const rgb24 cursorOverCellColor = {0x00, 0xff, 0x00};

#define HISTORY_GENERATIONS 10

uint16_t history[HISTORY_GENERATIONS];
uint16_t generations=0;

uint8_t *currentGenerationPtr;
uint8_t *previousGenerationPtr;
uint8_t generationBuffer[2][34][34];
uint8_t generationToggle = 0;

bool    editMode = false;
uint8_t editX = 16;
uint8_t editY = 16;
uint8_t editColor;

unsigned long speed = 100;
bool singleStep = false;
bool wrap = true;
long messageMillis = 0;
uint8_t color = 1;
rgb24 colors[] = {{0x99, 0x99, 0x99}, {0xff, 0xff, 0xff}, {0x00, 0x00, 0xff}, {0x00, 0xff, 0xff}, {0x00, 0xff, 0x00}, {0xff, 0xff, 0x00}, {0xff, 0x00, 0x00}, {0xff, 0x00, 0xff}};

// the setup() method runs once, when the sketch starts
void setup() {
    irrecv.enableIRIn(); // Start the receiver

    matrix.begin();
    matrix.setBrightness(brightness*(255/100));

    // Initialize generation buffer
    swapGenerationBuffer();

    // Use teensy RTC for time
    setSyncProvider(getTeensy3Time);

    // Set the random seed to the current time
    srand(now());

    randomizeField();
}

// the loop() method runs over and over again, as long as the board has power
void loop() {
    if (editMode) {
        editRemoteFunctions();
    } else {
        if (!singleStep && messageMillis == 0) {
            // Next generation
            advanceGeneration();

            // Delay
            delay(speed);
        }

        // Do remote control functions
        remoteFunctions();
    }

    messageTest();
}

void randomizeField() {
    for (int x = 0; x < 34*34; x++) {
        currentGenerationPtr[x] = rand()%2;
    }
}

void remoteFunctions() {
    // Check for code from remote control
    if (irrecv.decode(&results)) {
        switch(results.value){
            case IRCODE_NEC_FF:
                if (singleStep) {
                    singleStep = false;
                } else {
                    if (speed > 10) {
                        speed -= 10;
                    }
                    showSpeed();
                }
            break;

            case IRCODE_NEC_REW:
                if (singleStep) {
                    singleStep = false;
                } else {
                    if (speed < 140) {
                        speed += 10;
                    }
                    showSpeed();
                }
            break;

            case IRCODE_NEC_PLAY:
                if (singleStep) {
                    advanceGeneration();
                } else {
                    singleStep = true;
                }
            break;

            case IRCODE_NEC_PHONE:
                color++;
                if (color > 7) {
                    color = 1;
                }
                displayCurrentGeneration();
                delay(100);
            break;

            case IRCODE_NEC_EQ:
                wrap = !wrap;
                showWrap();
            break;

            case IRCODE_NEC_CH_DOWN:
                if (brightness > 10) {
                    brightness -= 10;
                    matrix.setBrightness(brightness*(255/100));
                }
                showBrightness();
            break;

            case IRCODE_NEC_CH_UP:
                if (brightness < 100) {
                    brightness += 10;
                    matrix.setBrightness(brightness*(255/100));
                }
                showBrightness();
            break;

            case IRCODE_NEC_5:
                editStart();
            break;

            case IRCODE_NEC_0:
                randomizeField();
                displayCurrentGeneration();
            break;
        }
        irrecv.resume(); // Receive the next value
    }
}

void editStart() {
    editMode = true;
    editColor = color;
    color = 0;
    displayCurrentGeneration();
    drawEditCursor();
}

void editEnd() {
    editMode = false;
    color = editColor;
    displayCurrentGeneration();
}

void moveEditCursor(int8_t x, int8_t y) {
    displayCurrentGeneration();
    editX += x;
    editY += y;
    editX %= 32;
    editY %= 32;
    drawEditCursor();
}

void drawEditCursor() {
    uint8_t cell = generationBuffer[generationToggle][editX+1][editY+1];
    cell ? matrix.drawPixel(editX, editY, cursorOverCellColor) : matrix.drawPixel(editX, editY, cursorColor);
}

void editRemoteFunctions() {
    // Check for code from remote control
    if (irrecv.decode(&results)) {
        switch(results.value){

            case IRCODE_NEC_PLAY:
                editEnd();
            break;

            case IRCODE_NEC_0:
                for (int x = 0; x < 34*34; x++) {
                    currentGenerationPtr[x] = 0;
                }
                moveEditCursor(0, 0);
            break;

            case IRCODE_NEC_PLUS:
                generationBuffer[generationToggle][editX+1][editY+1] = 1;
                moveEditCursor(0, 0);
            break;

            case IRCODE_NEC_MINUS:
                generationBuffer[generationToggle][editX+1][editY+1] = 0;
                moveEditCursor(0, 0);
            break;

            case IRCODE_NEC_4:
                moveEditCursor(-1, 0);
            break;

            case IRCODE_NEC_6:
                moveEditCursor(1, 0);
            break;

            case IRCODE_NEC_2:
                moveEditCursor(0, -1);
            break;

            case IRCODE_NEC_8:
                moveEditCursor(0, 1);
            break;

            case IRCODE_NEC_5:
                editX = 16;
                editY = 16;
                moveEditCursor(0, 0);
            break;

        }
        irrecv.resume(); // Receive the next value
    }
}

void showWrap() {
    matrix.fillScreen(black);

    matrix.setFont(font6x10);

    if (!wrap) {
        matrix.drawString(11, 3, textColor, "NO");

        for (int k = 1; k < 33; k++) {
            generationBuffer[generationToggle][k][0] = 0;
            generationBuffer[generationToggle][k][33] = 0;
            generationBuffer[generationToggle][0][k] = 0;
            generationBuffer[generationToggle][33][k] = 0;
            generationBuffer[!generationToggle][k][0] = 0;
            generationBuffer[!generationToggle][k][33] = 0;
            generationBuffer[!generationToggle][0][k] = 0;
            generationBuffer[!generationToggle][33][k] = 0;
        }

        generationBuffer[generationToggle][0][0] = 0;
        generationBuffer[generationToggle][33][33] = 0;
        generationBuffer[generationToggle][0][33] = 0;
        generationBuffer[generationToggle][33][0] = 0;
        generationBuffer[!generationToggle][0][0] = 0;
        generationBuffer[!generationToggle][33][33] = 0;
        generationBuffer[!generationToggle][0][33] = 0;
        generationBuffer[!generationToggle][33][0] = 0;
    }
    matrix.drawString(5, 14, textColor, "WRAP");

    matrix.swapBuffers(false);

    messageInit();
}

void showSpeed() {
    matrix.fillScreen(black);

    matrix.setFont(font6x10);
    matrix.drawString(2, 3, textColor, "SPEED");

    matrix.setFont(font8x13);

    char value[] = "00";
    value[0] = '0' + (150-speed) / 100;
    value[1] = '0' + ((150-speed) % 100) / 10;

    matrix.drawString(8, 16, valueColor, value);

    matrix.swapBuffers(false);

    messageInit();
}

void showBrightness() {
    matrix.fillScreen(black);
    matrix.setFont(font5x7);
    matrix.drawString(2, 3, textColor, "BRIGHT");

    matrix.setFont(font8x13);

    char value[] = "00";
    value[0] = '0' + (brightness/10) / 10;
    value[1] = '0' + (brightness/10) % 10;

    matrix.drawString(8, 16, valueColor, value);

    matrix.swapBuffers(false);

    messageInit();
}

// Initialize the timer for a message
void messageInit() {
    messageMillis = millis();
}

// Hide message if enough time has passed
void messageTest() {
    if (messageMillis != 0) {
        if (millis() > messageMillis + 2000) {
            messageMillis = 0;
            displayCurrentGeneration();
        }
        delay (100);
    }
}

// Do everything needed to advance to the next generation
void advanceGeneration() {
    // Swap generation buffers (current generation becomes previous generation)
    swapGenerationBuffer();

    if (wrap) {
        // Wrap outside borders of previous generation
        for (int k = 1; k < 33; k++) {
            generationBuffer[!generationToggle][k][0] = generationBuffer[!generationToggle][k][32];
            generationBuffer[!generationToggle][k][33] = generationBuffer[!generationToggle][k][1];
            generationBuffer[!generationToggle][0][k] = generationBuffer[!generationToggle][32][k];
            generationBuffer[!generationToggle][33][k] = generationBuffer[!generationToggle][1][k];
        }

        generationBuffer[!generationToggle][0][0] = generationBuffer[!generationToggle][32][32];
        generationBuffer[!generationToggle][33][33] = generationBuffer[!generationToggle][1][1];
        generationBuffer[!generationToggle][0][33] = generationBuffer[!generationToggle][32][1];
        generationBuffer[!generationToggle][33][0] = generationBuffer[!generationToggle][1][32];
    }

    // Do next generation calculation from previous generation into current generation
    for (int x = 1; x < 33; x++) {
        for (int y = 1; y < 33; y++) {
            generationBuffer[generationToggle][x][y] = getCellStatus(x, y);
        }
    }

    boringnessDetection();

    displayCurrentGeneration();
}

void displayCurrentGeneration() {
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            uint8_t cell = generationBuffer[generationToggle][x+1][y+1];
            cell ? matrix.drawPixel(x, y, colors[color]) : matrix.drawPixel(x, y, black);
        }
    }

    matrix.swapBuffers(false);
}

// Count neighbors and determine if cell is alive or dead
uint8_t getCellStatus(uint8_t x, uint8_t y) {

    uint16_t count = 0;

    count += generationBuffer[!generationToggle][x-1][y-1];
    count += generationBuffer[!generationToggle][x][y-1];
    count += generationBuffer[!generationToggle][x+1][y-1];
    count += generationBuffer[!generationToggle][x-1][y];
    count += generationBuffer[!generationToggle][x+1][y];
    count += generationBuffer[!generationToggle][x-1][y+1];
    count += generationBuffer[!generationToggle][x][y+1];
    count += generationBuffer[!generationToggle][x+1][y+1];

    return (count == 3 || (count == 2 && generationBuffer[!generationToggle][x][y]));
}

// Swap current and previous generation buffers
void swapGenerationBuffer() {
    generationToggle = !generationToggle;
    currentGenerationPtr = &generationBuffer[generationToggle][0][0];
    previousGenerationPtr = &generationBuffer[!generationToggle][0][0];
}

void boringnessDetection() {
    pushGeneration(countLiveCells());

    if (patternRepeat())
        generations++;
    else
        generations = 0;

    if(generations == HISTORY_GENERATIONS*20)
    {
        for (int x = 13; x < 21; x++) {
            for (int y = 13; y < 21; y++) {
                generationBuffer[generationToggle][x][y] = rand()%2;
            }
        }
        generations = 0;
    }
}

// Test if the pattern of total live cells in history duplicates
uint8_t patternRepeat(void) {
	uint8_t repeat = 0;

	for (int genPatternLength = HISTORY_GENERATIONS/2; genPatternLength >= 1; genPatternLength--)
	{
		int notsame = 0;

        // test for duplicate pattern
		for (int gen = genPatternLength-1; gen >= 0; gen--)
			if (history[gen] != history[genPatternLength+gen])
				notsame++;

		if (!notsame)
		{
		    // two patterns repeat
			repeat = 1;
		}
	}
	return repeat;
}

// Push generation live cell total on top of history stack
void pushGeneration(uint16_t total) {
	for (uint8_t x = HISTORY_GENERATIONS - 1; x > 0; x--)
		history[x-1] = history[x];

	history[HISTORY_GENERATIONS - 1] = total;
}

// Count the number of live cells in a generation
unsigned int countLiveCells() {
	uint16_t total = 0;
	uint8_t x,y;
	for(x=1; x < 33; x++)
	{
		for(y=1; y < 33; y++)
		{
			if (generationBuffer[generationToggle][x-1][y-1]) {
    			total++;
			}
		}
	}
	return total;
}

// Set time from RTC
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

// TODO: Add patterns to editor
// TODO: Add more C++ concepts
