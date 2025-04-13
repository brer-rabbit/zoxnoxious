# Zoxnoxious Analog Synthesizer #

"Beyond Obnoxious"

Zoxnoxious is a hardware analog music synthesizer with a software interface.  The intent is to provide a platform for card/module development ("card" may be a better term than "module", as the latter is a well established term in the modular synth world).  Cards interface via a Raspberry Pi to a host controller over USB, and the host controller runs VCV Rack which provides control signals.


![Zonoxious Block Diagram](zoxnoxious.png)

# Z-Cards #

A Z-Card is a circuit board that plugs into the Zoxnoxious backplane.  Each card receives digital signals such as I2C and SPI, and the card drives a defined set of pins with analog signals.  The Z-Cards are agnostic to the microcontroller; the interface being I2C and SPI most any microcontroller can do that.  While I'm using a Raspberry Pi in this project, these cards are ripe for anyone developing an analog synth with a microcontroller.  Cards developed to date include:

* AS3340 VCO
* AS3372 Signal Processor (VCF, VCA)
* SSI2130/AS3394 Dual VCO + VCF Synth Voice
* Pole Dancer multimode filter with morphing/crossfade

The backplane design allows for up to 8 cards to be controlled.  The backplane requires addressing so that counts as one of the eight.  The backplane board is designed with 6 slots, so one of the control addresses goes unused.

# Raspberry Pi Host #

A Raspberry Pi Zero 2 attaches to the Zoxnoxious backplane.  To a host computer, the Pi appears as USB Audio and USB MIDI devices.  Incoming USB Audio and MIDI are dispatched to a card's driver, which may choose to send SPI or I2C messages to the card.


# Zoxnoxious plugins for VCV Rack #

The user interacts with the synth through a VCV Rack plugin.  Zoxnoxious VCV Rack plugins map the Rack modules to audio channels and MIDI events that are sent via USB to the synthesizer.

# Deep Dive Videos

Deep dive 1: Project background and user interface
[![YouTube Zoxnoxious Deep Dive Video](https://img.youtube.com/vi/LsGcW3EjFYo/sddefault.jpg)](https://www.youtube.com/watch?v=LsGcW3EjFYo)

Deep dive 2: Raspberry Pi and Gadget mode
[![YouTube Zoxnoxious Deep Dive Video](https://img.youtube.com/vi/pGoO3mSk7ao/sddefault.jpg)](https://www.youtube.com/watch?v=pGoO3mSk7ao)

Deep dive 3: Hardware detail and autotune
[![YouTube Zoxnoxious Deep Dive Video](https://img.youtube.com/vi/C-MREijqNOM/sddefault.jpg)](https://www.youtube.com/watch?v=C-MREijqNOM)
