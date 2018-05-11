/*
 *      Copyright 2018 Particle Industries, Inc.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 *
 */
/*
 *      Project: #PartiBadge, 2018 Maker Faire Edition
 *
 *      Description:
 *          This is the firmware for a custom, Electron-based badge PCB that includes:
 *              1. A 1.7" LCD TFT
 *              2. An SMD Piezo buzzer
 *              3. A SPDT Switch
 *              4. A 5-way joystick
 *              5. An SMD Si7021 temperature and Humidity sensor
 *              6. 4 Tactile LED Buttons in Red, Blue, Green and Yello/Orange
 *              7. An I2C-Compatible breakout for #BadgeLife add-ons
 *          The exact functionality of this badge includes:
 *              1.
 */

#include "Particle.h"
#include "Debounce.h"

#include "parti-badge.h" // #define pin assignments
#include "music/tones.h" // Peizo Sounds

// Custom code for Si7021 Temp/Hu Sensor using Wire1 on Electron C4, C5
#include "Si7021_MultiWire/Si7021_MultiWire.h"

// TFT include
#include "Adafruit_ST7735.h"

Debounce displayDebouncer = Debounce();
Debounce gameDebouncer = Debounce();

// Initialize objects from the lib
Si7021_MultiWire envSensor = Si7021_MultiWire();
double currentTemp;
double currentHumidity;

// Initialize TFT Display
Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Battery reading variables
FuelGauge fuel;
int currentBatteryCharge;

// Timing variables
unsigned long elapsedTime;
unsigned long startTime = 0;
unsigned long previousEnvReading = 0;
unsigned long previousBattReading = 0;

// Wearer details
String wearerName;
String wearerEmail;
String wearerHandle;

// Default to display mode, but we'll determine this based on a switch
int badgeMode = DISPLAY_MODE;

void setup() {
  Serial.begin(115200);

  pinMode(RED_BUTTON, OUTPUT);
  digitalWrite(RED_BUTTON, HIGH);

  //Init TFT
  initDisplay();

  //Initialize Temp and Humidity sensor
  envSensor.begin();

  cloudInit();

  pinMode(BUZZER_PIN, OUTPUT);
  displayDebouncer.attach(DISPLAY_MODE_PIN, INPUT_PULLDOWN);
  displayDebouncer.interval(20);

  gameDebouncer.attach(GAME_MODE_PIN, INPUT_PULLDOWN);
  gameDebouncer.interval(20);

  // Play a startup sound on the Piezo
  playStartup(BUZZER_PIN);

  // Get an initial temp and humidity reading
  getTempAndHumidity();

  // Perform an initial battery check
  checkBattery();
}

void loop() {
  unsigned long currentMillis = millis();

  // Check the switch to see if the user has changed the badge mode
  checkBadgeMode();

  if (currentMillis - previousEnvReading > TEMP_CHECK_INTERVAL) {
    previousEnvReading = currentMillis;
    getTempAndHumidity();
  }

  if (currentMillis - previousBattReading > BATT_CHECK_INTERVAL) {
    previousBattReading = currentMillis;
    checkBattery();
  }
}

void initDisplay() {
  display.initG();
  display.setRotation(3);

  pinMode(TFT_LIGHT, OUTPUT);
  digitalWrite(TFT_LIGHT, HIGH);

  display.fillScreen(ST7735_WHITE);
  display.setCursor(0, 0);
  display.setTextColor(ST7735_RED);
  display.setTextWrap(true);
  display.setTextSize(2);

  display.println();
  display.println();
  display.println(" #PartiBadge");
  display.println(" v1.0");
  display.println();
  display.println(" BAMF Edition");
}

void cloudInit() {
  Particle.variable("wearerName", wearerName);
  Particle.variable("wearerEmail", wearerEmail);
  Particle.variable("wearerHandle", wearerHandle);

  Particle.variable("currentTemp", currentTemp);
  Particle.variable("currentHu", currentHumidity);

  Particle.variable("battCharge", currentBatteryCharge);

  Particle.subscribe("updateName", updateNameHandler);
  Particle.subscribe("updateEmail", updateEmailHandler);
  Particle.subscribe("updateHandle", updateHandleHandler);
}

void checkBadgeMode() {
  displayDebouncer.update();
  gameDebouncer.update();

  if (displayDebouncer.read() == HIGH) {
    badgeMode = DISPLAY_MODE;
  } else if (gameDebouncer.read() == HIGH) {
    badgeMode = GAME_MODE;
  }
}

void getTempAndHumidity() {
  currentTemp = envSensor.readTempF();
  currentHumidity = envSensor.getRH();
}

void checkBattery() {
  currentBatteryCharge = (int)fuel.getSoC();

  if (currentBatteryCharge < BATTERY_CRITICAL) {
    // TODO: Show warning on TFT
  }

  if (currentBatteryCharge < BATTERY_SHUTOFF) {
    playGameOver(BUZZER_PIN);

    //Sleep the device to prevent the battery from fully discharging
    System.sleep(SLEEP_MODE_SOFTPOWEROFF);
  }
}

void updateNameHandler(const char *event, const char *data) {
  if (String(event) == "updateName") {
    wearerName = String(data);
  }
}

void updateEmailHandler(const char *event, const char *data) {
  if (String(event) == "updateEmail") {
    wearerEmail = String(data);
  }
}
void updateHandleHandler(const char *event, const char *data) {
  if (String(event) == "updateHandle") {
    wearerHandle = String(data);
  }
}
