#include "macros.h"
#include "Particle.h"
#include "leds/leds.h"
#include "display/display.h"

unsigned long last_micros = 0;
long debouncing_time = 250;
extern bool menuShowing;
extern byte appmode;
extern byte btncounter;
extern byte btnid;

void UP()
{
  if (menuShowing && (long)(micros() - last_micros) >= debouncing_time * 1000)
  {
    if (appmode)
    {
    }
    else
    {
      btncounter++;
    }
    btnid = 3;
    last_micros = micros();
  }
}

void DOWN()
{
  if (menuShowing && (long)(micros() - last_micros) >= debouncing_time * 1000)
  {
    if (appmode)
    {
    }
    else
    {
      btncounter++;
    }
    btnid = 2;
    last_micros = micros();
  }
}

void ENTER()
{
  if (menuShowing && (long)(micros() - last_micros) >= debouncing_time * 1000)
  {
    if (appmode)
    {
    }
    else
    {
      btncounter++;
    }
    btnid = 1;
    last_micros = micros();
  }
}

void LEFT()
{
  if (menuShowing && (long)(micros() - last_micros) >= debouncing_time * 1000)
  {
    if (appmode)
    {
    }
    else
    {
      btncounter++;
    }
    resetDisplayBools();
    btnid = 4;
    last_micros = micros();
  }
}

void RIGHT()
{
  if (menuShowing && (long)(micros() - last_micros) >= debouncing_time * 1000)
  {
    if (appmode)
    {
    }
    else
    {
      btncounter++;
    }
    resetDisplayBools();
    btnid = 5;
    last_micros = micros();
  }
}

void setupJoystickInterrupts()
{
  // Set up Interrupts
  attachInterrupt(JOYSTICK_UP, UP, FALLING);
  attachInterrupt(JOYSTICK_DOWN, DOWN, FALLING);
  attachInterrupt(JOYSTICK_CENTER, ENTER, FALLING);
  attachInterrupt(JOYSTICK_LEFT, LEFT, FALLING);
  attachInterrupt(JOYSTICK_RIGHT, RIGHT, FALLING);
}

void ESC()
{
  if ((long)(micros() - last_micros) >= debouncing_time * 1000)
  {
    detachInterrupt(BLUE_BUTTON_B);
    detachInterrupt(YELLOW_BUTTON_D);

    appmode = 0;
    btnid = 4;
    btncounter++;

    resetDisplayBools();
    toggleAllButtons(LOW);
  }
}

void BACK()
{
  if ((long)(micros() - last_micros) >= debouncing_time * 1000)
  {
    appmode = 0;
    btnid = 4;
    btncounter++;
    resetDisplayBools();
    toggleAllButtons(LOW);

    detachInterrupt(JOYSTICK_LEFT);
    attachInterrupt(JOYSTICK_LEFT, LEFT, FALLING); // Set the default interrupt back up
  }
}

void setupLEDButtonInterrupt()
{
  attachInterrupt(BLUE_BUTTON_B, ESC, FALLING);
  attachInterrupt(YELLOW_BUTTON_D, ESC, FALLING);
}

void setupBackButtonInterrupt()
{
  detachInterrupt(JOYSTICK_LEFT);
  attachInterrupt(JOYSTICK_LEFT, BACK, FALLING);
}
