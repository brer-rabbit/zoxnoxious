#include "HardwareDiscovery.hpp"

namespace zox {

HardwareDiscoveryStatus HardwareDiscovery::discover(midi::InputQueue& midiInput,
                                                    ZoxnoxiousMidiOutput& midiOutput,
                                                    ZoxnoxiousAudioPort& audioPort,
                                                    const HardwareDiscoveryConfig& config) {
  HardwareDiscoveryStatus result = HardwareDiscoveryStatus::Success;

  if (!discoverMidiInput(midiInput)) {
    result = HardwareDiscoveryStatus::MidiInputMissing;
  }
  else if (!discoverMidiOutput(midiOutput)) {
    result = HardwareDiscoveryStatus::MidiOutputMissing;
  }
  else if (!discoverAudio(audioPort)) {
    result = HardwareDiscoveryStatus::AudioMissing;
  }

  return result;
}


static bool isZoxnoxiousMidiDevice(const std::string deviceName) {
// TODO: #ifdef platform string specifics
  const std::string zoxMidiMatch = "Zoxnoxious";

  if (deviceName.find(zoxMidiMatch)) {
    return true;
  }
  return false;
}

static bool isZoxnoxiousAudioDevice(const std::string driverName,
                                    const std::string deviceName,
                                    int inputs, int outputs) {
// TODO: #ifdef platform string specifics
  const std::string zoxMidiMatch = "Zoxnoxious";

  if (deviceName.find(zoxMidiMatch)) {
    return true;
  }
  return false;
}


bool HardwareDiscovery::discoverMidiInput(midi::InputQueue& midiIn) {
  for (int driverId : midi::getDriverIds()) {
    midi::Driver* driver = midi::getDriver(driverId);

    if (!driver) {
      continue;
    }

    for (int deviceId : driver->getInputDeviceIds()) {
      std::string name = driver->getInputDeviceName(deviceId);

      DEBUG("MIDI input: driver=%d device=%d name=%s",
            driverId, deviceId, name.c_str());

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

      DEBUG("MIDI output: driver=%d device=%d name=%s",
            driverId, deviceId, name.c_str());

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
