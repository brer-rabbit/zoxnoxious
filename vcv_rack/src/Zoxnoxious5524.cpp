#include "plugin.hpp"
#include "common.hpp"
#include "zcomponentlib.hpp"
#include "ParticipantAdapter.hpp"

namespace zox {

// VCO1 ==> SSI2130
// VCO2 ==> AS3394

// these enums define the channel order being sent over the wire
enum cvChannel {
    // DAC AS3394 / chip select 0
    VCO_MIX = 0,
    VCO_TWO_PW,
    FINAL_GAIN,
    VCO_TWO_TRI_VCF,
    VCF_RESONANCE,
    VCO_ONE_MOD_AMOUNT,
    VCO_TWO_VOCT,
    VCF_CUTOFF,
    // DAC SSI2130 / SPI chip select 1
    VCO_ONE_TRIANGLE,
    VCO_ONE_LINEAR,
    VCO_ONE_VOCT,
    VCO_ONE_PW,
    VCO_TWO_WAVESHAPE_TZFM,
    VCO_TWO_MOD_AMOUNT,
    VCO_ONE_PULSE,
    VCO_ONE_SAW
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
    ENUMS(LEFT_EXPANDER_LIGHT, 3),
    ENUMS(RIGHT_EXPANDER_LIGHT, 3),
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
  std::vector<ButtonState> buttonStates;
  ButtonMidiController<Zoxnoxious5524> buttonMidiController;


  // VCO Two pulse is enabled/disabled by setting the pulse width to
  // minimum value.  Track that outside of buttonParamToMidiProgramList.
  bool vcoTwoPulseEnabled = false;

  // limit pulse width to prevent it from going to DC
  // 0 => limit
  // 1 => allow DC
  int pwLimit = 0;

  std::array<CvRoute,16> routes;

  Zoxnoxious5524() : output1NameString(invalidCardOutputName),
    output2NameString(invalidCardOutputName),
    buttonStates(buttonMappings.size()),
    buttonMidiController(buttonMappings),
    routes{{
      {VCO_ONE_SAW_KNOB_PARAM, VCO_ONE_SAW_INPUT, VCO_ONE_SAW, 10.f, &vcoOneSawClipTimer, nullptr},
      {VCO_ONE_PULSE_KNOB_PARAM, VCO_ONE_PULSE_INPUT, VCO_ONE_PULSE, 10.f, &vcoOnePulseClipTimer, nullptr},
      {VCO_TWO_MOD_AMOUNT_KNOB_PARAM, VCO_TWO_MOD_AMOUNT_INPUT, VCO_TWO_MOD_AMOUNT, 10.f, &vcoTwoModAmountClipTimer, nullptr},
      {VCO_TWO_WAVESHAPE_TZFM_KNOB_PARAM, VCO_TWO_WAVESHAPE_TZFM_INPUT, VCO_TWO_WAVESHAPE_TZFM, 10.f, &vcoTwoWaveshapeTzfmClipTimer, nullptr},
      {VCO_ONE_PW_KNOB_PARAM,VCO_ONE_PW_INPUT, VCO_ONE_PW, 10.f, &vcoOnePwClipTimer, nullptr},
      {VCO_ONE_VOCT_KNOB_PARAM, VCO_ONE_VOCT_INPUT, VCO_ONE_VOCT, 8.f, &vcoOneVoctClipTimer, nullptr},
      {VCO_ONE_LINEAR_KNOB_PARAM, VCO_ONE_LINEAR_INPUT, VCO_ONE_LINEAR, 10.f, &vcoOneLinearClipTimer, nullptr},
      {VCO_ONE_TRIANGLE_KNOB_PARAM, VCO_ONE_TRIANGLE_INPUT, VCO_ONE_TRIANGLE, 10.f, &vcoOneTriangleClipTimer, nullptr},
      {VCF_CUTOFF_KNOB_PARAM,VCF_CUTOFF_INPUT, VCF_CUTOFF, 10.f, &vcfCutoffClipTimer, nullptr},
      {VCO_TWO_VOCT_KNOB_PARAM, VCO_TWO_VOCT_INPUT, VCO_TWO_VOCT, 6.f, &vcoTwoVoctClipTimer, nullptr},
      {VCO_ONE_MOD_AMOUNT_KNOB_PARAM, VCO_ONE_MOD_AMOUNT_INPUT, VCO_ONE_MOD_AMOUNT, 10.f, &vcoOneModAmountClipTimer, nullptr},
      {VCF_RESONANCE_KNOB_PARAM, VCF_RESONANCE_INPUT, VCF_RESONANCE, 10.f, &vcfResonanceClipTimer, dualLinearSwitch0_8},
      {VCO_TWO_TRI_VCF_KNOB_PARAM, VCO_TWO_TRI_VCF_INPUT, VCO_TWO_TRI_VCF, 10.f, &vcoTwoTriVcfClipTimer, nullptr},
      {FINAL_GAIN_KNOB_PARAM, FINAL_GAIN_INPUT, FINAL_GAIN, 10.f, &finalGainClipTimer, nullptr},
      {VCO_TWO_PW_KNOB_PARAM,VCO_TWO_PW_INPUT, VCO_TWO_PW, 10.f, &vcoTwoPwClipTimer, nullptr},
      {VCO_MIX_KNOB_PARAM, VCO_MIX_INPUT, VCO_MIX, 10.f, &vcoMixClipTimer, nullptr} }} {

    setParticipant(this);
    setLightEnum(RIGHT_EXPANDER_LIGHT);

    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(VCO_ONE_VOCT_KNOB_PARAM, 0.f, 1.f, 0.5f, "Frequency", " V", 0.f, 8.f);
    configParam(VCO_ONE_PW_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pulse Width", "%", 0.f, 100.f);
    configParam(VCO_ONE_LINEAR_KNOB_PARAM, 0.f, 1.f, 0.5f, "Linear Mod", " V", 0.f, 10.f -5.f);
    configParam(VCO_TWO_VOCT_KNOB_PARAM, 0.f, 1.f, 0.5f, "Frequency", " V", 0.f, 6.f);
    configParam(VCO_TWO_PW_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pulse Width", "%", 0.f, 100.f);
    configParam(VCF_CUTOFF_KNOB_PARAM, 0.f, 1.f, 1.f, "Cutoff", " V", 0.f, 10.f, -1.f);
    configParam(VCF_RESONANCE_KNOB_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
    configParam(VCO_MIX_KNOB_PARAM, 0.f, 1.f, 0.5f, "VCO1/VCO2 Mix", "%", 0.f, 200.f, -100.f);
    configParam(FINAL_GAIN_KNOB_PARAM, 0.f, 1.f, 0.f, "Final VCA", "%", 0.f, 100.f);
    configParam(VCO_ONE_PULSE_KNOB_PARAM, 0.f, 1.f, 0.f, "Pulse Gain", "%", 0.f, 100.f);
    configParam(VCO_ONE_TRIANGLE_KNOB_PARAM, 0.f, 1.f, 1.f, "Tri Gain", "%", 0.f, 100.f);
    configParam(VCO_ONE_SAW_KNOB_PARAM, 0.f, 1.f, 0.f, "Saw Gain", "%", 0.f, 100.f);

    configParam(VCO_ONE_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Mod Amount", "%", 0.f, 100.f);
    configParam(VCO_TWO_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Mod Amount", "%", 0.f, 100.f);
    configParam(VCO_TWO_WAVESHAPE_TZFM_KNOB_PARAM, 0.f, 1.f, 0.f, "Waveshape TZFM", "%", 0.f, 100.f);
    configParam(VCO_TWO_TRI_VCF_KNOB_PARAM, 0.f, 1.f, 0.f, "Tri to VCF", "%", 0.f, 100.f);

    configSwitch(VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO One to Exp FM VCO Two", {"Off", "On"});
    configSwitch(VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO One to Wave Select VCO Two", {"Off", "On"});

    configSwitch(VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two to TZ-FM VCO One", {"Off", "On"});
    configSwitch(VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Soft Sync VCO One from VCO Two", {"Off", "On"});
    configSwitch(VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Hard Sub Sync VCO One from VCO Two", {"Off", "On"});

    configSwitch(VCO_ONE_TO_PW_VCO_TWO_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO One to VCO Two Pulse Width", {"Off", "On"});
    configSwitch(VCO_ONE_TO_VCF_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO One to VCF Freq Cutoff", {"Off", "On"});
    configSwitch(VCO_TWO_TO_PW_VCO_ONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two to VCO One Pulse Width", {"Off", "On"});

    configSwitch(VCO_TWO_WAVE_PULSE_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two Pulse Enable", {"Off", "On"});
    configSwitch(VCO_TWO_WAVE_SAW_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two Sawtooth Enable", {"Off", "On"});
    configSwitch(VCO_TWO_WAVE_TRI_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two Triangle Enable", {"Off", "On"});

    configInput(VCO_ONE_VOCT_INPUT, "VCO One V/Oct");
    configInput(VCO_ONE_PW_INPUT, "VCO One Pulse Width");
    configInput(VCO_ONE_LINEAR_INPUT, "VCO One Linear Freq Mod");
    configInput(VCO_TWO_VOCT_INPUT, "VCO Two V/Oct");
    configInput(VCO_TWO_PW_INPUT, "VCO Two Pulse Width");
    configInput(VCF_CUTOFF_INPUT, "VCF Cutoff Frequency");
    configInput(VCF_RESONANCE_INPUT, "VCF Resonance");
    configInput(VCO_MIX_INPUT, "VCO One / VCO Two Mix");
    configInput(FINAL_GAIN_INPUT, "Final VCA Gain");
    configInput(VCO_ONE_PULSE_INPUT, "VCO One Pulse Level");
    configInput(VCO_ONE_TRIANGLE_INPUT, "VCO One Triangle Level");
    configInput(VCO_ONE_SAW_INPUT, "VCO One Sawtooth Level");
    configInput(VCO_ONE_MOD_AMOUNT_INPUT, "VCO One Mod Level");
    configInput(VCO_TWO_MOD_AMOUNT_INPUT, "VCO Two Mod Level");
    configInput(VCO_TWO_WAVESHAPE_TZFM_INPUT, "VCO Two Waveshape to VCO One TZFM");
    configInput(VCO_TWO_TRI_VCF_INPUT, "VCO Two Triangel to VCF Cutoff Frequency");

    output1NameString.reserve(16);
    output2NameString.reserve(16);
  }


  void pullSamples(const rack::engine::Module::ProcessArgs &args, dsp::Frame<maxAudioChannels> &sharedFrame, int offset) override {
    const float clipTime = 0.25f;

    // match the channels to what the Pi expects on the other side and it's all good
    processCvRoutes(routes.data(),
                    routes.size(),
                    clipTime,
                    offset,
                    sharedFrame.samples,
                    params.data(),
                    inputs.data());

    // VCO Two PW
    // Note Pulse Width handles enable/disable
    vcoTwoPulseEnabled = static_cast<bool>(params[VCO_TWO_WAVE_PULSE_BUTTON_PARAM].getValue());
    if (vcoTwoPulseEnabled) {
      if (pwLimit) {
        sharedFrame.samples[offset + VCO_TWO_PW] = sharedFrame.samples[offset + VCO_TWO_PW] * 0.88f;
      }
    }
    else {
      sharedFrame.samples[offset + VCO_TWO_PW] = 1.f;
    }

  }




  bool pullMidi(const rack::engine::Module::ProcessArgs &args, uint32_t clockDivision, int midiChannel, midi::Message &midiMessage) override {
    bool midiMsgSet = false;

    // pullMidi runs at something better than display frame rate, so all UI visual stuff is here

    // the pulse doesn't send any midi-- it just allows or limits the pulse width
    lights[VCO_TWO_WAVE_PULSE_BUTTON_LIGHT].setBrightness(
      params[VCO_TWO_WAVE_PULSE_BUTTON_PARAM].getValue() > 0.f
    );


    int vcoTwoSaw;
    int vcoTwoTri;
    if (params[VCO_TWO_WAVE_SAW_BUTTON_PARAM].getValue() > 0.f) {
      vcoTwoSaw = 2;
      lights[VCO_TWO_WAVE_SAW_BUTTON_LIGHT].setBrightness(1.f);
    } else {
      vcoTwoSaw = 0;
      lights[VCO_TWO_WAVE_SAW_BUTTON_LIGHT].setBrightness(0.f);
    }

    if (params[VCO_TWO_WAVE_TRI_BUTTON_PARAM].getValue() > 0.f) {
      vcoTwoTri = 1;
      lights[VCO_TWO_WAVE_TRI_BUTTON_LIGHT].setBrightness(1.f);
    } else {
      vcoTwoTri = 0;
      lights[VCO_TWO_WAVE_TRI_BUTTON_LIGHT].setBrightness(0.f);
    }

    // convert the saw & tri params to the MIDI prog change number with four options
    int vcoTwoTriSawCurrentState = vcoTwoSaw + vcoTwoTri + vcoTwoSawTriMidiProgramOffset;
    if (vcoTwoTriSawCurrentState != vcoTwoTriSawPrevState) {
      vcoTwoTriSawPrevState = vcoTwoTriSawCurrentState;
      setMidiProgramChangeMessage(midiMessage, midiChannel, vcoTwoTriSawCurrentState);
      midiMsgSet = true;
    }


    // all buttons outside of the above selector are handled here
    if (!midiMsgSet) {
      midiMsgSet = buttonMidiController.process(this, midiChannel, midiMessage);
    }
    buttonMidiController.updateLights(this);


    // clipping lights and timers
    const float lightTime = args.sampleTime * clockDivision;
    const float brightnessDeltaTime = 1 / lightTime;

    vcoOneVoctClipTimer -= lightTime;
    lights[VCO_ONE_VOCT_CLIP_LIGHT].setBrightnessSmooth(vcoOneVoctClipTimer > 0.f, brightnessDeltaTime);

    vcoOnePwClipTimer -= lightTime;
    lights[VCO_ONE_PW_CLIP_LIGHT].setBrightnessSmooth(vcoOnePwClipTimer > 0.f, brightnessDeltaTime);

    vcoOneLinearClipTimer -= lightTime;
    lights[VCO_ONE_LINEAR_CLIP_LIGHT].setBrightnessSmooth(vcoOneLinearClipTimer > 0.f, brightnessDeltaTime);

    vcoTwoVoctClipTimer -= lightTime;
    lights[VCO_TWO_VOCT_CLIP_LIGHT].setBrightnessSmooth(vcoTwoVoctClipTimer > 0.f, brightnessDeltaTime);

    vcoTwoPwClipTimer -= lightTime;
    lights[VCO_TWO_PW_CLIP_LIGHT].setBrightnessSmooth(vcoTwoPwClipTimer > 0.f, brightnessDeltaTime);

    vcfCutoffClipTimer -= lightTime;
    lights[VCF_CUTOFF_CLIP_LIGHT].setBrightnessSmooth(vcfCutoffClipTimer > 0.f, brightnessDeltaTime);

    vcfResonanceClipTimer -= lightTime;
    lights[VCF_RESONANCE_CLIP_LIGHT].setBrightnessSmooth(vcfResonanceClipTimer > 0.f, brightnessDeltaTime);

    vcoMixClipTimer -= lightTime;
    lights[VCO_MIX_CLIP_LIGHT].setBrightnessSmooth(vcoMixClipTimer > 0.f, brightnessDeltaTime);

    finalGainClipTimer -= lightTime;
    lights[FINAL_GAIN_CLIP_LIGHT].setBrightnessSmooth(finalGainClipTimer > 0.f, brightnessDeltaTime);

    vcoOnePulseClipTimer -= lightTime;
    lights[VCO_ONE_PULSE_CLIP_LIGHT].setBrightnessSmooth(vcoOnePulseClipTimer > 0.f, brightnessDeltaTime);

    vcoOneTriangleClipTimer -= lightTime;
    lights[VCO_ONE_TRIANGLE_CLIP_LIGHT].setBrightnessSmooth(vcoOneTriangleClipTimer > 0.f, brightnessDeltaTime);

    vcoOneSawClipTimer -= lightTime;
    lights[VCO_ONE_SAW_CLIP_LIGHT].setBrightnessSmooth(vcoOneSawClipTimer > 0.f, brightnessDeltaTime);

    vcoOneModAmountClipTimer -= lightTime;
    lights[VCO_ONE_MOD_AMOUNT_CLIP_LIGHT].setBrightnessSmooth(vcoOneModAmountClipTimer > 0.f, brightnessDeltaTime);

    vcoTwoModAmountClipTimer -= lightTime;
    lights[VCO_TWO_MOD_AMOUNT_CLIP_LIGHT].setBrightnessSmooth(vcoTwoModAmountClipTimer > 0.f, brightnessDeltaTime);

    vcoTwoWaveshapeTzfmClipTimer -= lightTime;
    lights[VCO_TWO_WAVESHAPE_TZFM_CLIP_LIGHT].setBrightnessSmooth(vcoTwoWaveshapeTzfmClipTimer > 0.f, brightnessDeltaTime);

    vcoTwoTriVcfClipTimer -= lightTime;
    lights[VCO_TWO_TRI_VCF_CLIP_LIGHT].setBrightnessSmooth(vcoTwoTriVcfClipTimer > 0.f, brightnessDeltaTime);

    return midiMsgSet;
  }


  /** getCardHardwareId
   * return the hardware Id of the card
   */
  static const uint8_t hardwareId = 0x04;
  uint8_t getHardwareId() const override {
    return hardwareId;
  }

  /* Participant interface: return Module identifier */
  int64_t getModuleId() override {
    return getId();
  }

  void onReset(const ResetEvent& e) override {
    Module::onReset(e);
    pwLimit = 0;
  }

  void onAttach() override {
    if (lifecycle.nameService == nullptr) {
      return;
    }
    auto *ptr1 = lifecycle.nameService->getNamePtr(lifecycle.slotNum * 2);
    auto *ptr2 = lifecycle.nameService->getNamePtr(lifecycle.slotNum * 2 + 1);
    output1NameString = ptr1 ? *ptr1 : invalidCardOutputName;
    output2NameString = ptr2 ? *ptr2 : invalidCardOutputName;
  }


  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "pwLimit", json_integer(pwLimit));
    return rootJ;
  }

  void dataFromJson(json_t* rootJ) override {
    json_t* pwLimitJ = json_object_get(rootJ, "pwLimit");
    if (pwLimitJ) {
      pwLimit = json_integer_value(pwLimitJ);
    }
  }

};




struct Zoxnoxious5524Widget : ModuleWidget {
    Zoxnoxious5524Widget(Zoxnoxious5524* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Zoxnoxious5524.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.298, 25.908)), module, Zoxnoxious5524::VCO_ONE_VOCT_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.903, 25.908)), module, Zoxnoxious5524::VCO_ONE_PW_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(49.645, 25.908)), module, Zoxnoxious5524::VCO_ONE_LINEAR_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(73.245, 25.908)), module, Zoxnoxious5524::VCO_TWO_VOCT_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(90.987, 25.908)), module, Zoxnoxious5524::VCO_TWO_PW_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(127.94, 25.908)), module, Zoxnoxious5524::VCF_CUTOFF_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.544, 25.908)), module, Zoxnoxious5524::VCF_RESONANCE_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(127.94, 62.522)), module, Zoxnoxious5524::VCO_MIX_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.544, 62.522)), module, Zoxnoxious5524::FINAL_GAIN_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.298, 65.704)), module, Zoxnoxious5524::VCO_ONE_PULSE_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(49.645, 65.594)), module, Zoxnoxious5524::VCO_ONE_TRIANGLE_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.903, 65.867)), module, Zoxnoxious5524::VCO_ONE_SAW_KNOB_PARAM));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.705, 102.585)), module, Zoxnoxious5524::VCO_ONE_MOD_AMOUNT_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(73.225, 103.172)), module, Zoxnoxious5524::VCO_TWO_MOD_AMOUNT_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(127.94, 103.172)), module, Zoxnoxious5524::VCO_TWO_WAVESHAPE_TZFM_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.045, 103.397)), module, Zoxnoxious5524::VCO_TWO_TRI_VCF_KNOB_PARAM));


        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(73.245, 66.311)), module, Zoxnoxious5524::VCO_TWO_WAVE_PULSE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_WAVE_PULSE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(90.518, 66.311)), module, Zoxnoxious5524::VCO_TWO_WAVE_SAW_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_WAVE_SAW_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(107.792, 66.311)), module, Zoxnoxious5524::VCO_TWO_WAVE_TRI_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_WAVE_TRI_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(31.971, 100.58)), module, Zoxnoxious5524::VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_PARAM, Zoxnoxious5524::VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(47.31, 100.58)), module, Zoxnoxious5524::VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_PARAM, Zoxnoxious5524::VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(90.846, 100.58)), module, Zoxnoxious5524::VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(110.444, 100.58)), module, Zoxnoxious5524::VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(31.971, 115.486)), module, Zoxnoxious5524::VCO_ONE_TO_PW_VCO_TWO_BUTTON_PARAM, Zoxnoxious5524::VCO_ONE_TO_PW_VCO_TWO_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(47.31, 115.486)), module, Zoxnoxious5524::VCO_ONE_TO_VCF_BUTTON_PARAM, Zoxnoxious5524::VCO_ONE_TO_VCF_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(90.846, 115.486)), module, Zoxnoxious5524::VCO_TWO_TO_PW_VCO_ONE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_TO_PW_VCO_ONE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(110.444, 115.486)), module, Zoxnoxious5524::VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_LIGHT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.298, 39.271)), module, Zoxnoxious5524::VCO_ONE_VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.903, 39.271)), module, Zoxnoxious5524::VCO_ONE_PW_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(49.645, 39.271)), module, Zoxnoxious5524::VCO_ONE_LINEAR_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(73.245, 39.271)), module, Zoxnoxious5524::VCO_TWO_VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(90.987, 39.271)), module, Zoxnoxious5524::VCO_TWO_PW_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(127.94, 39.271)), module, Zoxnoxious5524::VCF_CUTOFF_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.544, 39.271)), module, Zoxnoxious5524::VCF_RESONANCE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(127.94, 75.884)), module, Zoxnoxious5524::VCO_MIX_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.544, 75.884)), module, Zoxnoxious5524::FINAL_GAIN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.298, 79.066)), module, Zoxnoxious5524::VCO_ONE_PULSE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(49.645, 78.956)), module, Zoxnoxious5524::VCO_ONE_TRIANGLE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.903, 79.229)), module, Zoxnoxious5524::VCO_ONE_SAW_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.705, 115.948)), module, Zoxnoxious5524::VCO_ONE_MOD_AMOUNT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(73.225, 116.534)), module, Zoxnoxious5524::VCO_TWO_MOD_AMOUNT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(127.94, 116.534)), module, Zoxnoxious5524::VCO_TWO_WAVESHAPE_TZFM_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.045, 116.759)), module, Zoxnoxious5524::VCO_TWO_TRI_VCF_INPUT));

        addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(2.020, 8.219)), module, Zoxnoxious5524::LEFT_EXPANDER_LIGHT));
        addChild(createLightCentered<TriangleRightLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(160.346, 8.219)), module, Zoxnoxious5524::RIGHT_EXPANDER_LIGHT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.261, 31.798)), module, Zoxnoxious5524::VCO_ONE_VOCT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(35.865, 31.798)), module, Zoxnoxious5524::VCO_ONE_PW_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.608, 31.798)), module, Zoxnoxious5524::VCO_ONE_LINEAR_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(77.208, 31.798)), module, Zoxnoxious5524::VCO_TWO_VOCT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(94.95, 31.798)), module, Zoxnoxious5524::VCO_TWO_PW_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(131.902, 31.798)), module, Zoxnoxious5524::VCF_CUTOFF_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(151.507, 31.798)), module, Zoxnoxious5524::VCF_RESONANCE_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(131.902, 56.411)), module, Zoxnoxious5524::VCO_MIX_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(151.507, 56.411)), module, Zoxnoxious5524::FINAL_GAIN_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.261, 71.704)), module, Zoxnoxious5524::VCO_ONE_PULSE_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.608, 71.593)), module, Zoxnoxious5524::VCO_ONE_TRIANGLE_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(35.866, 71.867)), module, Zoxnoxious5524::VCO_ONE_SAW_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.668, 108.585)), module, Zoxnoxious5524::VCO_ONE_MOD_AMOUNT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(77.188, 109.172)), module, Zoxnoxious5524::VCO_TWO_MOD_AMOUNT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(131.902, 109.172)), module, Zoxnoxious5524::VCO_TWO_WAVESHAPE_TZFM_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(151.008, 109.286)), module, Zoxnoxious5524::VCO_TWO_TRI_VCF_CLIP_LIGHT));


        mix1OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(139.527, 50.578)));
        mix1OutputTextField->box.size = (mm2px(Vec(18.0, 3.636)));
        mix1OutputTextField->setText(module ? &module->output1NameString : NULL);
        addChild(mix1OutputTextField);

        mix2OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(40.269, 48.012)));
        mix2OutputTextField->box.size = (mm2px(Vec(18.0, 3.636)));
        mix2OutputTextField->setText(module ? &module->output2NameString : NULL);
        addChild(mix2OutputTextField);
    }

    void appendContextMenu(Menu* menu) override {
        Zoxnoxious5524* module = getModule<Zoxnoxious5524>();
        menu->addChild(new MenuSeparator);
        menu->addChild(createIndexPtrSubmenuItem("Pulse Width", {"Allow DC", "Limit"}, &module->pwLimit));
    }

    CardTextDisplay *mix1OutputTextField;
    CardTextDisplay *mix2OutputTextField;
};


const std::vector<ButtonMapping<Zoxnoxious5524> > Zoxnoxious5524::buttonMappings = {
  { VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_PARAM, VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_LIGHT, {4,5} },
  { VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_PARAM, VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_LIGHT, {6,7} },
  { VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_PARAM, VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_LIGHT, {8,9} },
  { VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_PARAM, VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_LIGHT, {10,11 } },
  { VCO_ONE_TO_PW_VCO_TWO_BUTTON_PARAM, VCO_ONE_TO_PW_VCO_TWO_BUTTON_LIGHT, {12,13} },
  { VCO_ONE_TO_VCF_BUTTON_PARAM, VCO_ONE_TO_VCF_BUTTON_LIGHT, {14,15} },
  { VCO_TWO_TO_PW_VCO_ONE_BUTTON_PARAM, VCO_TWO_TO_PW_VCO_ONE_BUTTON_LIGHT, {16,17 } },
  { VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_PARAM, VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_LIGHT, {18,19 } }
};


} // namespace zox


Model* modelZoxnoxious5524 = createModel<zox::Zoxnoxious5524, zox::Zoxnoxious5524Widget>("Zoxnoxious5524");
