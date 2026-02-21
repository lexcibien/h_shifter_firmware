#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

// Fim de curso do H-shifter - Vista de cima com a primeira marcha topo esquerda
const uint8_t SW_FRONT = 3;
const uint8_t SW_LEFT = 4;
const uint8_t SW_RIGHT = 5;
const uint8_t SW_BACK = 6;
const uint8_t SW_REVERSE = 7;

// Botões da manopla de caminhão
const uint8_t SW_KNOB_RANGE = 8;
const uint8_t SW_KNOB_SPLIT = 9;
const uint8_t BTN_KNOB_ENGINE_BRAKE = 10;

const uint8_t SW_ENABLE_REVERSE = 11;
const uint8_t SW_ENABLE_SEQUENTIAL = 12;


static const uint8_t LED_BUILTIN = 7;
#define BUILTIN_LED  LED_BUILTIN // backward compatibility
#define LED_BUILTIN LED_BUILTIN  // allow testing #ifdef LED_BUILTIN

static const uint8_t TX = 21;
static const uint8_t RX = 20;

static const uint8_t SDA = 8;
static const uint8_t SCL = 9;

static const uint8_t SS = 7;
static const uint8_t MOSI = 6;
static const uint8_t MISO = 5;
static const uint8_t SCK = 4;

static const uint8_t A0 = 0;
static const uint8_t A1 = 1;
static const uint8_t A2 = 2;
static const uint8_t A3 = 3;
static const uint8_t A4 = 4;
static const uint8_t A5 = 5;

#endif /* Pins_Arduino_h */
