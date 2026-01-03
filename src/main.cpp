// File: obs_ble_client.ino

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>


#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>


// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 13

// Max is 255, 32 is a conservative value to not overload
// a USB power supply (500mA) for 12x12 pixels.
//#define BRIGHTNESS 255

// Define matrix width and height.
#define mw 32
#define mh 8

#define LED_BLACK    0

#define NUM_MODES = 2;
volatile uint8_t mode;


// 'obs', 32x8px
const unsigned char epd_bitmap_obs[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0xe3, 0xc3, 0x80, 0x43, 0x32, 0x24, 0x04, 0x42, 0x12, 0x24, 0x04,
    0x7a, 0x13, 0xc3, 0x3c, 0x42, 0x12, 0x20, 0x84, 0x43, 0x32, 0x20, 0x84, 0x01, 0xe3, 0xc7, 0x00
};

#ifndef HORIZONTAL_START
#define HORIZONTAL_START NEO_MATRIX_RIGHT
#define VERTICAL_START NEO_MATRIX_BOTTOM
#endif

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoMatrix* matrix = new Adafruit_NeoMatrix(mw, mh, PIN,
    HORIZONTAL_START + VERTICAL_START +
    NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
    NEO_GRB + NEO_KHZ800);

// ==== OBS Service & Characteristic UUIDs ====
static const BLEUUID OBS_SERVICE_UUID("1FE7FAF9-CE63-4236-0004-000000000000");
static const BLEUUID OBS_TIME_UUID("1FE7FAF9-CE63-4236-0004-000000000001");
static const BLEUUID OBS_SENSOR_DIST_UUID("1FE7FAF9-CE63-4236-0004-000000000002");
static const BLEUUID OBS_CLOSE_PASS_UUID("1FE7FAF9-CE63-4236-0004-000000000003");
static const BLEUUID OBS_OFFSET_UUID("1FE7FAF9-CE63-4236-0004-000000000004");
static const BLEUUID OBS_TRACK_ID_UUID("1FE7FAF9-CE63-4236-0004-000000000005");

// ==== Globals for OBS data ====
// Handlebar offsets (cm), reported as two uint16: left, then right
volatile uint16_t g_offsetLeftCm = 0;
volatile uint16_t g_offsetRightCm = 0;

// Latest sensor distance (cm) you asked for: we store the minimum
volatile int g_latestDistanceCm = -1;

// If you also want raw values:
volatile uint16_t g_leftDistCm = 0xFFFF;
volatile uint16_t g_rightDistCm = 0xFFFF;

// BLE globals
static BLEAddress* pServerAddress = nullptr;
static bool doConnect = false;
static bool connected = false;
static bool scrolling = false;
static BLEClient* pClient = nullptr;
static BLERemoteCharacteristic* pSensorDistChar = nullptr;
static BLERemoteCharacteristic* pOffsetChar = nullptr;

volatile uint8_t brightness = 64;

// ===== Helper: parse little-endian uint16/uint32 =====
static uint16_t readLE16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t readLE32(const uint8_t* p) {
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);
}

// ===== Notification callback for Sensor Distance =====
// Payload: time uint32, left uint16, right uint16 (total 8 bytes)
static void sensorDistanceNotifyCallback(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {

    if (length < 8) {
        // Unexpected size; ignore
        return;
    }

    uint32_t timeMs = readLE32(pData);       // not stored, but available
    uint16_t leftCm = readLE16(pData + 4);
    uint16_t rightCm = readLE16(pData + 6);

    // Store raw values
    g_leftDistCm = leftCm;
    g_rightDistCm = rightCm;

    // Compute a single distance in cm for your global int:
    // Example: minimum of non-0xFFFF values, or -1 if both invalid.
    int best = -1;
    if (leftCm != 0xFFFF) best = (best < 0) ? leftCm : min(best, (int)leftCm);
    if (rightCm != 0xFFFF) best = (best < 0) ? rightCm : min(best, (int)rightCm);

    g_latestDistanceCm = best;

    // Debug print (optional)
    // Serial.printf("SensorDist: t=%lu ms, L=%u cm, R=%u cm, best=%d\n",
    //               (unsigned long)timeMs, leftCm, rightCm, g_latestDistanceCm);
}

// ===== Scan callback: find a device advertising the OBS service =====
class OBSAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        // Check if this device advertises our OBS service UUID
        if (advertisedDevice.haveServiceUUID() &&
            advertisedDevice.isAdvertisingService(OBS_SERVICE_UUID)) {

            Serial.print("Found OBS device: ");
            Serial.println(advertisedDevice.toString().c_str());

            pServerAddress = new BLEAddress(advertisedDevice.getAddress());
            doConnect = true;

            // Stop scanning once we found our device
            BLEDevice::getScan()->stop();
        }
    }
};

// ===== Connect and set up remote characteristics, read Offset, enable notify =====
bool connectToServer() {
    Serial.print("Connecting to OBS device at ");
    Serial.println(pServerAddress->toString().c_str());

    pClient = BLEDevice::createClient();

    if (!pClient->connect(*pServerAddress)) {
        Serial.println("Failed to connect.");
        return false;
    }

    Serial.println("Connected to server.");

    // Get the OBS service
    BLERemoteService* pService = pClient->getService(OBS_SERVICE_UUID);
    if (pService == nullptr) {
        Serial.println("Failed to find OBS service.");
        pClient->disconnect();
        return false;
    }

    // Get characteristics
    pSensorDistChar = pService->getCharacteristic(OBS_SENSOR_DIST_UUID);
    pOffsetChar = pService->getCharacteristic(OBS_OFFSET_UUID);

    if (pSensorDistChar == nullptr || pOffsetChar == nullptr) {
        Serial.println("Failed to find required characteristics.");
        pClient->disconnect();
        return false;
    }

    // --- Read Offset (2 x uint16: left, right) ---
    if (pOffsetChar->canRead()) {
        std::string value = pOffsetChar->readValue();
        if (value.size() >= 4) {
            const uint8_t* data = (const uint8_t*)value.data();
            uint16_t leftOffset = readLE16(data + 0);
            uint16_t rightOffset = readLE16(data + 2);

            g_offsetLeftCm = leftOffset;
            g_offsetRightCm = rightOffset;

            Serial.printf("Offset read: left=%u cm, right=%u cm\n",
                g_offsetLeftCm, g_offsetRightCm);
        }
        else {
            Serial.println("Offset characteristic too short.");
        }
    }
    else {
        Serial.println("Offset characteristic not readable.");
    }

    // --- Enable notifications for Sensor Distance ---
    if (pSensorDistChar->canNotify()) {
        pSensorDistChar->registerForNotify(sensorDistanceNotifyCallback);
        Serial.println("Registered for Sensor Distance notifications.");
    }
    else {
        Serial.println("Sensor Distance characteristic cannot notify.");
    }

    return true;
}

// ===== Arduino setup() =====
void setup() {
    Serial.begin(115200);
    matrix->begin();
    matrix->setTextWrap(false);
    matrix->setBrightness(brightness);
    // Test full bright of all LEDs. If brightness is too high
    // for your current limit (i.e. USB), decrease it.
    matrix->clear();

    matrix->drawBitmap(0, 0, epd_bitmap_obs, 32, 8, matrix->Color(0, 255, 0));
    matrix->show();
    //matrix->fillScreen(LED_WHITE_HIGH);
    //matrix->show();
    //delay(1000);


    // Configure GPIO pins as inputs with internal pull-ups enabled
    pinMode(15, INPUT_PULLDOWN);
    pinMode(2, INPUT_PULLDOWN);
    pinMode(4, INPUT_PULLDOWN);


    Serial.println("Starting ESP32 OBS BLE client...");

    // Initialize BLE and set client device name
    BLEDevice::init("ESP32_OBS_Client");

    // Start scanning for BLE peripherals advertising our service
    BLEScan* pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new OBSAdvertisedDeviceCallbacks());
    pScan->setInterval(1349);
    pScan->setWindow(449);
    pScan->setActiveScan(true);  // active scan to get more info
    pScan->start(5, false);      // scan for 5 seconds, don't stop on own

    Serial.println("Scanning for OBS device...");
}

void setColorByDistance(uint16_t dist) {
    if ((g_leftDistCm > 600)) {
        matrix->setTextColor(matrix->Color(160, 160, 160));
    }
    else if (dist < 150) {
        matrix->setTextColor(matrix->Color(255, 0, 0));
    }
    else if (dist < 200) {
        matrix->setTextColor(matrix->Color(255, 255, 0));
    }
    else {
        matrix->setTextColor(matrix->Color(0, 255, 0));
    }
}


void reactToKeys(int pin15State, int pin2State, int pin4State) {
    static bool k15press, k2press, k4press;
    Serial.println(k15press);

    if (k15press) {
        matrix->drawPixel(27, 0, matrix->Color(0, 255, 0));
        matrix->drawPixel(26, 0, matrix->Color(0, 255, 0));
    }
    if (k2press) {
        matrix->drawPixel(25, 0, matrix->Color(0, 0, 255));
        matrix->drawPixel(24, 0, matrix->Color(0, 0, 255));
    }
    if (k4press) {
        matrix->drawPixel(23, 0, matrix->Color(125, 125, 125));
        matrix->drawPixel(22, 0, matrix->Color(125, 125, 125));
    }
    static long int last_keypress;
    if (millis() - last_keypress < 250) {
        return;
    }

    if (pin15State) {
        brightness = max(brightness / 2, 1);
        k15press = true;
        last_keypress = millis();
    }
    else {
        k15press = false;
    }
    if (pin2State) {
        brightness = min(brightness * 2, 255);
        k2press = true;
        last_keypress = millis();
    }
    else {
        k2press = false;
    }
    if (pin4State) {
        k4press = true;
        last_keypress = millis();
        scrolling = !scrolling;
    }
    else {
        k4press = false;
    }
}


// ===== Arduino loop() =====
void loop() {
    int pin15State = digitalRead(15);
    int pin2State = digitalRead(2);
    int pin4State = digitalRead(4);
    // If we have discovered the server and not yet connected, connect now
    if (doConnect && !connected && pServerAddress != nullptr) {
        if (connectToServer()) {
            Serial.println("We are now connected to the OBS server.");
            connected = true;
        }
        else {
            Serial.println("Connection failed; restarting scan.");
            connected = false;
            doConnect = false;
            BLEDevice::getScan()->start(5, false);
        }
    }

    // Here you can use g_offsetLeftCm, g_offsetRightCm, g_latestDistanceCm, etc.
    // Example: print periodically
    static unsigned long lastPrint = 0;


    matrix->fillScreen(0);
    //matrix->setFont(&FreeMonoBold9pt7b);
    matrix->setCursor(1, 0);
    int dist = max(g_leftDistCm - g_offsetLeftCm, 0);
    char display[16] = { 0 };

    if (connected) {
        sprintf(display, "%3dcm", dist);
    }
    else {
        matrix->drawBitmap(scrolling*(32-(millis() / 100) % 64), 0, epd_bitmap_obs, 32, 8, matrix->Color(0, 255, 0));
        for (uint8_t i = 0; i < 255; i++)
        {
            if(matrix->getPixelColor(i)>0) {
                matrix->setPixelColor(i,matrix->ColorHSV((uint16_t)256*(i-(millis()/20)),255,brightness));
                Serial.println(i);
            }
        }
    }
    reactToKeys(pin15State, pin2State, pin4State);

    matrix->setBrightness(brightness);

    if (connected) {
        if ((g_leftDistCm > 600)) {
            sprintf(display, "---cm", dist);
        }
        setColorByDistance(dist);
        matrix->print(display);
    }

    matrix->show();

    unsigned long now = millis();
    if (now - lastPrint > 2000) {
        lastPrint = now;
        Serial.printf("Offsets L/R: %u / %u cm, latest distance: %d cm %d (L=%u, R=%u)\n",
            g_offsetLeftCm, g_offsetRightCm,
            g_latestDistanceCm, dist, g_leftDistCm, g_rightDistCm);
    }
}
