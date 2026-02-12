#pragma once

#include <atomic>
#include "Participant.hpp"
#include "AudioMidi.hpp"


namespace zox {

struct DiscoveredCard;

struct AudioIO final : rack::engine::Module {
  enum ParamId {
    OUT1_LEVEL_KNOB_PARAM,
    OUT2_LEVEL_KNOB_PARAM,
    // the MIX1 and MIX2 enums need to be sequential
    CARD_A_MIX1_OUTPUT_BUTTON_PARAM,
    CARD_B_MIX1_OUTPUT_BUTTON_PARAM,
    CARD_C_MIX1_OUTPUT_BUTTON_PARAM,
    CARD_D_MIX1_OUTPUT_BUTTON_PARAM,
    CARD_E_MIX1_OUTPUT_BUTTON_PARAM,
    CARD_F_MIX1_OUTPUT_BUTTON_PARAM,
    CARD_A_MIX2_OUTPUT_BUTTON_PARAM,
    CARD_B_MIX2_OUTPUT_BUTTON_PARAM,
    CARD_C_MIX2_OUTPUT_BUTTON_PARAM,
    CARD_D_MIX2_OUTPUT_BUTTON_PARAM,
    CARD_E_MIX2_OUTPUT_BUTTON_PARAM,
    CARD_F_MIX2_OUTPUT_BUTTON_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    OUT1_LEVEL_INPUT,
    OUT2_LEVEL_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    OUTPUTS_LEN
  };
  enum LightId {
    OUT1_LEVEL_CLIP_LIGHT,
    OUT2_LEVEL_CLIP_LIGHT,
    // the MIX1 and MIX2 enums need to be sequential
    CARD_A_MIX1_OUTPUT_BUTTON_LIGHT,
    CARD_B_MIX1_OUTPUT_BUTTON_LIGHT,
    CARD_C_MIX1_OUTPUT_BUTTON_LIGHT,
    CARD_D_MIX1_OUTPUT_BUTTON_LIGHT,
    CARD_E_MIX1_OUTPUT_BUTTON_LIGHT,
    CARD_F_MIX1_OUTPUT_BUTTON_LIGHT,
    CARD_A_MIX2_OUTPUT_BUTTON_LIGHT,
    CARD_B_MIX2_OUTPUT_BUTTON_LIGHT,
    CARD_C_MIX2_OUTPUT_BUTTON_LIGHT,
    CARD_D_MIX2_OUTPUT_BUTTON_LIGHT,
    CARD_E_MIX2_OUTPUT_BUTTON_LIGHT,
    CARD_F_MIX2_OUTPUT_BUTTON_LIGHT,
    ENUMS(LEFT_EXPANDER_LIGHT, 3),
    ENUMS(RIGHT_EXPANDER_LIGHT, 3),
    LIGHTS_LEN
  };


  // Global access point: no writes allowed from the audio thread
  static std::atomic<AudioIO*> instance;

  // Broker access
  Broker& getBroker() { return broker; }

  AudioIO();
  ~AudioIO();

  // Module methods
  void onAdd(const AddEvent &e) override;
  void onRemove(const RemoveEvent &e) override;
  void onReset() override;
  void onSampleRateChange(const SampleRateChangeEvent& e) override;
  void process(const ProcessArgs& args) override;

  json_t* dataToJson() override;
  void dataFromJson(json_t* rootJ) override;


  std::string cardAOutput1NameString;
  std::string cardAOutput2NameString;
  std::string cardBOutput1NameString;
  std::string cardBOutput2NameString;
  std::string cardCOutput1NameString;
  std::string cardCOutput2NameString;
  std::string cardDOutput1NameString;
  std::string cardDOutput2NameString;
  std::string cardEOutput1NameString;
  std::string cardEOutput2NameString;
  std::string cardFOutput1NameString;
  std::string cardFOutput2NameString;


  std::vector<ZoxnoxiousAudioPort*> audioPorts;
  ZoxnoxiousMidiOutput midiOutput;
  midi::InputQueue midiInput;

  // Discovery related variables
  midi::Message MIDI_DISCOVERY_REQUEST_SYSEX; // this should be const static
  midi::Message MIDI_SHUTDOWN_SYSEX;
  midi::Message MIDI_RESTART_SYSEX;
  midi::Message MIDI_TUNE_REQUEST;

  bool isPrimary() const;

private:
  Broker broker;

  dsp::ClockDivider lightDivider;
  float out1LevelClipTimer;
  float out2LevelClipTimer;


  struct buttonParamMidiProgram {
    enum ParamId button;
    int previousValue;
    uint8_t midiProgram[2];
  } buttonParamToMidiProgramList[12] =
  {
    // the ordering here is important- taking a dependency on
    // ordering when producing midi messages
    { CARD_A_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 0, 1 } },
    { CARD_B_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 2, 3 } },
    { CARD_C_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 4, 5 } },
    { CARD_D_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 6, 7 } },
    { CARD_E_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 8, 9 } },
    { CARD_F_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 10, 11 } },
    { CARD_A_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 12, 13 } },
    { CARD_B_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 14, 15 } },
    { CARD_C_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 16, 17 } },
    { CARD_D_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 18, 19 } },
    { CARD_E_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 20, 21 } },
    { CARD_F_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 22, 23 } }
  };


  uint8_t getHardwareId();
  int8_t cvChannelOffset;
  int8_t outputDeviceId;
  int8_t midiChannel;

  bool discoveryReportReceived = false;
  dsp::ClockDivider discoveryRequestClockDivider;

  dsp::ClockDivider midiPollClockDivider;

  int sendMidiProgramChangeMessage(int programNumber);
  void processMidiInMessage(const midi::Message &msg);
  void processDiscoveryReport(const midi::Message &msg);
  void applyDiscoveryReport(DiscoveredCard *cards);

  static const std::string audioPortNum;
};


} // namespace zox
