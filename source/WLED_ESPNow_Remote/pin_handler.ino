/**********************************************************************
  ESPNow remote for WLED
  Copyright (c) 2024  Damian Schneider (@DedeHai)
  Licensed under the EUPL v. 1.2 or later
  https://github.com/DedeHai/WLED-ESPNow-Remote
***********************************************************************/

#define BUTTON_MINPULSE 3  // minimum time in ms between valid button state changes

// function is called when detecting rising/falling edge on button pin and periodically while button is pressed to detect long press
void IRAM_ATTR handleButtons() {
  uint8_t buttonfunction = 0;  // button function, is added to send queue if > 0
  readButtons(); // read button GPIOs, saved to currentButtonState[]
  uint32_t now = esp_timer_get_time();

  for (int i = 0; i < NUM_BUTTONS; i++) {
    uint32_t actionduration = (now - buttons[i].lastchange) / 1000;  // time in milliseconds since last state change  note: esp_timer_get_time() works with light sleep but not with deepsleep
    if (actionduration < BUTTON_MINPULSE) continue;                  // deglitch: reject short pulses
    bool buttonReleased = currentButtonState[i];                     // low is pressed, high is released

    if (buttons[i].buttonPressed)  // button was previously pressed, check for long pressed or single press if released
    {      
      if (buttonReleased == true) {
        buttons[i].buttonPressed = false;
        buttons[i].lastchange = now;
        lastButtonEdgeTime = now;
      } else if (actionduration >= LONG_PRESS_TIME) {
        buttons[i].longPress = true;
        buttons[i].buttonClicks = 0;  // reset single click
        buttons[i].lastchange = now;  // reset time (long press will repeat)
        lastButtonEdgeTime = now;
      }
    } else if (buttonReleased == false) {  // button was not pressed -> new press detected
      buttons[i].buttonPressed = true;
      buttons[i].buttonClicks++;
      buttons[i].lastchange = now;
      lastButtonEdgeTime = now;
      continue;  // wait for button to be released to determine the function to send
    } else       // button is still released
      buttons[i].buttonPressed = false;

    // now handle button action
    if (buttons[i].longPress) {
      buttons[i].longPress = false;  //reset (repeats if still pressed after another period)
      buttonfunction = buttons[i].longPressFunction;
    }

    if (buttons[i].buttonClicks > 0 && buttonReleased && actionduration >= DOUBLE_CLICK_TIME) {  // process single click after time window for double cklick has passed
      buttons[i].buttonClicks = 0;
      buttonfunction = buttons[i].singleClickFunction;
    }

    if (buttons[i].buttonClicks > 1) {  // second click detected, process it immediately (do not wait for button release), a long press still happens if button is not released
      buttons[i].buttonClicks = 0;
#ifdef ROTARY_ENCODER
      if (i == 0)                                            // rotary button has index 0
        rotaryLevel = (rotaryLevel + 1) % NUM_ROTARYLEVELS;  // go to next level on double click
#endif
      buttonfunction = buttons[i].doubleClickFunction;
    }

    if (buttonfunction > 0)
      buffer_push(buttonfunction);
    buttonfunction = 0; // reset for next pin
  }
}

#ifdef ROTARY_ENCODER
static unsigned deglitchtime;      // timestamp for non-blocking deglitching of clk-pin: glitches are rare and short, usually less than 100us
static unsigned dtState;           // for interrupt based rotary readout
#define ROTARY_DEBOUNCE_DELAY 500  // read new CLK pin state no sooner to avoid switching-glitches: time in microseconds

// handler for encoder rotation, skipDTread is used when DT pin is read in interrupt (interrupts are only used during sendout)
// note on  rotary level: level 0 sends brightness up/down button commands (as used by WiZmote), level 1 sends WIZMOTE_BUTTON_FOUR (counter clockwise) and WIZMOTE_BUTTON_FOUR+1 (clockwise). Higher levels add +2 (or +3 for clockwise) for each level.
void IRAM_ATTR handleRotaryEncoder(bool skipDTread) {
  if (!skipDTread) {
    dtState = digitalRead(DT_PIN);        // read current dt pin state to determine rotation direction
    deglitchtime = esp_timer_get_time();  // save time of detected edge
  }
  while (esp_timer_get_time() - deglitchtime < ROTARY_DEBOUNCE_DELAY)
    ;  // discard switching noise before setting ntew CLK_PIN state
  uint8_t clkState = digitalRead(CLK_PIN);
  uint8_t rotaryaction = 0;  // action to add to queue

  if (clkState != lastClkState) {  // Detect change on CLK_PIN
    if (clkState == dtState) {
      if (rotaryLevel == 0)
        rotaryaction = WIZMOTE_BUTTON_BRIGHT_DOWN;  // Counterclockwise
      else
        rotaryaction = WIZMOTE_BUTTON_ONE + (rotaryLevel - 1) * 2;

      encodervalue--;
    } else {  // Clockwise
      if (rotaryLevel == 0)
        rotaryaction = WIZMOTE_BUTTON_BRIGHT_UP;
      else
        rotaryaction = 1 + WIZMOTE_BUTTON_ONE + (rotaryLevel - 1) * 2;
      encodervalue++;
    }

    lastClkState = clkState;
    buffer_push(rotaryaction);
    //buffer_push(encodervalue); // debug
  }
}

// CLK pin interrupt handler (interrupt used for rotary encoder only, buttons are polled)
void IRAM_ATTR clk_pin_isr() {
  dtState = digitalRead(DT_PIN);        // read current dt pin state to determine rotation direction
  deglitchtime = esp_timer_get_time();  // save time of detected edge
}
#endif