#pragma once

namespace zox {
  
// maximum number of channels on a USB Audio interface
static constexpr int8_t maxAudioChannels = 27;
static constexpr int8_t maxAudioDevices = 2;
static constexpr int maxVoiceCards = 6;

static constexpr int8_t invalidSlot = -1;
static constexpr int8_t invalidOutputDeviceId = -1;
static constexpr int8_t invalidCvChannelOffset = -1;
static constexpr int8_t invalidMidiChannel = -1;
static constexpr uint8_t invalidCardId = 0;
static constexpr int invalidPrimaryDepth = -1;

static const std::string invalidCardOutputName = "----";
//static std::vector<std::string> cardOutputNames;

static constexpr uint8_t midiProgramChangeStatus = 0xC;

void setMidiProgramChangeMessage(rack::midi::Message& midiOutMessage, int8_t midiChannel, int8_t program);
}
