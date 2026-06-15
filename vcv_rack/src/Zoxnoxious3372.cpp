#include "plugin.hpp"
#include "zcomponentlib.hpp"
#include "constants.hpp"
#include "modulehelpers.hpp"
#include "ParticipantAdapter.hpp"
#include "TurnsCountingKnob.hpp"

namespace zox {

// relative cv channel for the control voltage being sent over the wire
enum cvChannel {
    NOISE_LEVEL = 0, // = 0 enforces the purpose though even if "= 0" is implicit
    OUTPUT_PAN,
    RESONANCE,
    FILTER_VCA,
    CUTOFF,
    SOURCE_ONE_LEVEL,
    SOURCE_TWO_LEVEL,
    MOD_AMOUNT
};



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
static constexpr int source1Sources[] = { 0, 1, 3, 4, 7, 8, 11, 12 };

// card6 out1
// card5 out2
// card4 out1
// card3 out2
// card2 out2
// card2 out1
// card1 out2
// card1 out1
static constexpr int source2Sources[] = { 0, 1, 2, 3, 5, 6, 9, 10 };


struct Zoxnoxious3372 final : ParticipantAdapter, Participant {
  enum ParamId {
    CUTOFF_KNOB_PARAM,
    OUTPUT_PAN_KNOB_PARAM,
    MOD_AMOUNT_KNOB_PARAM,
    SOURCE_ONE_LEVEL_KNOB_PARAM,
    SOURCE_TWO_LEVEL_KNOB_PARAM,
    RESONANCE_KNOB_PARAM,
    FILTER_VCA_KNOB_PARAM,
    VCA_MOD_SWITCH_PARAM,
    PAN_MOD_SWITCH_PARAM,
    NOISE_KNOB_PARAM,
    FILTER_MOD_SWITCH_PARAM,
    SOURCE_ONE_VALUE_HIDDEN_PARAM,
    SOURCE_ONE_DOWN_BUTTON_PARAM,
    SOURCE_ONE_UP_BUTTON_PARAM,
    SOURCE_TWO_VALUE_HIDDEN_PARAM,
    SOURCE_TWO_DOWN_BUTTON_PARAM,
    SOURCE_TWO_UP_BUTTON_PARAM,
    REZ_MOD_SWITCH_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    CUTOFF_INPUT,
    OUTPUT_PAN_INPUT,
    NOISE_LEVEL_INPUT,
    MOD_AMOUNT_INPUT,
    SOURCE_ONE_LEVEL_INPUT,
    SOURCE_TWO_LEVEL_INPUT,
    RESONANCE_INPUT,
    FILTER_VCA_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    OUTPUTS_LEN
  };
  enum LightId {
    MOD_AMOUNT_CLIP_LIGHT,
    SOURCE_ONE_LEVEL_CLIP_LIGHT,
    SOURCE_TWO_LEVEL_CLIP_LIGHT,
    OUTPUT_PAN_CLIP_LIGHT,
    CUTOFF_CLIP_LIGHT,
    RESONANCE_CLIP_LIGHT,
    FILTER_VCA_CLIP_LIGHT,
    OUTPUT_VCA_CLIP_LIGHT,
    VCA_MOD_ENABLE_LIGHT,
    FILTER_MOD_ENABLE_LIGHT,
    REZ_MOD_ENABLE_LIGHT,
    PAN_MOD_ENABLE_LIGHT,
    SOURCE_ONE_DOWN_BUTTON_LIGHT,
    SOURCE_ONE_UP_BUTTON_LIGHT,
    SOURCE_TWO_DOWN_BUTTON_LIGHT,
    SOURCE_TWO_UP_BUTTON_LIGHT,
    ENUMS(LEFT_EXPANDER_LIGHT, 3),
    ENUMS(RIGHT_EXPANDER_LIGHT, 3),
    LIGHTS_LEN
  };


  float noiseClipTimer = 0.f;
  float modAmountClipTimer = 0.f;
  float sourceOneLevelClipTimer = 0.f;
  float sourceTwoLevelClipTimer = 0.f;
  float outputPanClipTimer = 0.f;
  float cutoffClipTimer = 0.f;
  float resonanceClipTimer = 0.f;
  float outputVcaClipTimer = 0.f;

  std::string source1NameString;
  std::string source2NameString;
  std::string output1NameString;
  std::string output2NameString;

  static constexpr int8_t sourceOneSelectMidiPrograms[] = { 4, 5, 6, 7, 8, 9, 10, 11 };
  static constexpr int8_t sourceTwoSelectMidiPrograms[] = { 12, 13, 14, 15, 16, 17, 18, 19 };

  // index corresponds on both vectors for tracking button pushes and outgoing MIDI msg
  static const std::vector<ButtonMapping<Zoxnoxious3372> > buttonMappings;
  ButtonMidiController<Zoxnoxious3372> buttonMidiController;

  std::array<CvRoute,8> routes;

  Zoxnoxious3372() :
    source1NameString(invalidCardOutputName), source2NameString(invalidCardOutputName),
    output1NameString(invalidCardOutputName), output2NameString(invalidCardOutputName),
    buttonMidiController(buttonMappings),
    routes{{
      {NOISE_KNOB_PARAM, NOISE_LEVEL_INPUT, NOISE_LEVEL, 10.f, &noiseClipTimer, nullptr, CvOperation::Add},
      {OUTPUT_PAN_KNOB_PARAM, OUTPUT_PAN_INPUT, OUTPUT_PAN, 10.f, &outputPanClipTimer, nullptr, CvOperation::Add},
      {RESONANCE_KNOB_PARAM, RESONANCE_INPUT, RESONANCE, 10.f, &resonanceClipTimer, dualLinearSwitch0_8, CvOperation::Add},
      {FILTER_VCA_KNOB_PARAM, FILTER_VCA_INPUT, FILTER_VCA, 10.f, &outputVcaClipTimer, nullptr, CvOperation::Add},
      {CUTOFF_KNOB_PARAM, CUTOFF_INPUT, CUTOFF, 10.f, &cutoffClipTimer, nullptr, CvOperation::Add},
      {SOURCE_ONE_LEVEL_KNOB_PARAM, SOURCE_ONE_LEVEL_INPUT, SOURCE_ONE_LEVEL, 10.f, &sourceOneLevelClipTimer, nullptr, CvOperation::Add},
      {SOURCE_TWO_LEVEL_KNOB_PARAM, SOURCE_TWO_LEVEL_INPUT, SOURCE_TWO_LEVEL, 10.f, &sourceTwoLevelClipTimer, nullptr, CvOperation::Add},
      {MOD_AMOUNT_KNOB_PARAM, MOD_AMOUNT_INPUT, MOD_AMOUNT, 10.f, &modAmountClipTimer, nullptr, CvOperation::Add} }} {

    setParticipant(this);
    setLightEnum(RIGHT_EXPANDER_LIGHT);


    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configButton(SOURCE_ONE_DOWN_BUTTON_PARAM, "Previous");
    configButton(SOURCE_ONE_UP_BUTTON_PARAM, "Next");
    configButton(SOURCE_TWO_DOWN_BUTTON_PARAM, "Previous");
    configButton(SOURCE_TWO_UP_BUTTON_PARAM, "Next");

    configParam(MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Modulation Amount", "%", 0.f, 100.f);
    configParam(NOISE_KNOB_PARAM, 0.f, 1.f, 0.f, "White Noise", "%", 0.f, 100.f);
    configParam(SOURCE_ONE_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Level", "%", 0.f, 100.f);
    configParam(SOURCE_TWO_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Level", "%", 0.f, 100.f);
    configParam(OUTPUT_PAN_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pan", "%", 0.f, 200.f, -100.f);
    configParam(CUTOFF_KNOB_PARAM, 0.f, 1.f, 1.f, "Cutoff", " V", 0.f, 10.f, -1.f);
    configParam(RESONANCE_KNOB_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
    configParam(FILTER_VCA_KNOB_PARAM, 0.f, 1.f, 0.f, "Level", "%", 0.f, 100.f);

    configSwitch(FILTER_MOD_SWITCH_PARAM, 0.f, 1.f, 0.f, "Filter Mod", {"Off", "On"});
    configSwitch(VCA_MOD_SWITCH_PARAM, 0.f, 1.f, 0.f, "VCA Mod", {"Off", "On"});
    configSwitch(REZ_MOD_SWITCH_PARAM, 0.f, 1.f, 0.f, "Rez Mod", {"Off", "On"});
    configSwitch(PAN_MOD_SWITCH_PARAM, 0.f, 1.f, 0.f, "Pan Mod", {"Off", "On"});

    configInput(MOD_AMOUNT_INPUT, "Modulation Amount");
    configInput(NOISE_LEVEL_INPUT, "Noise Level");
    configInput(SOURCE_ONE_LEVEL_INPUT, "Source One Level");
    configInput(SOURCE_TWO_LEVEL_INPUT, "Source Two Level");
    configInput(OUTPUT_PAN_INPUT, "Pan");
    configInput(CUTOFF_INPUT, "Cutoff");
    configInput(RESONANCE_INPUT, "Resonance");
    configInput(FILTER_VCA_INPUT, "Output Level");

    // no UI elements for these
    configSwitch(SOURCE_ONE_VALUE_HIDDEN_PARAM, 0.f, 7.f, 0.f, "Source One", {"0", "1", "2", "3", "4", "5", "6", "7"} );
    configSwitch(SOURCE_TWO_VALUE_HIDDEN_PARAM, 0.f, 7.f, 0.f, "Source Two", {"0", "1", "2", "3", "4", "5", "6", "7"} );

  }

  void pullSamples(const rack::engine::Module::ProcessArgs &args, dsp::Frame<maxAudioChannels> &sharedFrame, int offset) override {
    // clipping
    static constexpr float clipTime = 0.25f;

    processCvRoutes(routes.data(),
                    routes.size(),
                    clipTime,
                    offset,
                    sharedFrame.samples,
                    params.data(),
                    inputs.data());

  }

  bool pullMidi(const rack::engine::Module::ProcessArgs &args, uint32_t clockDivision, int midiChannel, midi::Message &midiMessage) override {
    const float lightTime = args.sampleTime * clockDivision;
    const float brightnessDeltaTime = 1 / lightTime;

    // lights first then MIDI messages
    modAmountClipTimer -= lightTime;
    lights[MOD_AMOUNT_CLIP_LIGHT].setBrightnessSmooth(modAmountClipTimer > 0.f, brightnessDeltaTime);

    sourceOneLevelClipTimer -= lightTime;
    lights[SOURCE_ONE_LEVEL_CLIP_LIGHT].setBrightnessSmooth(sourceOneLevelClipTimer > 0.f, brightnessDeltaTime);

    sourceTwoLevelClipTimer -= lightTime;
    lights[SOURCE_TWO_LEVEL_CLIP_LIGHT].setBrightnessSmooth(sourceTwoLevelClipTimer > 0.f, brightnessDeltaTime);

    outputPanClipTimer -= lightTime;
    lights[OUTPUT_PAN_CLIP_LIGHT].setBrightnessSmooth(outputPanClipTimer > 0.f, brightnessDeltaTime);

    cutoffClipTimer -= lightTime;
    lights[CUTOFF_CLIP_LIGHT].setBrightnessSmooth(cutoffClipTimer > 0.f, brightnessDeltaTime);

    resonanceClipTimer -= lightTime;
    lights[RESONANCE_CLIP_LIGHT].setBrightnessSmooth(resonanceClipTimer > 0.f, brightnessDeltaTime);

    outputVcaClipTimer -= lightTime;
    lights[OUTPUT_VCA_CLIP_LIGHT].setBrightnessSmooth(outputVcaClipTimer > 0.f, brightnessDeltaTime);

    lights[SOURCE_ONE_DOWN_BUTTON_LIGHT].setBrightness(params[SOURCE_ONE_DOWN_BUTTON_PARAM].getValue());
    lights[SOURCE_ONE_UP_BUTTON_LIGHT].setBrightness(params[SOURCE_ONE_UP_BUTTON_PARAM].getValue());
    lights[SOURCE_TWO_DOWN_BUTTON_LIGHT].setBrightness(params[SOURCE_TWO_DOWN_BUTTON_PARAM].getValue());
    lights[SOURCE_TWO_UP_BUTTON_LIGHT].setBrightness(params[SOURCE_TWO_UP_BUTTON_PARAM].getValue());

    buttonMidiController.updateLights(this);
    if (buttonMidiController.process(this, midiChannel, midiMessage)) {
      return true;
    }

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

    return false;
  }


  /** getCardHardwareId
   * return the hardware Id of the 3340 card
   */
  static constexpr uint8_t hardwareId = 0x03;
  uint8_t getHardwareId() const override {
    return hardwareId;
  }

  /* Participant interface */
  int64_t getModuleId() override {
    return getId();
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
  }


  bool pullGraphInfo(ParticipantGraphInfo& info) override {
    info.moduleId = getId();
    info.hardwareId = hardwareId;

    int sourceOneIndex = static_cast<int>(params[SOURCE_ONE_VALUE_HIDDEN_PARAM].getValue());
    int sourceOneSource = source1Sources[sourceOneIndex];

    int sourceTwoIndex = static_cast<int>(params[SOURCE_TWO_VALUE_HIDDEN_PARAM].getValue());
    int sourceTwoSource = source2Sources[sourceTwoIndex];

    info.source1.valid = true;
    info.source1.slotNum = sourceOneSource / 2;
    info.source1.port = (sourceOneSource % 2 == 0) ? GraphPort::A : GraphPort::B;
    info.source2.valid = true;
    info.source2.slotNum = sourceTwoSource / 2;
    info.source2.port = (sourceTwoSource % 2 == 0) ? GraphPort::A : GraphPort::B;
    return true;
  }
};



struct Zoxnoxious3372Widget : ModuleWidget {
    Zoxnoxious3372Widget(Zoxnoxious3372* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Zoxnoxious3372.svg")));

        addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<ZPushButtonSmallLeft>(mm2px(Vec(14.198, 25.135)), module, Zoxnoxious3372::SOURCE_ONE_DOWN_BUTTON_PARAM));
        addParam(createParamCentered<ZPushButtonSmallRight>(mm2px(Vec(21.487, 25.135)), module, Zoxnoxious3372::SOURCE_ONE_UP_BUTTON_PARAM));
        addParam(createParamCentered<ZPushButtonSmallLeft>(mm2px(Vec(14.198, 66.266)), module, Zoxnoxious3372::SOURCE_TWO_DOWN_BUTTON_PARAM));
        addParam(createParamCentered<ZPushButtonSmallRight>(mm2px(Vec(21.487, 66.266)), module, Zoxnoxious3372::SOURCE_TWO_UP_BUTTON_PARAM));

        addParam(createParamCentered<ZoxMediumKnob>(mm2px(Vec(46.976, 28.787)), module, Zoxnoxious3372::MOD_AMOUNT_KNOB_PARAM));
        addParam(createParamCentered<ZoxMediumKnob>(mm2px(Vec(15.547, 105.453)), module, Zoxnoxious3372::NOISE_KNOB_PARAM));
        addParam(createParamCentered<ZoxMediumKnob>(mm2px(Vec(15.547, 37.648)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_KNOB_PARAM));
        addParam(createParamCentered<ZoxMediumKnob>(mm2px(Vec(15.546, 78.977)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_KNOB_PARAM));
        addParam(createParamCentered<ZoxMediumKnob>(mm2px(Vec(46.948, 105.233)), module, Zoxnoxious3372::OUTPUT_PAN_KNOB_PARAM));
        auto* knob = createParamCentered<TurnsCountingKnob>(
          mm2px(Vec(84.344, 27.787)),
          module,
          Zoxnoxious3372::CUTOFF_KNOB_PARAM);
        knob->setTurns(10);
        addParam(knob);

        addParam(createParamCentered<ZoxMediumKnob>(mm2px(Vec(78.418, 64.5)), module, Zoxnoxious3372::RESONANCE_KNOB_PARAM));
        addParam(createParamCentered<ZoxMediumKnob>(mm2px(Vec(78.706, 92.611)), module, Zoxnoxious3372::FILTER_VCA_KNOB_PARAM));

        addParam(createLightParamCentered<ZPushButtonMediumStatefulLightLatch<SmallSimpleLight<ZoxAmberLight>>>(mm2px(Vec(50.857, 45.419)), module, Zoxnoxious3372::FILTER_MOD_SWITCH_PARAM, Zoxnoxious3372::FILTER_MOD_ENABLE_LIGHT));
        addParam(createLightParamCentered<ZPushButtonMediumStatefulLightLatch<SmallSimpleLight<ZoxAmberLight>>>(mm2px(Vec(50.857, 56.723)), module, Zoxnoxious3372::REZ_MOD_SWITCH_PARAM, Zoxnoxious3372::REZ_MOD_ENABLE_LIGHT));
        addParam(createLightParamCentered<ZPushButtonMediumStatefulLightLatch<SmallSimpleLight<ZoxAmberLight>>>(mm2px(Vec(50.857, 68.026)), module, Zoxnoxious3372::VCA_MOD_SWITCH_PARAM, Zoxnoxious3372::VCA_MOD_ENABLE_LIGHT));
        addParam(createLightParamCentered<ZPushButtonMediumStatefulLightLatch<SmallSimpleLight<ZoxAmberLight>>>(mm2px(Vec(50.857, 79.326)), module, Zoxnoxious3372::PAN_MOD_SWITCH_PARAM, Zoxnoxious3372::PAN_MOD_ENABLE_LIGHT));

        addInput(createInputCentered<BNCPort>(mm2px(Vec(59.64, 28.787)), module, Zoxnoxious3372::MOD_AMOUNT_INPUT));
        addInput(createInputCentered<BNCPort>(mm2px(Vec(28.211, 105.426)), module, Zoxnoxious3372::NOISE_LEVEL_INPUT));
        addInput(createInputCentered<BNCPort>(mm2px(Vec(28.211, 37.648)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_INPUT));
        addInput(createInputCentered<BNCPort>(mm2px(Vec(28.21, 78.977)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_INPUT));
        addInput(createInputCentered<BNCPort>(mm2px(Vec(59.64, 105.233)), module, Zoxnoxious3372::OUTPUT_PAN_INPUT));
        addInput(createInputCentered<BNCPort>(mm2px(Vec(84.344, 44.648)), module, Zoxnoxious3372::CUTOFF_INPUT));
        addInput(createInputCentered<BNCPort>(mm2px(Vec(91.418, 64.5)), module, Zoxnoxious3372::RESONANCE_INPUT));
        addInput(createInputCentered<BNCPort>(mm2px(Vec(91.68, 92.611)), module, Zoxnoxious3372::FILTER_VCA_INPUT));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(92.043, 38.725)), module, Zoxnoxious3372::CUTOFF_CLIP_LIGHT));

        addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(3.794, 121.706)), module, Zoxnoxious3372::RIGHT_EXPANDER_LIGHT));

        source1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(6.819, 18.415)));
        source1NameTextField->setNumChars(13);
        source1NameTextField->box.size = mm2px(Vec(18.0, 3.636));
        source1NameTextField->setText(module ? &module->source1NameString : NULL);
        addChild(source1NameTextField);

        source2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(6.819, 59.546)));
        source2NameTextField->setNumChars(13);
        source2NameTextField->box.size = mm2px(Vec(18.0, 3.636));
        source2NameTextField->setText(module ? &module->source2NameString : NULL);
        addChild(source2NameTextField);

        // mm2px(Vec(18.0, 3.636))
        output1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(75.6, 105.0)));
        output1NameTextField->setNumChars(13);
        output1NameTextField->box.size = mm2px(Vec(18.0, 3.636));
        output1NameTextField->setText(module ? &module->output1NameString : NULL);
        addChild(output1NameTextField);


        // mm2px(Vec(18.0, 3.636))
        output2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(75.6, 109.748)));
        output2NameTextField->setNumChars(13);
        output2NameTextField->box.size = mm2px(Vec(18.0, 3.636));
        output2NameTextField->setText(module ? &module->output2NameString : NULL);
        addChild(output2NameTextField);
    }

    CardTextDisplay *source1NameTextField;
    CardTextDisplay *source2NameTextField;
    CardTextDisplay *output1NameTextField;
    CardTextDisplay *output2NameTextField;

};

const std::vector<ButtonMapping<Zoxnoxious3372> > Zoxnoxious3372::buttonMappings = {
  { FILTER_MOD_SWITCH_PARAM, FILTER_MOD_ENABLE_LIGHT, {0,1} },
  { VCA_MOD_SWITCH_PARAM, VCA_MOD_ENABLE_LIGHT, {2,3} },
  { REZ_MOD_SWITCH_PARAM, REZ_MOD_ENABLE_LIGHT, {20, 21} },
  { PAN_MOD_SWITCH_PARAM, PAN_MOD_ENABLE_LIGHT, {22, 23} }
};

constexpr int8_t zox::Zoxnoxious3372::sourceOneSelectMidiPrograms[];
constexpr int8_t zox::Zoxnoxious3372::sourceTwoSelectMidiPrograms[];


} // namespace zox

Model* modelZoxnoxious3372 = createModel<zox::Zoxnoxious3372, zox::Zoxnoxious3372Widget>("Zoxnoxious3372");
