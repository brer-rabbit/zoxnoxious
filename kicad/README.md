# Zoxnoxious KiCad Files

The directories below contain KiCad files for the Zoxnoxious synthesizer.  This includes files for both schematics, PCB layout, and assembly.  Some boards also have a datasheet.

# Voice cards

* [Z2203](z2203/): Yamaha FM-based voice card with YM2203 chip
* [Pole Dancer](poledancer/): pole mixing filter utilizing an SSI2140 and SSI2190
* [Z3340](z3340/): Oscillator based around an AS3340 with four sync modes that can be combined
* [Z3372](z3372/): Lowpass filter with AS3372 filter chip.  Stereo panning, filter and resonance modulation.
* [Z5524](z5524/): dual VCO voice card.  SSI2130 oscillator for TZFM, AS3194 for second VCO and filter.  Powerful.


# Backplane

The [backplane](vca_backplane/) hosts six voice cards and a Raspberry Pi Zero2.


# Frontpanel

[Frontpanel](frontpanel/) for project.


# Template for development

The [zoxnoxiouscard](zoxnoxiouscard/) directory has a template to run from for creating voice cards.  Includes DAC, GPIO expander, and layout for the same.
