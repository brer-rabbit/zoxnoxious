#include "HardwareDiscovery.hpp"
#include "constants.hpp"

namespace zox {

HardwareDiscoveryStatus HardwareDiscovery::discover(midi::InputQueue& midiInput,
                                                    ZoxnoxiousMidiOutput& midiOutput,
                                                    ZoxnoxiousAudioPort& audioPort) {
  HardwareDiscoveryStatus result = HardwareDiscoveryStatus::Success;

  bool midiInputFound = discoverMidiInput(midiInput);
  bool midiOutputFound = discoverMidiOutput(midiOutput);
  bool audioFound = discoverAudio(audioPort);

  if (!midiInputFound) {
    return HardwareDiscoveryStatus::MidiInputMissing;
  }
  if (!midiOutputFound) {
    return HardwareDiscoveryStatus::MidiOutputMissing;
  }
  if (!audioFound) {
    return HardwareDiscoveryStatus::AudioMissing;
  }

  return result;
}


static bool isZoxnoxiousMidiDevice(const std::string& deviceName) {
  // Linux:
  // MIDI input: driver=2 device=1 name=Zoxnoxious MIDI and Audio:Zoxnoxious MIDI and Audio MIDI  24:0
  // MIDI output: driver=2 device=1 name=Zoxnoxious MIDI and Audio:Zoxnoxious MIDI and Audio MIDI  24:0
  //
  // macOS:
  // MIDI input: driver=1 device=0 name=Zoxnoxious MIDI and Audio
  // MIDI output: driver=1 device=0 name=Zoxnoxious MIDI and Audio
  const std::string zoxMidiMatch = "Zoxnoxious";

  return deviceName.find(zoxMidiMatch) != std::string::npos;
}

static bool isZoxnoxiousAudioDevice(const std::string& driverName,
                                    const std::string& deviceName,
                                    int inputs, int outputs) {
  // Linux:
  // Audio: driver=1 (ALSA) device=137 name=Zoxnoxious MIDI and Audio (USB Audio) inputs=96 outputs=96
  //
  // macOS:
  // note macOS appears to split the devices as an input device and an output device:
  // Audio: driver=5 (Core Audio) device=129 name=Playback Inactive inputs=0 outputs=96
  // Audio: driver=5 (Core Audio) device=130 name=Capture Inactive inputs=96 outputs=0
  // we want the "Playback Inactive" device that has output channels.

#ifdef ARCH_MAC
  const std::string zoxAudioMatch = "Playback Inactive";
  const std::string driver = "CoreAudio";

  return driverName == driver &&
    deviceName.find(zoxAudioMatch) != std::string::npos &&
    inputs == 0 && outputs >= maxAudioChannels;
#elif defined(ARCH_LIN)
  const std::string zoxAudioMatch = "Zoxnoxious";
  const std::string driver = "ALSA";

  return driverName == driver &&
    deviceName.find(zoxAudioMatch) != std::string::npos;
#else
  return false;
#endif

}


bool HardwareDiscovery::discoverMidiInput(midi::InputQueue& midiIn) {
  for (int driverId : midi::getDriverIds()) {
    midi::Driver* driver = midi::getDriver(driverId);

    if (!driver) {
      continue;
    }

    for (int deviceId : driver->getInputDeviceIds()) {
      std::string name = driver->getInputDeviceName(deviceId);
      //DEBUG("MIDI input: driver=%d device=%d name=%s", driverId, deviceId, name.c_str());
      if (!isZoxnoxiousMidiDevice(name)) {
        continue;
      }

      midiIn.setDriverId(driverId);
      midiIn.setDeviceId(deviceId);
      return true;
    }
  }

  return false;
}



bool HardwareDiscovery::discoverMidiOutput(ZoxnoxiousMidiOutput& midiOut) {
  for (int driverId : midi::getDriverIds()) {
    midi::Driver* driver = midi::getDriver(driverId);
    if (!driver)
      continue;

    for (int deviceId : driver->getOutputDeviceIds()) {
      std::string name = driver->getOutputDeviceName(deviceId);
      //DEBUG("MIDI output: driver=%d device=%d name=%s", driverId, deviceId, name.c_str());
      if (!isZoxnoxiousMidiDevice(name)) {
        continue;
      }

      midiOut.setDriverId(driverId);
      midiOut.setDeviceId(deviceId);
      return true;
    }
  }

  return false;
}


bool HardwareDiscovery::discoverAudio(ZoxnoxiousAudioPort& audioPort) {
  for (int driverId : audio::getDriverIds()) {
    audio::Driver* driver = audio::getDriver(driverId);

    if (!driver) {
      continue;
    }

    for (int deviceId : driver->getDeviceIds()) {
      std::string name = driver->getDeviceName(deviceId);
      int inputs = driver->getDeviceNumInputs(deviceId);
      int outputs = driver->getDeviceNumOutputs(deviceId);

      DEBUG("Audio: driver=%d (%s) device=%d name=%s inputs=%d outputs=%d",
            driverId, driver->getName().c_str(),
            deviceId, name.c_str(),
            inputs, outputs);

      if (!isZoxnoxiousAudioDevice(driver->getName(), name, inputs, outputs)) {
        continue;
      }

      audioPort.setDriverId(driverId);
      audioPort.setDeviceId(deviceId);

      return true;
    }
  }

  return false;
}



} // namespace zox
