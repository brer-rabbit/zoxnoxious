#include "plugin.hpp"
#include "constants.hpp"
#include "modulehelpers.hpp"
#include "zcomponentlib.hpp"
#include "ParticipantAdapter.hpp"
#include "TurnsCountingKnob.hpp"

namespace zox {

// map wired mux inputs to signal from cardOutputNames array
// (card(N-1) * 2 + (out(N-1))
// card7 out1
// card6 out2
// card5 out1
// card4 out2
// card3 out1
// card2 out2
// card1 out2
// card1 out1
static const int source1Sources[] = { 0, 1, 3, 4, 7, 8, 11, 12 };

// card6 out1
// card5 out2
// card4 out1
// card3 out2
// card2 out2
// card2 out1
// card1 out2
// card1 out1
static const int source2Sources[] = { 0, 1, 2, 3, 5, 6, 9, 10 };

// cv channels to send over the audio device.  This must match the receiver, and is
// offset by what the expander provides with cvChannelOffset.
enum cvChannel {
  VCF_CUTOFF = 0,
  SOURCE_ONE_LEVEL,
  SOURCE_TWO_LEVEL,
  SOURCE_ONE_MOD_AMOUNT,
  SOURCE_TWO_MOD_AMOUNT,
  DRY_LEVEL,
  POLE1_LEVEL,
  POLE2_LEVEL,
  POLE3_LEVEL,
  POLE4_LEVEL,
  Q_VCA,
};

static const std::string rezCompModes[] = { "Uncompensated", "Bandpass 4P", "Alt Mode 1", "Alt Mode 2" };


static constexpr float mixerGain = 8.f;

struct PoleDancer final : ParticipantAdapter, Participant {

  enum ParamId {
    SOURCE_ONE_LEVEL_KNOB_PARAM,
    SOURCE_TWO_LEVEL_KNOB_PARAM,
    SOURCE_ONE_MOD_AMOUNT_KNOB_PARAM,
    SOURCE_TWO_MOD_AMOUNT_KNOB_PARAM,
    CUTOFF_KNOB_PARAM,
    RESONANCE_KNOB_PARAM,
    FILTER_VCA_KNOB_PARAM,
    SOURCE_ONE_VALUE_HIDDEN_PARAM,
    SOURCE_ONE_DOWN_BUTTON_PARAM,
    SOURCE_ONE_UP_BUTTON_PARAM,
    SOURCE_TWO_VALUE_HIDDEN_PARAM,
    SOURCE_TWO_DOWN_BUTTON_PARAM,
    SOURCE_TWO_UP_BUTTON_PARAM,
    REZ_COMP_VALUE_HIDDEN_PARAM,
    REZ_COMP_DOWN_BUTTON_PARAM,
    REZ_COMP_UP_BUTTON_PARAM,
    /*
    REZ_COMP_MODE0,
    REZ_COMP_MODE1,
    REZ_COMP_MODE2,
    REZ_COMP_MODE3,
    REZ_COMP_MODE4,
    REZ_COMP_MODE5,
    REZ_COMP_MODE6,
    REZ_COMP_MODE7,
    */
    PARAMS_LEN
  };
  enum InputId {
    SOURCE_ONE_LEVEL_INPUT,
    SOURCE_TWO_LEVEL_INPUT,
    SOURCE_ONE_MOD_AMOUNT_INPUT,
    SOURCE_TWO_MOD_AMOUNT_INPUT,
    POLE_MIX_INPUT,
    CUTOFF_INPUT,
    RESONANCE_INPUT,
    FILTER_VCA_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    OUTPUTS_LEN
  };
  enum LightId {
    SOURCE_ONE_LEVEL_CLIP_LIGHT,
    SOURCE_ONE_MOD_AMOUNT_CLIP_LIGHT,
    SOURCE_TWO_LEVEL_CLIP_LIGHT,
    SOURCE_TWO_MOD_AMOUNT_CLIP_LIGHT,
    CUTOFF_CLIP_LIGHT,
    RESONANCE_CLIP_LIGHT,
    FILTER_VCA_CLIP_LIGHT,
    SOURCE_ONE_DOWN_BUTTON_LIGHT,
    SOURCE_ONE_UP_BUTTON_LIGHT,
    SOURCE_TWO_DOWN_BUTTON_LIGHT,
    SOURCE_TWO_UP_BUTTON_LIGHT,
    REZ_COMP_DOWN_BUTTON_LIGHT,
    REZ_COMP_UP_BUTTON_LIGHT,
    ENUMS(RIGHT_EXPANDER_LIGHT, 3),
    REZ_COMP_LIGHT0,
    REZ_COMP_LIGHT1,
    REZ_COMP_LIGHT2,
    REZ_COMP_LIGHT3,
    REZ_COMP_LIGHT4,
    REZ_COMP_LIGHT5,
    REZ_COMP_LIGHT6,
    REZ_COMP_LIGHT7,
    LIGHTS_LEN
  };

  float sourceOneLevelClipTimer = 0.f;
  float sourceOneModAmountClipTimer = 0.f;
  float sourceTwoLevelClipTimer = 0.f;
  float sourceTwoModAmountClipTimer = 0.f;
  float cutoffClipTimer = 0.f;
  float resonanceClipTimer = 0.f;
  float filterVcaClipTimer = 0.f;

  std::string source1NameString;
  std::string source2NameString;
  std::string output1NameString;
  std::string output2NameString;
  std::string rezCompModeNameString;

  static constexpr int8_t sourceOneSelectMidiPrograms[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
  static constexpr int8_t sourceTwoSelectMidiPrograms[] = { 8, 9, 10, 11, 12, 13, 14, 15 };
  static constexpr int8_t rezModeSelectMidiPrograms[] = { 24, 25, 26, 27 };

  std::array<CvRoute,5> routes;

  PoleDancer() :
    source1NameString(invalidCardOutputName),
    source2NameString(invalidCardOutputName),
    output1NameString(invalidCardOutputName),
    output2NameString(invalidCardOutputName),
    routes{{
      {CUTOFF_KNOB_PARAM, CUTOFF_INPUT, VCF_CUTOFF, 10.f, &cutoffClipTimer, nullptr, CvOperation::Add},
      {SOURCE_ONE_LEVEL_KNOB_PARAM, SOURCE_ONE_LEVEL_INPUT, SOURCE_ONE_LEVEL, 10.f, &sourceOneLevelClipTimer, nullptr, CvOperation::Add},
      {SOURCE_TWO_LEVEL_KNOB_PARAM, SOURCE_TWO_LEVEL_INPUT, SOURCE_TWO_LEVEL, 10.f, &sourceTwoLevelClipTimer, nullptr, CvOperation::Add},
      {SOURCE_ONE_MOD_AMOUNT_KNOB_PARAM, SOURCE_ONE_MOD_AMOUNT_INPUT, SOURCE_ONE_MOD_AMOUNT, 10.f, &sourceOneModAmountClipTimer, nullptr, CvOperation::Add},
      {SOURCE_TWO_MOD_AMOUNT_KNOB_PARAM, SOURCE_TWO_MOD_AMOUNT_INPUT, SOURCE_TWO_MOD_AMOUNT, 10.f, &sourceTwoModAmountClipTimer, nullptr, CvOperation::Add}
    }} {

    setParticipant(this);
    setLightEnum(RIGHT_EXPANDER_LIGHT);

    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    // buttons for inputs select
    configButton(SOURCE_ONE_DOWN_BUTTON_PARAM, "Previous");
    configButton(SOURCE_ONE_UP_BUTTON_PARAM, "Next");
    configButton(SOURCE_TWO_DOWN_BUTTON_PARAM, "Previous");
    configButton(SOURCE_TWO_UP_BUTTON_PARAM, "Next");

    configButton(REZ_COMP_DOWN_BUTTON_PARAM, "Previous");
    configButton(REZ_COMP_UP_BUTTON_PARAM, "Next");

    configParam(SOURCE_ONE_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Source One Level", "%", 0.f, 100.f);
    configParam(SOURCE_ONE_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Source One Mod", "%", 0.f, 100.f);
    configParam(SOURCE_TWO_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Source Two Level", "%", 0.f, 100.f);
    configParam(SOURCE_TWO_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Source Two Mod", "%", 0.f, 100.f);

    configParam(CUTOFF_KNOB_PARAM, 0.f, 1.f, 1.f, "Cutoff", "V", 0.f, 10.f, -1.f);

    configParam(RESONANCE_KNOB_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
    configParam(FILTER_VCA_KNOB_PARAM, 0.f, 1.f, 0.5f, "Output VCA", "%", 0.f, 100.f);

    configInput(SOURCE_ONE_LEVEL_INPUT, "Source One Level");
    configInput(SOURCE_ONE_MOD_AMOUNT_INPUT, "Source One Mod Amount");
    configInput(SOURCE_TWO_LEVEL_INPUT, "Source Two Level");
    configInput(SOURCE_TWO_MOD_AMOUNT_INPUT, "Source Two Mod Amount");
    configInput(CUTOFF_INPUT, "Filter Cutoff");
    configInput(RESONANCE_INPUT, "Resonance");
    configInput(FILTER_VCA_INPUT, "Output VCA");
    configInput(POLE_MIX_INPUT, "Pole Mix");

    // no UI elements for these
    configSwitch(SOURCE_ONE_VALUE_HIDDEN_PARAM, 0.f, 7.f, 0.f, "Source One", {"0", "1", "2", "3", "4", "5", "6", "7"} );
    configSwitch(SOURCE_TWO_VALUE_HIDDEN_PARAM, 0.f, 7.f, 0.f, "Source Two", {"0", "1", "2", "3", "4", "5", "6", "7"} );
    configSwitch(REZ_COMP_VALUE_HIDDEN_PARAM, 0.f, 3.f, 0.f, "Rez Compensation", {"0", "1", "2", "3"} );
    source1NameString.reserve(16);
    source2NameString.reserve(16);
    output1NameString.reserve(16);
    output2NameString.reserve(16);
    rezCompModeNameString.reserve(16);
    rezCompModeNameString = rezCompModes[0];
  }

  /* Participant interface: return Module identifier */
  int64_t getModuleId() override {
    return getId();
  }


  void pullSamples(const rack::engine::Module::ProcessArgs &args, dsp::Frame<maxAudioChannels> &sharedFrame, int offset) override {

    float v;
    bool clipped;
    static constexpr float clipTime = 0.25f;

    processCvRoutes(routes.data(),
                    routes.size(),
                    clipTime,
                    offset,
                    sharedFrame.samples,
                    params.data(),
                    inputs.data());

    // qvca
    v = params[RESONANCE_KNOB_PARAM].getValue() + inputs[RESONANCE_INPUT].getVoltage() / 10.f;
    clipped = (v < 0.f) || (v > 1.f);
    if (clipped) {
        resonanceClipTimer = clipTime;
    }
    // Resonance slope & max will vary depending on Resonance Compensation mode
    // for consistency between modes, try to get oscillation to start around 80%.
    // From 80% then ramp up to the max allowable value.
    switch( int(params[ REZ_COMP_VALUE_HIDDEN_PARAM ].getValue() + 0.5f) ) {
    case 2: // modified 2P-bandpass
        // oscillation starts at 16%, max rez is at 33%
        // map 80% --> 16% and 100% --> 33%
        v = v < 0.80f ? 0.20f * v : 0.85f * v - 0.52f;
        sharedFrame.samples[offset + Q_VCA] = clamp(v, 0.f, 0.34f);
        break;
    case 3: // oddball comp
        // oscillation starts at 26%, max rez is at 50%
        // map 80% --> 26% and 100% --> 50%
        v = v < 0.80f ? 0.325f * v : 1.2f * v - 0.7f;
        sharedFrame.samples[offset + Q_VCA] = clamp(v, 0.f, 0.51f);
        break;
    default: // uncomp and 4P-bandpass
        // map 80% --> 70% and 100% --> 100%
        v = v < 0.80f ? 0.875f * v : 1.5f * v - 0.5f;
        sharedFrame.samples[offset + Q_VCA] = clamp(v, 0.f, 1.f);
        break;
    }



    // pole levels:
    // these are scaled by the Filter VCA, so get that first
    v = params[FILTER_VCA_KNOB_PARAM].getValue() + inputs[FILTER_VCA_INPUT].getVoltage() / 10.f;
    clipped = (v < 0.f) || (v > 1.f);
    float filterVcaGain = clamp(v, 0.f, 1.f);
    if (clipped) {
      filterVcaClipTimer = clipTime;
    }

    if (inputs[POLE_MIX_INPUT].isConnected()) {
      sharedFrame.samples[offset + DRY_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(0) / 10.f) * filterVcaGain;

      sharedFrame.samples[offset + POLE1_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(1) / 10.f) * filterVcaGain;

      sharedFrame.samples[offset + POLE2_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(2) / 10.f) * filterVcaGain;

      sharedFrame.samples[offset + POLE3_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(3) / 10.f) * filterVcaGain;

      sharedFrame.samples[offset + POLE4_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(4) / 10.f) * filterVcaGain;
    } else {
      sharedFrame.samples[offset + DRY_LEVEL] = 0.f;
      sharedFrame.samples[offset + POLE1_LEVEL] = 0.f;
      sharedFrame.samples[offset + POLE2_LEVEL] = 0.f;
      sharedFrame.samples[offset + POLE3_LEVEL] = 0.f;
      sharedFrame.samples[offset + POLE4_LEVEL] = filterVcaGain / mixerGain;
    }

  }


  bool pullMidi(const rack::engine::Module::ProcessArgs &args, uint32_t clockDivision, int midiChannel, midi::Message &midiMessage) override {

    // clipping
    const float lightTime = args.sampleTime * clockDivision;
    const float brightnessDeltaTime = 1 / lightTime;

    // for clipping lights just keep subtracting every cycle
    sourceOneLevelClipTimer -= lightTime;
    lights[SOURCE_ONE_LEVEL_CLIP_LIGHT].setBrightnessSmooth(sourceOneLevelClipTimer > 0.f, brightnessDeltaTime);

    sourceOneModAmountClipTimer -= lightTime;
    lights[SOURCE_ONE_MOD_AMOUNT_CLIP_LIGHT].setBrightnessSmooth(sourceOneModAmountClipTimer > 0.f, brightnessDeltaTime);

    sourceTwoLevelClipTimer -= lightTime;
    lights[SOURCE_TWO_LEVEL_CLIP_LIGHT].setBrightnessSmooth(sourceTwoLevelClipTimer > 0.f, brightnessDeltaTime);

    sourceTwoModAmountClipTimer -= lightTime;
    lights[SOURCE_TWO_MOD_AMOUNT_CLIP_LIGHT].setBrightnessSmooth(sourceTwoModAmountClipTimer > 0.f, brightnessDeltaTime);

    cutoffClipTimer -= lightTime;
    lights[CUTOFF_CLIP_LIGHT].setBrightnessSmooth(cutoffClipTimer > 0.f, brightnessDeltaTime);

    resonanceClipTimer -= lightTime;
    lights[RESONANCE_CLIP_LIGHT].setBrightnessSmooth(resonanceClipTimer > 0.f, brightnessDeltaTime);

    filterVcaClipTimer -= lightTime;
    lights[FILTER_VCA_CLIP_LIGHT].setBrightnessSmooth(filterVcaClipTimer > 0.f, brightnessDeltaTime);


    // light up any buttons-- processing of these is done immediately below
    lights[SOURCE_ONE_DOWN_BUTTON_LIGHT].setBrightness(params[SOURCE_ONE_DOWN_BUTTON_PARAM].getValue());
    lights[SOURCE_ONE_UP_BUTTON_LIGHT].setBrightness(params[SOURCE_ONE_UP_BUTTON_PARAM].getValue());
    lights[SOURCE_TWO_DOWN_BUTTON_LIGHT].setBrightness(params[SOURCE_TWO_DOWN_BUTTON_PARAM].getValue());
    lights[SOURCE_TWO_UP_BUTTON_LIGHT].setBrightness(params[SOURCE_TWO_UP_BUTTON_PARAM].getValue());
    lights[REZ_COMP_DOWN_BUTTON_LIGHT].setBrightness(params[REZ_COMP_DOWN_BUTTON_PARAM].getValue());
    lights[REZ_COMP_UP_BUTTON_LIGHT].setBrightness(params[REZ_COMP_UP_BUTTON_PARAM].getValue());


    // add/subtract the up/down buttons and set a string that
    // the UI can use.  There ought to be some todos here to
    // make/fix this.
    if (handleUpDownSelector(
          params[SOURCE_ONE_UP_BUTTON_PARAM],
          params[SOURCE_ONE_DOWN_BUTTON_PARAM],
          params[SOURCE_ONE_VALUE_HIDDEN_PARAM],
          7,
          [&](int i){ return *lifecycle.nameService->getNamePtr(source1Sources[i]); },
          source1NameString,
          sourceOneSelectMidiPrograms,
          midiMessage,
          midiChannel)) {
      return true;
    }

    if (handleUpDownSelector(
          params[SOURCE_TWO_UP_BUTTON_PARAM],
          params[SOURCE_TWO_DOWN_BUTTON_PARAM],
          params[SOURCE_TWO_VALUE_HIDDEN_PARAM],
          7,
          [&](int i){ return *lifecycle.nameService->getNamePtr(source2Sources[i]); },
          source2NameString,
          sourceTwoSelectMidiPrograms,
          midiMessage,
          midiChannel)) {
      return true;
    }

    if (handleUpDownSelector(
          params[REZ_COMP_UP_BUTTON_PARAM],
          params[REZ_COMP_DOWN_BUTTON_PARAM],
          params[REZ_COMP_VALUE_HIDDEN_PARAM],
          3,
          [&](int i){ return rezCompModes[i]; },
          rezCompModeNameString,
          rezModeSelectMidiPrograms,
          midiMessage,
          midiChannel)) {
      return true;
    }

    return false;
  }



  /** getCardHardwareId
   * return the hardware Id of the poledancer card
   */
  static constexpr uint8_t hardwareId = 0x06;
  uint8_t getHardwareId() const override {
    return hardwareId;
  }


  void onAttach() override {
    if (lifecycle.nameService == nullptr) {
      return;
    }
    auto *ptr1 = lifecycle.nameService->getNamePtr(lifecycle.slotNum * 2);
    auto *ptr2 = lifecycle.nameService->getNamePtr(lifecycle.slotNum * 2 + 1);
    output1NameString = ptr1 ? *ptr1 : invalidCardOutputName;
    output2NameString = ptr2 ? *ptr2 : invalidCardOutputName;

    int sourceOneIndex = static_cast<int>(params[SOURCE_ONE_VALUE_HIDDEN_PARAM].getValue());
    int sourceTwoIndex = static_cast<int>(params[SOURCE_TWO_VALUE_HIDDEN_PARAM].getValue());
    auto *ptrSource1 = lifecycle.nameService->getNamePtr( source1Sources[sourceOneIndex] );
    auto *ptrSource2 = lifecycle.nameService->getNamePtr( source2Sources[sourceTwoIndex] );
    source1NameString = ptrSource1 ? *ptrSource1 : invalidCardOutputName;
    source2NameString = ptrSource2 ? *ptrSource2 : invalidCardOutputName;

    // fake out the handleUpDownSelector() to force a MIDI message to be sent
    params[SOURCE_ONE_VALUE_HIDDEN_PARAM].setValue(
      params[SOURCE_ONE_VALUE_HIDDEN_PARAM].getValue() - 1.f);
    params[SOURCE_ONE_UP_BUTTON_PARAM].setValue(1.f);

    params[SOURCE_TWO_VALUE_HIDDEN_PARAM].setValue(
      params[SOURCE_TWO_VALUE_HIDDEN_PARAM].getValue() - 1.f);
    params[SOURCE_TWO_UP_BUTTON_PARAM].setValue(1.f);

    params[REZ_COMP_VALUE_HIDDEN_PARAM].setValue(
      params[REZ_COMP_VALUE_HIDDEN_PARAM].getValue() - 1.f);
    params[REZ_COMP_UP_BUTTON_PARAM].setValue(1.f);

  }


  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    return rootJ;
  }

  void dataFromJson(json_t* rootJ) override {
    int rezCompModeInt = static_cast<int>(params[ REZ_COMP_VALUE_HIDDEN_PARAM ].getValue());
    rezCompModeNameString = rezCompModes[ rezCompModeInt ];
    INFO("poledancer: setting comp mode to %s", rezCompModeNameString.c_str());
  }
};


struct PoleDancerWidget : ModuleWidget {
  PoleDancerWidget(PoleDancer* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/PoleDancer.svg")));

    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH / 2, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addParam(createParamCentered<ZPushButtonMediumLeft>(mm2px(Vec(6.461, 20.235)), module, PoleDancer::SOURCE_ONE_DOWN_BUTTON_PARAM));
    addParam(createParamCentered<ZPushButtonMediumRight>(mm2px(Vec(37.539, 20.235)), module, PoleDancer::SOURCE_ONE_UP_BUTTON_PARAM));
    addParam(createParamCentered<ZPushButtonMediumLeft>(mm2px(Vec(6.461, 70.437)), module, PoleDancer::SOURCE_TWO_DOWN_BUTTON_PARAM));
    addParam(createParamCentered<ZPushButtonMediumRight>(mm2px(Vec(37.539, 70.437)), module, PoleDancer::SOURCE_TWO_UP_BUTTON_PARAM));

    addParam(createParamCentered<ZPushButtonMediumDown>(mm2px(Vec(102.65, 68.437)), module, PoleDancer::REZ_COMP_DOWN_BUTTON_PARAM));
    addParam(createParamCentered<ZPushButtonMediumUp>(mm2px(Vec(93.596, 68.437)), module, PoleDancer::REZ_COMP_UP_BUTTON_PARAM));

    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(17.589, 32.734)), module, PoleDancer::SOURCE_ONE_LEVEL_KNOB_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(17.589, 52.517)), module, PoleDancer::SOURCE_ONE_MOD_AMOUNT_KNOB_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(17.589, 82.936)), module, PoleDancer::SOURCE_TWO_LEVEL_KNOB_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(17.589, 102.719)), module, PoleDancer::SOURCE_TWO_MOD_AMOUNT_KNOB_PARAM));
    auto* knob = createParamCentered<TurnsCountingKnob>(
      mm2px(Vec(71.0, 24.223)),
      module,
      PoleDancer::CUTOFF_KNOB_PARAM);
    knob->setTurns(10);
    addParam(knob);

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(64.692, 57.752)), module, PoleDancer::RESONANCE_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(70.975, 88.858)), module, PoleDancer::FILTER_VCA_KNOB_PARAM));


    addInput(createInputCentered<BNCPort>(mm2px(Vec(30.254, 32.734)), module, PoleDancer::SOURCE_ONE_LEVEL_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(30.254, 52.517)), module, PoleDancer::SOURCE_ONE_MOD_AMOUNT_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(30.254, 82.936)), module, PoleDancer::SOURCE_TWO_LEVEL_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(30.254, 102.719)), module, PoleDancer::SOURCE_TWO_MOD_AMOUNT_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(71.0, 38.223)), module, PoleDancer::CUTOFF_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(77.296, 58.0)), module, PoleDancer::RESONANCE_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(71.0, 102.719)), module, PoleDancer::FILTER_VCA_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(49.0, 102.719)), module, PoleDancer::POLE_MIX_INPUT));

    addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(4.488, 122.5)), module, PoleDancer::RIGHT_EXPANDER_LIGHT));

    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(78.7, 32.694)), module, PoleDancer::CUTOFF_CLIP_LIGHT));

    addChild(createLightCentered<SmallLight<ZoxAmberLight>>(mm2px(Vec(91.323, 24.677)), module, PoleDancer::REZ_COMP_LIGHT0));
    addChild(createLightCentered<SmallLight<ZoxAmberLight>>(mm2px(Vec(91.323, 29.676)), module, PoleDancer::REZ_COMP_LIGHT1));
    addChild(createLightCentered<SmallLight<ZoxAmberLight>>(mm2px(Vec(91.323, 34.676)), module, PoleDancer::REZ_COMP_LIGHT2));
    addChild(createLightCentered<SmallLight<ZoxAmberLight>>(mm2px(Vec(91.323, 39.676)), module, PoleDancer::REZ_COMP_LIGHT3));
    addChild(createLightCentered<SmallLight<ZoxAmberLight>>(mm2px(Vec(91.323, 44.676)), module, PoleDancer::REZ_COMP_LIGHT4));
    addChild(createLightCentered<SmallLight<ZoxAmberLight>>(mm2px(Vec(91.323, 49.676)), module, PoleDancer::REZ_COMP_LIGHT5));
    addChild(createLightCentered<SmallLight<ZoxAmberLight>>(mm2px(Vec(91.323, 54.676)), module, PoleDancer::REZ_COMP_LIGHT6));
    addChild(createLightCentered<SmallLight<ZoxAmberLight>>(mm2px(Vec(91.323, 59.676)), module, PoleDancer::REZ_COMP_LIGHT7));


    // Text fields
    source1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(10.806, 18.497)));
    source1NameTextField->setNumChars(13);
    source1NameTextField->box.size = mm2px(Vec(18.388, 3.636));
    source1NameTextField->setText(module ? &module->source1NameString : NULL);
    addChild(source1NameTextField);

    source2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(10.806, 68.699)));
    source2NameTextField->setNumChars(13);
    source2NameTextField->box.size = mm2px(Vec(18.388, 3.636));
    source2NameTextField->setText(module ? &module->source2NameString : NULL);
    addChild(source2NameTextField);

    output1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(59.616, 76.044)));
    output1NameTextField->setNumChars(13);
    output1NameTextField->box.size = mm2px(Vec(20.0, 3.636));
    output1NameTextField->setText(module ? &module->output1NameString : NULL);
    addChild(output1NameTextField);

    output2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(85.825, 101.519)));
    output2NameTextField->setNumChars(13);
    output2NameTextField->box.size = mm2px(Vec(20.0, 3.636));
    output2NameTextField->setText(module ? &module->output2NameString : NULL);
    addChild(output2NameTextField);

  }


  CardTextDisplay *source1NameTextField;
  CardTextDisplay *source2NameTextField;
  CardTextDisplay *output1NameTextField;
  CardTextDisplay *output2NameTextField;
};


constexpr int8_t zox::PoleDancer::sourceOneSelectMidiPrograms[];
constexpr int8_t zox::PoleDancer::sourceTwoSelectMidiPrograms[];
constexpr int8_t zox::PoleDancer::rezModeSelectMidiPrograms[];


} // namespace zox

Model* modelPoleDancer = createModel<zox::PoleDancer, zox::PoleDancerWidget>("PoleDancer");
