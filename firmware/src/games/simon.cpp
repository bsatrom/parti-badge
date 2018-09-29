/*
 Started: 6-19-2007
 Spark Fun Electronics
 Nathan Seidle

 Simon Says is a memory game. Start the game by pressing one of the four buttons. When a button lights up,
 press the button, repeating the sequence. The sequence will get longer and longer. The game is won after
 13 rounds.

 This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 Simon Says game originally written in C for the PIC16F88.
 Ported for the ATmega168, then ATmega328, then Arduino 1.0.
 Fixes and cleanup by Joshua Neal <joshua[at]trochotron.com>

 Migration to Particle ecosystem by Brandon Satrom <brandon@particle.io> from June to October, 2018.

 Generates random sequence, plays music, and displays button lights.

 Simon tones from Wikipedia
 - A (red, upper left) - 440Hz - 2.272ms - 1.136ms pulse
 - a (green, upper right, an octave higher than A) - 880Hz - 1.136ms,
 0.568ms pulse
 - D (blue, lower left, a perfect fourth higher than the upper left)
 587.33Hz - 1.702ms - 0.851ms pulse
 - G (yellow, lower right, a perfect fourth higher than the lower left) -
 784Hz - 1.276ms - 0.638ms pulse

 The tones are close, but probably off a bit, but they sound all right.

 The old version of SparkFun simon used an ATmega8. An ATmega8 ships
 with a default internal 1MHz oscillator.  You will need to set the
 internal fuses to operate at the correct external 16MHz oscillator.

 Original Fuses:
 avrdude -p atmega8 -P lpt1 -c stk200 -U lfuse:w:0xE1:m -U hfuse:w:0xD9:m

 Command to set to fuses to use external 16MHz:
 avrdude -p atmega8 -P lpt1 -c stk200 -U lfuse:w:0xEE:m -U hfuse:w:0xC9:m

 The current version of Simon uses the ATmega328. The external osciallator
 was removed to reduce component count.  This version of simon relies on the
 internal default 1MHz osciallator. Do not set the external fuses.
 */
#include "Particle.h"
#include "Adafruit_SSD1306.h"
#include "macros.h"
#include "display/display.h"
#include "interrupts/interrupts.h"
#include "inputs/inputs.h"
#include "leds/leds.h"
#include "music/music.h"

// Define game parameters
#define ROUNDS_TO_WIN 13      //Number of rounds to succesfully remember before you win. 13 is do-able.
#define ENTRY_TIME_LIMIT 3000 //Amount of time to press a button before game times out. 3000ms = 3 sec

#define MODE_MEMORY 0
#define MODE_BATTLE 1
#define MODE_BEEGEES 2

// Game state variables
byte gameMode = MODE_MEMORY; //By default, let's play the memory game
byte gameBoard[32];          //Contains the combination of buttons as we advance
byte gameRound = 0;          //Counts the number of succesful rounds the player has made it through

boolean gameConfigured = false;

extern Adafruit_SSD1306 display;
extern byte appmode;
extern byte btncounter;
extern byte btnid;

const char score[] = "Current Score";

void configureGame();
void playGame();
boolean play_memory(void);
void add_to_moves(void);
byte wait_for_button(void);
boolean play_battle(void);
void play_winner(void);
void winner_sound(void);
void play_loser(void);
void attractMode(void);
void playMoves(void);

void initSimon()
{
  appmode = 1;
  btnid = 0;

  configureGame();
  setupBackButtonInterrupt();

  while (appmode)
  {
    playGame();
  }
}

void configureGame()
{
  if (!gameConfigured)
  {
    //Setup hardware inputs/outputs. These pins are defined in the hardware_versions header file

    //Mode checking
    gameMode = MODE_MEMORY; // By default, we're going to play the memory game

    // Check to see if upper right button is pressed
    if (checkButton() == CHOICE_GREEN)
    {
      gameMode = MODE_BATTLE; //Put game into battle mode

      //Turn on the upper right (green) LED
      setLEDs(CHOICE_GREEN);
      toner(CHOICE_GREEN, 150);

      setLEDs(CHOICE_RED | CHOICE_BLUE | CHOICE_YELLOW); // Turn on the other LEDs until you release button

      while (checkButton() != CHOICE_NONE)
        ; // Wait for user to stop pressing button

      //Now do nothing. Battle mode will be serviced in the main routine
    }

    play_winner(); // After setup is complete, say hello to the world

    gameConfigured = true;
  }
}

void playGame()
{
  attractMode(); // Blink lights while waiting for user to press a button

  // Indicate the start of game play
  setLEDs(CHOICE_RED | CHOICE_GREEN | CHOICE_BLUE | CHOICE_YELLOW); // Turn all LEDs on
  delay(1000);
  setLEDs(CHOICE_OFF); // Turn off LEDs
  delay(250);

  if (gameMode == MODE_MEMORY)
  {
    // Play memory game and handle result
    if (play_memory() == true)
      play_winner(); // Player won, play winner tones
    else
      play_loser(); // Player lost, play loser tones
  }

  if (gameMode == MODE_BATTLE)
  {
    play_battle(); // Play game until someone loses

    play_loser(); // Player lost, play loser tones
  }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//The following functions are related to game play only

// Play the regular memory game
// Returns 0 if player loses, or 1 if player wins
boolean play_memory(void)
{
  randomSeed(millis()); // Seed the random generator with random amount of millis()

  gameRound = 0; // Reset the game to the beginning

  while (gameRound < ROUNDS_TO_WIN)
  {
    int lenModifier = gameRound < 10 ? 1 : 18;
    int x = 55 - (lenModifier);

    clearScreen();
    display.setTextSize(1);
    display.setCursor(64 - strlen(score) * 3, 5);
    display.println(score);
    display.drawFastHLine(0, 14, 128, WHITE);

    display.setTextSize(4);
    display.setCursor(x, 25);
    display.println(gameRound);
    display.display();

    add_to_moves(); // Add a button to the current moves, then play them back

    playMoves(); // Play back the current game board

    // Then require the player to repeat the sequence.
    for (byte currentMove = 0; currentMove < gameRound; currentMove++)
    {
      byte choice = wait_for_button(); // See what button the user presses

      if (choice == 0)
        return false; // If wait timed out, player loses

      if (choice != gameBoard[currentMove])
        return false; // If the choice is incorect, player loses
    }

    delay(1000); // Player was correct, delay before playing moves
  }

  return true; // Player made it through all the rounds to win!
}

// Play the special 2 player battle mode
// A player begins by pressing a button then handing it to the other player
// That player repeats the button and adds one, then passes back.
// This function returns when someone loses
boolean play_battle(void)
{
  gameRound = 0; // Reset the game frame back to one frame

  while (1) // Loop until someone fails
  {
    byte newButton = wait_for_button(); // Wait for user to input next move
    gameBoard[gameRound++] = newButton; // Add this new button to the game array

    // Then require the player to repeat the sequence.
    for (byte currentMove = 0; currentMove < gameRound; currentMove++)
    {
      byte choice = wait_for_button();

      if (choice == 0)
        return false; // If wait timed out, player loses.

      if (choice != gameBoard[currentMove])
        return false; // If the choice is incorect, player loses.
    }

    delay(100); // Give the user an extra 100ms to hand the game to the other player
  }

  return true; // We should never get here
}

// Plays the current contents of the game moves
void playMoves(void)
{
  for (byte currentMove = 0; currentMove < gameRound; currentMove++)
  {
    toner(gameBoard[currentMove], 150);

    // Wait some amount of time between button playback
    // Shorten this to make game harder
    delay(150); // 150 works well. 75 gets fast.
  }
}

// Adds a new random button to the game sequence, by sampling the timer
void add_to_moves(void)
{
  byte newButton = random(0, 4); //min (included), max (exluded)

  // We have to convert this number, 0 to 3, to CHOICEs
  if (newButton == 0)
    newButton = CHOICE_RED;
  else if (newButton == 1)
    newButton = CHOICE_GREEN;
  else if (newButton == 2)
    newButton = CHOICE_BLUE;
  else if (newButton == 3)
    newButton = CHOICE_YELLOW;

  gameBoard[gameRound++] = newButton; // Add this new button to the game array
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//The following functions control the hardware

// Wait for a button to be pressed.
// Returns one of LED colors (LED_RED, etc.) if successful, 0 if timed out
byte wait_for_button(void)
{
  long startTime = millis(); // Remember the time we started the this loop

  while ((millis() - startTime) < ENTRY_TIME_LIMIT) // Loop until too much time has passed
  {
    byte button = checkButton();

    if (button != CHOICE_NONE)
    {
      toner(button, 150); // Play the button the user just pressed

      while (checkButton() != CHOICE_NONE)
        ; // Now let's wait for user to release button

      delay(10); // This helps with debouncing and accidental double taps

      return button;
    }
  }

  return CHOICE_NONE; // If we get here, we've timed out!
}

// Play the winner sound and lights
void play_winner(void)
{
  playStartup(BUZZER_PIN, false);

  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
}

// Play the loser sound/lights
void play_loser(void)
{
  playGameOver(BUZZER_PIN, false);

  setLEDs(CHOICE_RED | CHOICE_GREEN);
  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
  setLEDs(CHOICE_RED | CHOICE_GREEN);
  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
}

// Show an "attract mode" display while waiting for user to press button.
void attractMode(void)
{
  while (1)
  {
    setLEDs(CHOICE_RED);
    delay(100);
    if (checkButton() != CHOICE_NONE)
      return;

    setLEDs(CHOICE_BLUE);
    delay(100);
    if (checkButton() != CHOICE_NONE)
      return;

    setLEDs(CHOICE_GREEN);
    delay(100);
    if (checkButton() != CHOICE_NONE)
      return;

    setLEDs(CHOICE_YELLOW);
    delay(100);
    if (checkButton() != CHOICE_NONE)
      return;
  }
}