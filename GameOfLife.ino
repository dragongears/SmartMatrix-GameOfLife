/*
	SmartMatrix Game of Life
*/

// 9|789|7
// -+---+-
// 3|123|1
// 6|456|4
// 9|789|7
// -+---+-
// 3|123|1

#include <SmartMatrix_32x32.h>

#define HISTORY_GENERATIONS 10

uint16_t history[HISTORY_GENERATIONS];
uint16_t generations=0;



SmartMatrix matrix;

const int defaultBrightness = 100*(255/100);    // full brightness
//const int defaultBrightness = 15*(255/100);    // dim: 15% brightness
const rgb24 black = {0, 0, 0};
const rgb24 white = {0xff, 0xff, 0xff};

// Teensy 3.1 has the LED on pin 13
const int ledPin = 13;

uint8_t generationBuffer[2][34][34];
uint8_t generationToggle = 0;
uint8_t *currentGenerationPtr;
uint8_t *previousGenerationPtr;

// the setup() method runs once, when the sketch starts
void setup() {
    // initialize the digital pin as an output.
    pinMode(ledPin, OUTPUT);

    Serial.begin(38400);

    matrix.begin();
    matrix.setBrightness(defaultBrightness);

    // Initialize generation buffer
    swapGenerationBuffer();

    srand(108);

    for (int x = 0; x < 34*34; x++) {
        currentGenerationPtr[x] = rand()%2;
    }
}

// the loop() method runs over and over again, as long as the board has power
void loop() {

    // Swap generation buffers (current generation becomes previous generation)
    swapGenerationBuffer();


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

    // Do next generation calculation from previous generation into current generation
    for (int x = 1; x < 33; x++) {
        for (int y = 1; y < 33; y++) {
          generationBuffer[generationToggle][x][y] = getCellStatus(x, y);
        }
    }

    // Boringness detection
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

    // Display current generation
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
          uint8_t cell = generationBuffer[generationToggle][x][y];
          cell ? matrix.drawPixel(x, y, white) : matrix.drawPixel(x, y, black);
        }
    }

    matrix.swapBuffers(false);

    // Delay
    delay(100);
}

void swapGenerationBuffer() {
    generationToggle = !generationToggle;
    currentGenerationPtr = &generationBuffer[generationToggle][0][0];
    previousGenerationPtr = &generationBuffer[!generationToggle][0][0];
}

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

// Test if the pattern of total live cells in history duplicates
unsigned int patternRepeat(void)
{
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
void pushGeneration(uint16_t total)
{
	for (uint8_t x = HISTORY_GENERATIONS - 1; x > 0; x--)
		history[x-1] = history[x];

	history[HISTORY_GENERATIONS - 1] = total;
}

// Count the number of live cells in a generation
unsigned int countLiveCells()
{
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

// TODO: Random number seed
// TODO: Colors
