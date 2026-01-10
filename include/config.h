#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <version.h>

// Pin connected to the NeoPixels
#define PIN 13

// Matrix width and height
#define mw 32
#define mh 8

// Colors and modes
#define LED_BLACK 0

#ifndef HORIZONTAL_START
#define HORIZONTAL_START NEO_MATRIX_RIGHT
#define VERTICAL_START NEO_MATRIX_BOTTOM
#endif

// OBS BLE Service & Characteristic UUIDs
static const BLEUUID OBS_SERVICE_UUID("1FE7FAF9-CE63-4236-0004-000000000000");
static const BLEUUID OBS_TIME_UUID("1FE7FAF9-CE63-4236-0004-000000000001");
static const BLEUUID OBS_SENSOR_DIST_UUID("1FE7FAF9-CE63-4236-0004-000000000002");
static const BLEUUID OBS_CLOSE_PASS_UUID("1FE7FAF9-CE63-4236-0004-000000000003");
static const BLEUUID OBS_OFFSET_UUID("1FE7FAF9-CE63-4236-0004-000000000004");
static const BLEUUID OBS_TRACK_ID_UUID("1FE7FAF9-CE63-4236-0004-000000000005");

// Globals that might be used across translation units
extern volatile uint8_t mode;
extern volatile uint16_t g_offsetLeftCm;
extern volatile uint16_t g_offsetRightCm;
extern volatile int g_latestDistanceCm;
extern volatile uint16_t g_leftDistCm;
extern volatile uint16_t g_rightDistCm;
extern volatile uint8_t brightness;
extern Adafruit_NeoMatrix* matrix;

// Forward declarations
bool connectToServer();
void setColorByDistance(uint16_t dist);
void reactToKeys(int pin15State, int pin2State, int pin4State);
void rainbowify(Adafruit_NeoMatrix* matrix);

#endif // CONFIG_H