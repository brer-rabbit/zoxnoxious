# Zoxnoxious Analog Synthesizer #

"Beyond Obnoxious"

Zoxnoxious is a the beginings of a hardware analog music synthesizer with a software interface.  The intent is to provide a platform for "card" development ("card" may be a better term than "module", as modular is a well established term).  Cards interface to a host controller, and the host controller responds to an external controller.  The host controller for this project is a Raspberry Pi, and the external controller is VCV Rack.


![Zonoxious Block Diagram](zoxnoxious.png)

# Cards #

A card is a circuit board that plugs into a backplane/motherboard (TBD).  The first card developed is based around a CEM 3340 clone, the AS 3340 oscillator.  Each card receives digital signal such as I2C and SPI, and drives a defined set of pins with analog signals.

# Raspberry Pi Host #

A Raspberry Pi is setup with USB Audio and USB MIDI interface.  Incoming USB Audio and MIDI is sent to the Card's driver/plugin.  The card's driver is intended to convert these to SPI or I2C to drive the card's DAC and other card components such as switches.

# Zoxnoxious plugins for VCV Rack #

The user interacts with the synth through a VCV Rack plugin.  The Raspberry Pi is plugged in via USB to a host computer (Linux, Mac, Windows).  The Zoxnoxious plugin for VCV Rack map audio channels and generate MIDI events for the card.
