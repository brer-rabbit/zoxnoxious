#pragma once
#include "plugin.hpp"
#include "AudioMidi.hpp"


namespace zox {

struct HardwareDiscoveryConfig {
  // JSON Save/Restore fields
  bool autoDetect = true;
  bool swapAudioPorts = false;
  float sampleRate = 8000.f;
  int blockSize = 32;

  // Runtime/default behavior
  bool bindMidi = true;
  bool bindAudio = true;
  bool allowAudioHeuristicOrder = true;
};


struct HardwareDiscoveryResult {
  bool midiInputBound = false;
  bool midiOutputBound = false;
  bool audio0Bound = false;
  bool audio1Bound = false;

  std::string message;

  bool fullyBound() const {
    return midiInputBound && midiOutputBound && audio0Bound && audio1Bound;
  }

  bool audioBound() const {
    return audio0Bound && audio1Bound;
  }
};

HardwareDiscoveryResult discoverHardware(midi::InputQueue& midiInput,
                                         ZoxnoxiousMidiOutput& midiOutput,
                                         std::vector<ZoxnoxiousAudioPort*>& audioPorts,
                                         const HardwareDiscoveryConfig& config);

}
