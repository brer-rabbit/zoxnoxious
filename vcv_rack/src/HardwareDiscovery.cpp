#include "HardwareDiscovery.hpp"

namespace zox {

HardwareDiscoveryResult discoverHardware(midi::InputQueue& midiInput,
                                         ZoxnoxiousMidiOutput& midiOutput,
                                         std::vector<ZoxnoxiousAudioPort*>& audioPorts,
                                         const HardwareDiscoveryConfig& config) {

  HardwareDiscoveryResult result;

  INFO("HardwareDiscovery");

  return result;
}


} // namespace zox
