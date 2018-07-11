/*
 *      Copyright 2018 Particle Industries, Inc.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */
/*
 *      Project: #PartiBadge, 2018 THAT Conference Edition
 *
 *      Description:
 *          This is the firmware for a custom, Photon-based badge PCB that includes:
 *              1. A 1" OLED Screen
 *              2. An SMD Piezo buzzer
 *              3. A SPDT Switch
 *              4. A 5-way joystick
 *              5. An SMD Si7021 temperature and Humidity sensor
 *              6. 4 Tactile LED Buttons in Red, Blue, Green and Yello/Orange
 *              7. An I2C-Compatible breakout for #BadgeLife add-ons
 *
 */

#include "Particle.h"
#include "Debounce.h"

#include "parti-badge.h" // #define pin assignments and other general macros
#include "music/tones.h" // Peizo Sounds
#include "music/roll.h"

#include "Particle_SI7021.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

// TODO: Set-up new product
// PRODUCT_ID(7461);
// PRODUCT_VERSION(2);

String deviceId;

// Button Debounce Support
Debounce displayDebouncer = Debounce();
Debounce redButtonADebouncer = Debounce();
Debounce blueButtonBDebouncer = Debounce();
Debounce greenButtonCDebouncer = Debounce();
Debounce yellowButtonDDebouncer = Debounce();
#define DEBOUNCE_DELAY 20

// Initialize Si7021 sensor
SI7021 envSensor;
double currentTemp;
double currentHumidity;

#include "simonsays/simon.h" // Simon Says Code

// Timing variables
unsigned long elapsedTime;
unsigned long startTime = 0;
unsigned long previousEnvReading = 0;

// Wearer details
String wearerFirstName;
String wearerLastName;

// Default to display mode, but we'll determine this based on a switch
unsigned long meshImagesTriggerTime = 0;
unsigned long wearerDetailsTriggerTime = 0;

// Display variables
bool displayingTemp = false;
bool displayingLogo = false;
bool displayingTitle = false;
bool displayingWearerDetails = false;

// Display state management
bool titleShown = false;
bool buttonsInitialized = false;


void setup() {
  resetDisplayBools();

  // Get the current deviceId
  deviceId = System.deviceID();

  //Initialize Temp and Humidity sensor
  envSensor.begin();

  //Init OLED
  // TODO: Add

  // Set up cloud variables and functions
  cloudInit();

  rollSetup();

  // Show the Particle Logo on the screen
  showLogo();

  pinMode(BUZZER_PIN, OUTPUT);
  // displayDebouncer.attach(DISPLAY_MODE_PIN, INPUT_PULLDOWN);
  // displayDebouncer.interval(DEBOUNCE_DELAY);

  // Init the LED Buttons
  initButtons();

  // Get an initial temp and humidity reading
  getTempAndHumidity();

  // Show the title screen
  showTitle();

  //Init Tactile LED Buttons
  initLEDButtons();

  // Play a startup sound on the Piezo
  if (!startupSoundPlayed) playStartup(BUZZER_PIN);

  Particle.connect();
  /*
  checkBadgeMode();
  if (badgeMode == DISPLAY_MODE) {
    displayingMeshImages = true;
    meshImagesTriggerTime = millis();
  }
  */
}

void loop() {
  unsigned long currentMillis = millis();

  checkBadgeMode();

  //if (badgeMode == DISPLAY_MODE) {
    redButtonADebouncer.update();
    if (redButtonADebouncer.read() == LOW) {
      toggleAllButtons(LOW);
      digitalWrite(RED_LED, HIGH);
    }

    blueButtonBDebouncer.update();
    if (blueButtonBDebouncer.read() == LOW && ! displayingTemp) {
      //resetDisplayBools();
      displayingTemp = true;
      toggleAllButtons(LOW);
      digitalWrite(BLUE_LED, HIGH);

      // Show Temp and Humidity on Display
      showTempAndHumidity();
    }

    greenButtonCDebouncer.update();
    if (greenButtonCDebouncer.read() == LOW) {
      toggleAllButtons(LOW);
      digitalWrite(GREEN_LED, HIGH);

      // Show Name
      //showName();
      delay(1000);
      initButtons();
      attractMode();
    }

    yellowButtonDDebouncer.update();
    if (yellowButtonDDebouncer.read() == LOW) {
      wearerDetailsTriggerTime = millis();
      // resetDisplayBools();
      // displayingWearerDetails = true;

      toggleAllButtons(LOW);
      digitalWrite(YELLOW_LED, HIGH);
    }

    if (currentMillis - previousEnvReading > TEMP_CHECK_INTERVAL) {
      previousEnvReading = currentMillis;
      getTempAndHumidity();
    }
  //} else if (badgeMode == GAME_MODE) {
  //  configureGame();
  //
  //  playGame();
  //}
}

void cloudInit() {
  Particle.variable("wearerFName", wearerFirstName);
  Particle.variable("wearerLName", wearerLastName);

  Particle.variable("currentTemp", currentTemp);
  Particle.variable("currentHu", currentHumidity);

  Particle.function("updateFName", updateFirstNameHandler);
  Particle.function("updateLName", updateLastNameHandler);
}

void showLogo() {
  /*
  display.setCursor(0, 0);
  display.setRotation(3);

  bmpDraw("spark.bmp", 0, 0);
  delay(2000);
  */
}

void showTitle() {
  /*
  titleShown = true;

  display.setRotation(3);
  display.fillScreen(ST7735_WHITE);
  display.setCursor(0, 0);
  display.setTextColor(ST7735_BLUE);
  display.setTextWrap(true);
  display.setTextSize(2);

  display.println();
  display.println();
  display.println(" #PartiBadge");
  display.println(" v1.1");
  display.println();
  display.println(" THAT Conference");
  */
}

void displayWearerDetails() {
  /*
  display.fillScreen(ST7735_WHITE);
  display.setCursor(0, 0);
  display.setTextColor(ST7735_BLUE);
  display.setTextWrap(true);
  display.setTextSize(3);

  display.println();
  display.println(wearerFirstName);
  display.println(wearerLastName);
  */
}

void showName() {
  /*
  display.fillScreen(ST7735_BLACK);
  display.setCursor(0, 0);
  display.setTextColor(ST7735_WHITE);
  display.setTextWrap(true);
  display.setTextSize(3);

  display.println();
  display.println(" Brandon");
  display.println("");
  display.println(" Particle");
  */
}

void initButtons() {
  // Init Buttons as Inputs
  redButtonADebouncer.attach(RED_BUTTON_A, INPUT_PULLUP);
  redButtonADebouncer.interval(DEBOUNCE_DELAY);
  blueButtonBDebouncer.attach(BLUE_BUTTON_B, INPUT_PULLUP);
  blueButtonBDebouncer.interval(DEBOUNCE_DELAY);
  greenButtonCDebouncer.attach(GREEN_BUTTON_C, INPUT_PULLUP);
  greenButtonCDebouncer.interval(DEBOUNCE_DELAY);
  yellowButtonDDebouncer.attach(YELLOW_BUTTON_D, INPUT_PULLUP);
  yellowButtonDDebouncer.interval(DEBOUNCE_DELAY);
}

void initLEDButtons() {
  buttonsInitialized = true;

  int del = 300;
  int medDel = 500;

  // Init LEDs and Outputs
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);

  digitalWrite(RED_LED, HIGH);
  delay(del);
  digitalWrite(BLUE_LED, HIGH);
  delay(del);
  digitalWrite(GREEN_LED, HIGH);
  delay(del);
  digitalWrite(YELLOW_LED, HIGH);
  delay(del);

  toggleAllButtons(LOW);
  delay(medDel);

  toggleAllButtons(HIGH);
  delay(medDel);

  toggleAllButtons(LOW);
  delay(medDel);

  toggleAllButtons(HIGH);
}

void showTempAndHumidity() {
  /*
  clearScreen();

  display.println();
  display.println("  Curr Temp");
  display.setTextSize(3);
  display.print("  ");
  display.print((int)currentTemp);
  display.println("f");
  display.setTextSize(2);
  display.println();
  display.println("  Humidity");
  display.setTextSize(3);
  display.print("  ");
  display.print((int)currentHumidity);
  display.println("%");
  */
}

void toggleAllButtons(int state) {
  digitalWrite(RED_LED, state);
  digitalWrite(BLUE_LED, state);
  digitalWrite(GREEN_LED, state);
  digitalWrite(YELLOW_LED, state);
}

void resetDisplayBools() {
  displayingTemp = false;
  displayingWearerDetails = false;
  displayingLogo = false;
  displayingTitle = false;
}

void checkBadgeMode() {
  /*
  displayDebouncer.update();
  gameDebouncer.update();

  if (displayDebouncer.read() == HIGH) {
    badgeMode = DISPLAY_MODE;
  } else if (gameDebouncer.read() == HIGH) {
    badgeMode = GAME_MODE;
  }
  */
}

void getTempAndHumidity() {
  si7021_env sensorData = envSensor.getHumidityAndTemperature();
  currentTemp = sensorData.celsiusHundredths;
  currentHumidity = sensorData.humidityBasisPoints;
}

void clearScreen() {
  /*
  display.fillScreen(ST7735_BLACK);
  display.setCursor(0, 0);
  display.setTextColor(ST7735_WHITE);
  display.setTextWrap(true);
  display.setTextSize(2);
  */
}

int updateFirstNameHandler(String data) {
  wearerFirstName = data;

  return 1;
}

int updateLastNameHandler(String data) {
  wearerLastName = data;

  return 1;
}
