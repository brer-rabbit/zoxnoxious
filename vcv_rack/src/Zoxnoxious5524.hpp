#pragma once

#include "plugin.hpp"
#include "constants.hpp"
#include "modulehelpers.hpp"
#include "zcomponentlib.hpp"
#include "ParticipantAdapter.hpp"

namespace zox {

struct CouplingDisplayState {
  float vco2RawToVco1Tzfm = 0.f;
  float vco2ShapedToVco1Tzfm = 0.f;
  int vco2RawWaveMask = 0;
  int vco2ShapedWaveMask = 0;

  float vco1ToVco2ExpFm = 0.f;
  float vco1ToVcfExpFm = 0.f;
  float vco2ToVcfLinFm = 0.f;

  bool syncHardSub = false;
  bool syncSoft = false;
  bool vco1ToVco2WaveSelect = false;
};

enum RawWaveMask {
  RAW_PULSE = 1 << 0,
  RAW_SAW   = 1 << 1,
  RAW_TRI   = 1 << 2
};

enum ShapedWaveMask {
  SHAPED_PULSE     = 1 << 0,
  SHAPED_HALF_SINE = 1 << 1,
  SHAPED_SINE      = 1 << 2
};

struct Zoxnoxious5524 final : ParticipantAdapter, Participant {
  enum ParamId {
    VCO_ONE_VOCT_KNOB_PARAM,
    VCO_ONE_PW_KNOB_PARAM,
    VCO_ONE_LINEAR_KNOB_PARAM,
    VCO_TWO_VOCT_KNOB_PARAM,
    VCO_TWO_PW_KNOB_PARAM,
    VCF_CUTOFF_KNOB_PARAM,
    VCF_RESONANCE_KNOB_PARAM,
    VCO_MIX_KNOB_PARAM,
    FINAL_GAIN_KNOB_PARAM,
    VCO_ONE_PULSE_KNOB_PARAM,
    VCO_ONE_TRIANGLE_KNOB_PARAM,
    VCO_ONE_SAW_KNOB_PARAM,
    VCO_TWO_WAVE_PULSE_BUTTON_PARAM,
    VCO_TWO_WAVE_SAW_BUTTON_PARAM,
    VCO_TWO_WAVE_TRI_BUTTON_PARAM,
    VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_PARAM,
    VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_PARAM,
    VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_PARAM,
    VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_PARAM,
    VCO_ONE_MOD_AMOUNT_KNOB_PARAM,
    VCO_TWO_MOD_AMOUNT_KNOB_PARAM,
    VCO_TWO_WAVESHAPE_TZFM_KNOB_PARAM,
    VCO_TWO_TRI_VCF_KNOB_PARAM,
    VCO_ONE_TO_PW_VCO_TWO_BUTTON_PARAM,
    VCO_ONE_TO_VCF_BUTTON_PARAM,
    VCO_TWO_TO_PW_VCO_ONE_BUTTON_PARAM,
    VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    // inputs are in DAC order from schematic
    // DAC AS3394 / SPI chip select 0
    VCO_MIX_INPUT,
    VCO_TWO_PW_INPUT,
    FINAL_GAIN_INPUT,
    VCO_TWO_TRI_VCF_INPUT,
    VCF_RESONANCE_INPUT,
    VCO_ONE_MOD_AMOUNT_INPUT,
    VCO_TWO_VOCT_INPUT,
    VCF_CUTOFF_INPUT,
    // DAC SSI2130 / SPI chip select 1
    VCO_ONE_TRIANGLE_INPUT,
    VCO_ONE_LINEAR_INPUT,
    VCO_ONE_VOCT_INPUT,
    VCO_ONE_PW_INPUT,
    VCO_TWO_WAVESHAPE_TZFM_INPUT,
    VCO_TWO_MOD_AMOUNT_INPUT,
    VCO_ONE_PULSE_INPUT,
    VCO_ONE_SAW_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    OUTPUTS_LEN
  };
  enum LightId {
    VCO_ONE_VOCT_CLIP_LIGHT,
    VCO_ONE_PW_CLIP_LIGHT,
    VCO_ONE_LINEAR_CLIP_LIGHT,
    VCO_TWO_VOCT_CLIP_LIGHT,
    VCO_TWO_PW_CLIP_LIGHT,
    VCF_CUTOFF_CLIP_LIGHT,
    VCF_RESONANCE_CLIP_LIGHT,
    VCO_MIX_CLIP_LIGHT,
    FINAL_GAIN_CLIP_LIGHT,
    VCO_ONE_PULSE_CLIP_LIGHT,
    VCO_ONE_TRIANGLE_CLIP_LIGHT,
    VCO_ONE_SAW_CLIP_LIGHT,
    VCO_ONE_MOD_AMOUNT_CLIP_LIGHT,
    VCO_TWO_MOD_AMOUNT_CLIP_LIGHT,
    VCO_TWO_WAVESHAPE_TZFM_CLIP_LIGHT,
    VCO_TWO_TRI_VCF_CLIP_LIGHT,
    VCO_TWO_WAVE_PULSE_BUTTON_LIGHT,
    VCO_TWO_WAVE_SAW_BUTTON_LIGHT,
    VCO_TWO_WAVE_TRI_BUTTON_LIGHT,
    VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_LIGHT,
    VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_LIGHT,
    VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_LIGHT,
    VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_LIGHT,
    VCO_ONE_TO_PW_VCO_TWO_BUTTON_LIGHT,
    VCO_ONE_TO_VCF_BUTTON_LIGHT,
    VCO_TWO_TO_PW_VCO_ONE_BUTTON_LIGHT,
    VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_LIGHT,
    TZFM_PULSE_STATUS_LIGHT,
    TZFM_SAW_STATUS_LIGHT,
    TZFM_TRI_STATUS_LIGHT,
    WAVESHAPE_PULSE_STATUS_LIGHT,
    WAVESHAPE_HALFSINE_STATUS_LIGHT,
    WAVESHAPE_SINE_STATUS_LIGHT,
    ENUMS(LINK_STATUS_LIGHT, 3),
    LIGHTS_LEN
  };

  float vcoOneVoctClipTimer = 0.f;
  float vcoOnePwClipTimer = 0.f;
  float vcoOneLinearClipTimer = 0.f;
  float vcoTwoVoctClipTimer = 0.f;
  float vcoTwoPwClipTimer = 0.f;
  float vcfCutoffClipTimer = 0.f;
  float vcfResonanceClipTimer = 0.f;
  float vcoMixClipTimer = 0.f;
  float finalGainClipTimer = 0.f;
  float vcoOnePulseClipTimer = 0.f;
  float vcoOneTriangleClipTimer = 0.f;
  float vcoOneSawClipTimer = 0.f;
  float vcoOneModAmountClipTimer = 0.f;
  float vcoTwoModAmountClipTimer = 0.f;
  float vcoTwoWaveshapeTzfmClipTimer = 0.f;
  float vcoTwoTriVcfClipTimer = 0.f;

  std::string output1NameString;
  std::string output2NameString;

  // mapping button switches to send MIDI program changes.

  // VCO2 Saw and Tri params send a single MIDI prog change.
  // the const is the program change offset from zero for the program change.
  // Four program changes handle off/off, off/on, on/off, on/on.
  // With a value of 0, the prog changes for tri/saw are 0,1,2,3.
  const uint8_t vcoTwoSawTriMidiProgramOffset = 0;
  uint8_t vcoTwoTriSawPrevState = 255; // init to invalid state


  // index corresponds on both vectors for tracking button pushes and outgoing MIDI msg
  static const std::vector<ButtonMapping<Zoxnoxious5524> > buttonMappings;
  ButtonMidiController<Zoxnoxious5524> buttonMidiController;

  // VCO Two pulse is enabled/disabled by setting the pulse width to
  // minimum value.  Track that outside of buttonParamToMidiProgramList.
  bool vcoTwoPulseEnabled = false;

  // limit pulse width to prevent it from going to DC
  // 0 => limit
  // 1 => allow DC
  int pwLimit = 0;

  std::array<CvRoute,16> routes;

  // for the visualizer
  CouplingDisplayState getCouplingDisplayState();


  Zoxnoxious5524();
  void pullSamples(const rack::engine::Module::ProcessArgs &args, dsp::Frame<maxAudioChannels> &sharedFrame, int offset) override;
  bool pullMidi(const rack::engine::Module::ProcessArgs &args, uint32_t clockDivision, int midiChannel, midi::Message &midiMessage) override;

  uint8_t getHardwareId() const override;
  int64_t getModuleId() override;
  void onReset(const ResetEvent& e) override;
  void onAttach() override;

  json_t* dataToJson() override;
  void dataFromJson(json_t* rootJ) override;

};

} // namespace zox
