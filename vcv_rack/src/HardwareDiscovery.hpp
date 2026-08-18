#pragma once
#include "plugin.hpp"
#include "AudioMidi.hpp"


namespace zox {

enum class HardwareDiscoveryStatus {
  Success,
  MidiInputMissing,
  MidiOutputMissing,
  AudioMissing
};

class HardwareDiscovery {
public:
  HardwareDiscoveryStatus discover(midi::InputQueue& midiIn,
                                   ZoxnoxiousMidiOutput& midiOut,
                                   ZoxnoxiousAudioPort& audioPort);


private:
  bool discoverMidiInput(midi::InputQueue& midiIn);
  bool discoverMidiOutput(ZoxnoxiousMidiOutput& midiOut);
  bool discoverAudio(ZoxnoxiousAudioPort& audioPort);

};


}
