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
        currentGenerationPtr[x] = rand()%2 ? 0x00 : 0xff;
//        currentGenerationPtr[x] = 0x00;
    }

//    currentGenerationPtr[34*16+8] = 0xff;
//    currentGenerationPtr[34*16+9] = 0xff;
//    currentGenerationPtr[34*16+10] = 0xff;
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
//    return generationBuffer[!generationToggle][x][y];

    uint16_t count = 0;

    count += generationBuffer[!generationToggle][x-1][y-1] ? 1 : 0;
    count += generationBuffer[!generationToggle][x][y-1] ? 1 : 0;
    count += generationBuffer[!generationToggle][x+1][y-1] ? 1 : 0;
    count += generationBuffer[!generationToggle][x-1][y] ? 1 : 0;
    count += generationBuffer[!generationToggle][x+1][y] ? 1 : 0;
    count += generationBuffer[!generationToggle][x-1][y+1] ? 1 : 0;
    count += generationBuffer[!generationToggle][x][y+1] ? 1 : 0;
    count += generationBuffer[!generationToggle][x+1][y+1] ? 1 : 0;

//    count = count >> 8;

    return (count == 3 || (count == 2 && generationBuffer[!generationToggle][x][y])) ? 0xff : 0x00;
}

// TODO: Random number seed
// TODO: Boredom detector
// TODO: Colors
