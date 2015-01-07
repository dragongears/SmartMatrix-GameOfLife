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

// IR Raw Key Codes for Adafruit remote
#define ADAFRUIT_KEY_1               0xFD08F7
#define ADAFRUIT_KEY_2               0xFD8877
#define ADAFRUIT_KEY_3               0xFD48B7
#define ADAFRUIT_KEY_4               0xFD28D7
#define ADAFRUIT_KEY_5               0xFDA857
#define ADAFRUIT_KEY_6               0xFD6897
#define ADAFRUIT_KEY_7               0xFD18E7
#define ADAFRUIT_KEY_8               0xFD9867
#define ADAFRUIT_KEY_9               0xFD58A7
#define ADAFRUIT_KEY_0               0xFD30CF
#define ADAFRUIT_KEY_DOWN            0xFDB04F
#define ADAFRUIT_KEY_LEFT            0xFD10EF
#define ADAFRUIT_KEY_UP              0xFDA05F
#define ADAFRUIT_KEY_RIGHT           0xFD50AF
#define ADAFRUIT_KEY_BACK            0xFD708F
#define ADAFRUIT_KEY_ENTER           0xFD906F
#define ADAFRUIT_KEY_SETUP           0xFD20DF
#define ADAFRUIT_KEY_PAUSE           0xFD807F
#define ADAFRUIT_KEY_STOP            0xFD609F
#define ADAFRUIT_KEY_VOLUMEUP        0xFD40BF
#define ADAFRUIT_KEY_VOLUMEDOWN      0xFD00FF

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
            case ADAFRUIT_KEY_VOLUMEUP:
                if (singleStep) {
                    singleStep = false;
                } else {
                    if (speed > 10) {
                        speed -= 10;
                    }
                    showSpeed();
                }
            break;

            case ADAFRUIT_KEY_VOLUMEDOWN:
                if (singleStep) {
                    singleStep = false;
                } else {
                    if (speed < 140) {
                        speed += 10;
                    }
                    showSpeed();
                }
            break;

            case ADAFRUIT_KEY_PAUSE:
                if (singleStep) {
                    advanceGeneration();
                } else {
                    singleStep = true;
                }
            break;

            case ADAFRUIT_KEY_ENTER:
                color++;
                if (color > 7) {
                    color = 1;
                }
                displayCurrentGeneration();
                delay(100);
            break;

            case ADAFRUIT_KEY_BACK:
                wrap = !wrap;
                showWrap();
            break;

            case ADAFRUIT_KEY_DOWN:
                if (brightness > 10) {
                    brightness -= 10;
                    matrix.setBrightness(brightness*(255/100));
                }
                showBrightness();
            break;

            case ADAFRUIT_KEY_UP:
                if (brightness < 100) {
                    brightness += 10;
                    matrix.setBrightness(brightness*(255/100));
                }
                showBrightness();
            break;

            case ADAFRUIT_KEY_STOP:
                editStart();
            break;

            case ADAFRUIT_KEY_SETUP:
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

            case ADAFRUIT_KEY_PAUSE:
                editEnd();
            break;

            case ADAFRUIT_KEY_BACK:
                for (int x = 0; x < 34*34; x++) {
                    currentGenerationPtr[x] = 0;
                }
                moveEditCursor(0, 0);
            break;

            case ADAFRUIT_KEY_1:
                generationBuffer[generationToggle][editX+1][editY+1] = 1;
                moveEditCursor(0, 0);
            break;

            case ADAFRUIT_KEY_0:
                generationBuffer[generationToggle][editX+1][editY+1] = 0;
                moveEditCursor(0, 0);
            break;

            case ADAFRUIT_KEY_LEFT:
                moveEditCursor(-1, 0);
            break;

            case ADAFRUIT_KEY_RIGHT:
                moveEditCursor(1, 0);
            break;

            case ADAFRUIT_KEY_UP:
                moveEditCursor(0, -1);
            break;

            case ADAFRUIT_KEY_DOWN:
                moveEditCursor(0, 1);
            break;

            case ADAFRUIT_KEY_ENTER:
                editX = 16;
                editY = 16;
                moveEditCursor(0, 0);
            break;

            case ADAFRUIT_KEY_SETUP:
                randomizeField();
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
