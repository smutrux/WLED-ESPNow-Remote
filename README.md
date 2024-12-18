# WLED-ESPNow-Remote
ESPNow based remote controller for WLED using an ESP32-C3 with support for buttons, button matrix and rotary encoder. The code is Arduino based for easy compilation: developed and tested with Arduino IDE version 2.3.4

Hardware setup and button settings can be easily configured in the source code.

## Features
- Support for up to 6 standard button inputs (more when not using deep-sleep)
- Support for up to 36 buttons when using a 6x6 button matrix
- Support for rotary encoder input with configurable number of levels (see below)
- Each button can be assigned up to 3 functions: single click, double click and long press with automatic repeat
- Makes use of the low-power capabilities for battery powered remote controllers

## Supported hardware
- Controller: ESP32 C3
- Buttons: any button or sensor that has an active-low signal i.e. pulls to GND, external pullups are optional
- Button Matrix: any button matrix with row and column outputs with up to 6 rows and 6 columns, no external resistors or diodes required
- Rotary encoder: standard rotary encoders are supported, 1M external pullups are recommended for battery powered remotes

## Hardware setup
It is recommended to use the suggested pins given in the code. GPIO9 (boot pin) can generally not be used. For normal buttons, use GPIO0 through GPIO5. Any higher numbered GPIOs do not have wake-up capabilities and are only responsive when not in deep-sleep.

You can use the `ESP32_ESPNow_receiver.ino` in the `tools` folder running on a second ESP32 (should work on any ESP32 flavour) and open the serial monitor to check for received button codes. As an alternative: using a WLED device with debug output enabled will also display received messages on the serial output. 

### Standard Buttons
Connect each button between the GPIO and GND. External pull-up resistors are not needed but can be used, it makes no difference to functionality.

If more than 6 buttons are needed there are several options:
- use a matrix button configuration
- disable deep-sleep (`#DISABLE_DEEPSLEEP` in `config.h`) and any GPIO can be used (even GPIO9 although not recommended) at the expense of about **8 fold increased power consumption** (0.35mA vs. 0.04mA)
- use a button connected to GPIO0-GPIO5 to wake from deep sleep: during `ACTIVE_TIME` (see `config.h`) all configured GPIOs are responsive. Adjust `ACTIVE_TIME`  to your requriements

### Button Matrix
Connect the column-pins to GPIO0-GPIO5 as they are used as the inputs and need to be able to awaken the controller from deep-sleep. Connect the row-pins to any GPIO **except GPIO9**. Avoid pins with LEDs connected to them (GPIO8 on the C3 supermini for example) as it leads to (much) increased standby power consumption. 

The code is already set-up for a 4x4 button matrix, use the pins recommended in the code if possible. 

The button numbering is done from left to right, top to bottom with column 0 being on the left and row 0 being the top row. 

### Rotary Encoder
The CLK and DT pins of the encoder should be connected to GPIO0 and GPIO1. GPIO3, 4 and 5 can also be used, avoid any GPIOs with on-board pullups (i.e. GPIO2). 

For the button pin the use of GPIO2 is recommended although any GPIO from GPIO0 to GPIO5 can be used.

When using a rotary encoder module, connect GND to GND of the ESP and + to 3.3V of the ESP.

Additional buttons can be connected if required, the same restrictions mentioned above apply.

#### Tips for longer battery life using a rotary encoder
Standard rotary encoders have two resting postions: both (DT and CLK) pins floating and both pins connected to GND. To detect signals in deep-sleep an external pullup resistor is required. 
When using rotary encoder modules, these resistors are already in place, however, they consume a lot of power: when the two pins are in GND state, the pullup resistor of usually 10k resulting in deep-sleep curren of 700uA vs. ~45uA (see below). 

One way to get around this is to replace the resistors with higher value ones: 1M resistors work well, values larger than 3M are too weak to pull the pins to a reliable voltage. On the DT pin, an external resistor is not required and can be omitted.
Using 1M pull-up resistor only adds ~3uA per resistor to the deep-sleep current.

If you have no such resistors available, removing/omitting any pull-up resistors and enabling the `ENABLE_DEEPSLEEP_PULL` option is also possible, the internal pull-up resistors have a value of ~50k. Since only the pull-up on the CLK pin is enabled in deep-sleep,
the resulting current will be about 100uA. 

## Configuring and uploading the code
The default configuration uses 6 standard button inputs on GPIO0 - GPIO5. You can use this default configuration even when using less than 6 buttons: unused button pins will simple be idle and have no influence on power consumption. 

To upload the code, use the Arduino IDE (version 2.3.4 tested, newer version may or may not work)

Since the program puts the ESP in sleep mode, upload only works in bootloader mode: disconnect from USB, press and hold the boot button on the ESP32 C3, connect to USB then release the button and upload the code. Press reset after uploading to exit the bootloader.

By default, the on-board LED (GPIO8) will blink when data is sent out. If you see no blinking when pressing a button, there is something wrong with your hardware setup or the code was not uploaded properly. As mentioned above you can use the provided receiver sketch to for testing. 

### Changing the configuration
To change the hardware configuration, adjust the code in `config.h` according to your setup: comments in that file hopefully make it clear what each config parameter is used for.

The function numbers assigned to each button can be changed in the buttons.ino file where I added some examples. WLED supports the standard WiZ-mote codes by default, however those are highly limited in use. 
You can add a `remote.json` file to WLED using a web-browser (wled-ip/edit) to assign any supported JSON-API command to any button function. 

Triggering a preset using the command `{"ps":5}` to trigger preset 5 for example and adding the JSON-commands in the WLED web UI allows for easier adjustments than editing the remote.json file as that requires an active internet connection for the editor.

Example remote.json files are provided in the remote-json folder.

For details about the JSON-API please see [the documentation](https://kno.wled.ge)

## WLED setup
On your WLED controller you need to enable the ESPNow support and pair the remote in config->WiFi Setup, see https://kno.wled.ge/interfaces/infrared/#esp-now-based-remote-control

## How it all works
ESPNow is a wifi frequency based protocol by Espressif that works without an active wifi connection. This allows for low power communication as devices can quickly send messages without having to go 
through the wifi authentification process. The ESPNow remote code broadcasts a small message across all wifi channels (1-13) so it can be picked up by any WLED controller, no matter on what
channel it is currently on. This also works if the WLED controller is not connected to any wifi i.e. in AP mode. 

Since the messages are "blindly" broadcast there is no acknowledgement sent by the receiving controller. The remote therefore does not know if a message was received, all it knows if the message was sent: if there is heavy traffic
on the wifi channel used it can lead to messages being lost: the remote detects message collisions with normal wifi traffic and will try to send the message again but there is no guarantee that it will go through.
This can lead to messages sometimes being lost. If there is heavy traffic in your network and a lot of messages get lost, try increasing the `REPEATSEND` parameter or change the channel.

### Rotary encoder 
Standard rotary encoders allow to detect the rotation direction and trigger a message send out for each detent. To be able to control more settings, the "level" can be changed with a double-click on the encoder button.
The codes sent out for rotation are (counter clockwise / clockwise):

- level 0: WIZMOTE_BUTTON_BRIGHT_DOWN = 8 / WIZMOTE_BUTTON_BRIGHT_UP = 9
- level 1: WIZMOTE_BUTTON_ONE = 16 	  / WIZMOTE_BUTTON_ONE+1 = 17
- level 2: WIZMOTE_BUTTON_ONE+2 = 18 	  / WIZMOTE_BUTTON_ONE+3 = 19

(higher levels continue the same way)

- WIZMOTE_BUTTON_ONE+1 is also WIZMOTE_BUTTON_TWO
- WIZMOTE_BUTTON_ONE+2 is also WIZMOTE_BUTTON_THREE
- WIZMOTE_BUTTON_ONE+3 is also WIZMOTE_BUTTON_FOUR

these have default actions defined in WLED if no remote.json file is found

after some idle time (KEEPROTARYLEVEL_TIME) the level will be reset to 0 so that if you pick-up the remote after some time, it is set to the default of controlling brightness.

## Battery operation
The remote controller code was written and optimized for battery operated devices using a ESP32 C3 supermini. These controllers work down to ~3.6V input voltage on the 5V pin and are therefore perfectly suited for lithium polymer batteries. 
When the power LED is removed (see below) these controllers use only ~45uA (micro amperes or 0.045mA) when put into deep-sleep allowing for a very long battery life: a small 500mAh battery will last over a year without recharge.

Only use batteries with built-in protection circuits or use an external battery protection module for safety. It is also recommended to connect the battery through a charger module so it can be easily recharged.

Connect the battery power to the 5V and GND pin of the ESP board, using an external 3.3V regulator and using the 3.3V pin is not recommended as it increases power consumption unless the LDO on the ESP is removed. 

A key-press and the following sendout use about the same amount of enery as the remote uses in 2 minutes of deep-sleep, assuming the modifications mentioned i.e. removing the power LED. 
So unless a button is pressed more than 700 times per day, the deep-sleep power consumption is the dominating factor: hence the importance of low deep-sleep current. 

<img src="/images/rotary_setup_example.jpg" width="100%">

### Controller modifications for low power
The power-LED on the controller uses "a lot" of current, around 1mA and it is the dominating factor for battery life. It has to be removed to achieve the proclaimed ~45uA. The best (but not easiest) way to do this is to desolder it. 
It can also be cut/destroyed using pointy side-cutters or a knife, however there is the risk of damaging other components when not being careful. The small black resistor just next to the LED can also be removed as it is only used for the LED.

For more reliable operation add a 4.7uF or larger ceramic capacitor between the 3.3V pin and GND: I use 0805 sized 10uF/25V capacitors soldered directly to the C3 supermini pads. The capacitor helps to store energy required for the send-bursts.

<img src="/images/C3supermini.jpg" width="100%">

### Note on ESP32 C3 Supermini variants
There are several variations of the C3 supermini, at least one of them is know to cause issues with the wifi-reception due to bad hardware design. The badly designe ones are easy to spot if you know what exactly to look for.
Here is a side-by-side comparison of the good and the bad.

<img src="/images/C3_good_bad.jpg" width="100%">

I only own one of the bad ones and mine still works ok, the signal strength is reduced by about 50%. 

## Examples for button modules
During development and testing I tried several different button-module designs that are commonly available in internet shops. Here are some fotos of tested and working configurations and which GPIOs are best to be used.

<img src="/images/tactile_buttons.jpg" width="100%">

<img src="/images/membrane_buttonpads.jpg" width="100%">

<img src="/images/rotary_encoder.jpg" width="100%">

## License and Credits

Licensed under the EUPL v1.2 license  
Code written and maintained by @DedeHai

*Disclaimer:*   

As per the EUPL license, I assume no liability for any damage to you or any other person or equipment.  
