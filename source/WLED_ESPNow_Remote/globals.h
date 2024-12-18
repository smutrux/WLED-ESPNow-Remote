/**********************************************************************
  ESPNow remote for WLED
  Copyright (c) 2024  Damian Schneider (@DedeHai)
  Licensed under the EUPL v. 1.2 or later
  https://github.com/DedeHai/WLED-ESPNow-Remote
***********************************************************************/

#define DOUBLE_CLICK_TIME 250     // max pause in ms between presses for double click detection
#define LONG_PRESS_TIME 600       // time in ms threshold for long press detection (also repeats at this rate)
#define MINSENDINTERVAL 40000     // minimum time in microseconds between sendouts to not overload the clients or sender
#define BUTTONPOLLINTERVAL 20000  // polling for single vs. double press should not be larger than MINSENDINTERVAL, 10ms-50ms are good values, faster polling means higher power consumption

// struct for buttons
typedef struct {
  bool buttonPressed = false;   // button state tracking
  bool longPress = false;       // long press tracking
  uint8_t buttonClicks = 0;     // single or double click tracking
  uint8_t singleClickFunction;  // button function number that is sent out when this button is pressed once
  uint8_t doubleClickFunction;  // button function number that is sent out when this button is double clicked
  uint8_t longPressFunction;    // button function number that is sent out when this button is held down long
  uint32_t lastchange = 0;      // local time of last button state change
} buttonprops;

//global variables: variables put in RTC memory persist during deep sleep
RTC_DATA_ATTR buttonprops buttons[NUM_BUTTONS];  // array with button properties

#ifdef ROTARY_ENCODER
RTC_DATA_ATTR volatile bool lastClkState = 0;
RTC_DATA_ATTR volatile int32_t encoderValue = 0;  // Encoder counter (used for debug)
RTC_DATA_ATTR volatile uint32_t rotaryLevel = 0;  // currently set rotary encoder level
#endif

// non RTC memory global variables are reset when entering deepsleep
bool currentButtonState[NUM_BUTTONS]; // array for temporary button state to detect rising/falling edges
unsigned lastButtonEdgeTime = 0;      // time of last detected button edge, used for timing in main loop. microsecond timer (overflows after 4294 seconds so no issue as it is reset in deep sleep)
uint8_t encodervalue = 0;             // debug only

// function prototypes
void sleepAndMonitor(unsigned micros, bool fulltime = false);
#ifdef ROTARY_ENCODER
void handleRotaryEncoder(bool skipDTread);
void clk_pin_isr();
#endif
void handleButtons();
bool anyButtonPressed();
bool buffer_push(uint8_t value);
bool buffer_read(uint8_t &value);
bool buffer_is_empty();

// ESP32 C3 onboard LED macros (keeps state in sleep modes)
#define LED_PIN 8   // LED_BUILTIN for C3 supermini
bool ledState = 0;  // used to toggle LED

// blink LED x times (blocking)
void blink(int times) {
  gpio_hold_dis((gpio_num_t)LED_PIN);
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(30);
    digitalWrite(LED_PIN, HIGH);
    delay(20);
  }
}

// enable onboard LED, hold during sleep
#define LED_ON \
  { \
    gpio_hold_dis((gpio_num_t)LED_PIN); \
    digitalWrite(LED_PIN, LOW); \
    gpio_hold_en((gpio_num_t)LED_PIN); \
  }

// disable onboard LED, hold during sleep
#define LED_OFF \
  { \
    gpio_hold_dis((gpio_num_t)LED_PIN); \
    digitalWrite(LED_PIN, HIGH); \
    gpio_hold_en((gpio_num_t)LED_PIN); \
  }

// toggle LED, hold during sleep
#define LED_TOGGLE \
  { \
    gpio_hold_dis((gpio_num_t)LED_PIN); \
    ledState = !ledState; \
    digitalWrite(LED_PIN, ledState); \
    gpio_hold_en((gpio_num_t)LED_PIN); \
  }
