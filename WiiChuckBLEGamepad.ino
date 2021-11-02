#include <bluefruit.h>
#include <WiiChuck.h>

BLEDis bledis;
BLEHidGamepad blegamepad;

// defined in hid.h from Adafruit_TinyUSB_Arduino
hid_gamepad_report_t last_gamepad_report = {0};

Accessory nunchuck;

void setup() {
    Serial.begin(115200);

    setupNunchuck();
    setupBLE();
}

void loop() {
    if (!Bluefruit.connected()) {
        return;
    }
    
    nunchuck.readData();

    uint8_t rawX = nunchuck.values[0];
    uint8_t rawY = nunchuck.values[1];

    const uint8_t minX = 27;
    const uint8_t maxX = 226;
    const uint8_t minY = 32;
    const uint8_t maxY = 221;    

    rawX = max(rawX, minX);
    rawX = min(rawX, maxX);
    
    rawY = max(rawY, minY);
    rawY = min(rawY, maxY);

    uint8_t mappedX = map(rawX, minX, maxX, 0, 255);
    uint8_t mappedY = map(rawY, minX, maxX, 0, 255);

    int8_t joyX = (int16_t)mappedX - 128;
    int8_t joyY = (int16_t)mappedY - 128;

    const int8_t deadZone = 12;

    if (joyX <= -deadZone) {
        joyX = map(joyX, -deadZone, -128, 0, -128);
    } else if (joyX >= deadZone) {
        joyX = map(joyX, deadZone, 127, 0, 127);
    } else {
        joyX = 0;
    }

    if (joyY <= -deadZone) {
        joyY = map(joyY, -deadZone, -128, 0, -128);
    } else if (joyY >= deadZone) {
        joyY = map(joyY, deadZone, 127, 0, 127);
    } else {
        joyY = 0;
    }

    hid_gamepad_report_t report = {0};

    report.x = joyX;
    report.y = joyY;

    if (nunchuck.values[10]) {
        report.buttons |= GAMEPAD_BUTTON_Z;
    }

    if (nunchuck.values[11]) {
        report.buttons |= GAMEPAD_BUTTON_C;
    }

    if ( memcmp(&last_gamepad_report, &report, sizeof(hid_gamepad_report_t)) ) {
        blegamepad.report(&report);
        memcpy(&last_gamepad_report, &report, sizeof(hid_gamepad_report_t));
        delay(5);
    }
}

bool setupNunchuck() {
    nunchuck.begin();

    // Currently only support the nunchuck.
    if (nunchuck.type != NUNCHUCK) {
        return false;
    }

    // Attempt to read; if we fail, return false.
    if (!nunchuck.readData()) {
        return false;
    }

    return true;
}

void setupBLE() {
    Bluefruit.begin();
    Bluefruit.setTxPower(4);
    Bluefruit.setName("WiiChuck-BLE");

    // Configure and Start Device Information Service
    bledis.setManufacturer("Dharma Institute");
    bledis.setModel("WiiChuck BLE");
    bledis.begin();

    // Configure gamepad
    blegamepad.begin();

    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    // Set up and start advertising
    startAdv();    
}

void startAdv(void) {
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_GAMEPAD);
    
    // Include BLE HID service
    Bluefruit.Advertising.addService(blegamepad);
    
    // There is enough room for the dev name in the advertising packet
    if (!Bluefruit.Advertising.addName()) {
        Bluefruit.ScanResponse.addName();
    }
    
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    (void) conn_handle;
    (void) reason;

    hid_gamepad_report_t report = {0};
    memcpy(&last_gamepad_report, &report, sizeof(hid_gamepad_report_t));    
}
