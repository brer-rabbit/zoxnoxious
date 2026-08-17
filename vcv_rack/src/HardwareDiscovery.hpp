#pragma once
#include "plugin.hpp"
#include "AudioMidi.hpp"


namespace zox {

struct HardwareDiscoveryConfig {
  // JSON Save/Restore fields
  bool autoDetect = true;
  float sampleRate = 8000.f;
  int blockSize = 32;

  // Runtime/default behavior
  bool bindMidi = true;
  bool bindAudio = true;
};




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
                                   ZoxnoxiousAudioPort& audioPort,
                                   const HardwareDiscoveryConfig& config);


private:
  bool discoverMidiInput(midi::InputQueue& midiIn);
  bool discoverMidiOutput(ZoxnoxiousMidiOutput& midiOut);
  bool discoverAudio(ZoxnoxiousAudioPort& audioPort);

};


}
