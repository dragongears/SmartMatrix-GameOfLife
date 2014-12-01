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

    for (int x = 0; x < 34*34; x++) {
        currentGenerationPtr[x] = rand()%2 ? 0x00 : 0xff;
    }

}

// the loop() method runs over and over again,
// as long as the board has power
void loop() {
    // Copy current generation to previous generation
    for (int x = 0; x < 34*34; x++) {
        previousGenerationPtr[x] = currentGenerationPtr[x];
    }

    // Fill outside borders
//    for (int x = 0; x < 32; x++) {
//
//    }

    // Do calculation

    // Display current generation
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
          uint8_t cell = generationBuffer[generationToggle][x][y];
          cell ? matrix.drawPixel(x, y, white) : matrix.drawPixel(x, y, black);
        }
    }

    matrix.swapBuffers(false);

    // Swap generation buffers

    // Delay
    delay(5000);
}

void swapGenerationBuffer() {
    generationToggle = !generationToggle;
    currentGenerationPtr = &generationBuffer[generationToggle][0][0];
    previousGenerationPtr = &generationBuffer[!generationToggle][0][0];
}

// TODO: Random number seed
// TODO: Boredom detector
// TODO: Colors
