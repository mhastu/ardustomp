# Ardustomp
## Prerequisites
### Library
Install [MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) through the Arduino Library Manager (tested with 4.3.1).

### Arduino USB controller firmware
If you are using the Arduino Uno, [flash](https://docs.arduino.cc/hacking/software/DFUProgramming8U2) the `USBMidiKliK_dual_uno.hex.build` from [USBMidiKliK](https://github.com/TheKikGen/USBMidiKliK).

## Installation
Flash the .ino file using the Arduino IDE.

## Usage
Connect the Arduino via USB cable.
It will present itself as a MIDI device.
When connecting the pins 2-13 to GND, a "note on" signal is sent, and a "note off" signal at disconnect.
The analog pins 0-3 report analog values via MIDI CC.

When holding down buttons during power-up those buttons will change their behavior
to a latching note-on/off.

## Reflashing via Arduino IDE
The Board will, by default, not be programmable via the Arduino IDE, because it is now a MIDI device.
To active programming mode, however, the smart USBMidiKliK firmware can set the board into a programmable state again, when receiving following MIDI sysex:
```
F0 77 77 77 09 F7
```

This can be achieved using `amidi` (installable under Ubuntu via apt):
```
amidi -p hw:0,0 -S 'F0 77 77 77 09 F7'
```
where you have to replace `hw:0,0` with your device identifier (find out using `amidi -l`).

A simple reboot can be trigger by sending:
```
F0 77 77 77 0A F7
```

There are more sysex commands possible, check out the USBMidiKliK repo linked above.
