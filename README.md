# Mighty Mute

**A tiny USB HID device to send a "Mute Microphone" command to a PC**

## What?

A USB device which conforms to the USB HID spec such that it is driverless. It enumerates as a USB Input device, and when a button is pressed, it sends the `PHONE MUTE` command (`0x2F`) which lives in the Telephony (`0x05`) usage page. It actually sends two commands, one for the key being pressed and then, after a short pause, one for the key being released. 

## Why?

The `PHONE MUTE` USB HID command is interpreted by most telephony programs as "mute the microphone". It's handy to have a physical desktop button to do this.

## Usage

This is a project for an ARM STM32F4 microcontroller (supporting USB OTG) which uses `libopencm3`. Some adaption is probably required for other STM32s.

* Clone (recursively) this repo.
* Adjust the GPIO and linker scripts for your specific target.
* Build using `arm-gcc-embedded`.
* Flash to your chosen microcontroller.

## License

Released under the MIT license. 
