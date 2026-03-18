#include "plugin.hpp"
#include "zcomponentlib.hpp"
#include "common.hpp"
#include "ParticipantAdapter.hpp"

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
  std::vector<ButtonState> buttonStates;
  ButtonMidiController<Zoxnoxious3372> buttonMidiController;

  std::array<CvRoute,8> routes;

  Zoxnoxious3372() :
    source1NameString(invalidCardOutputName), source2NameString(invalidCardOutputName),
    output1NameString(invalidCardOutputName), output2NameString(invalidCardOutputName),
    buttonStates(buttonMappings.size()),
    buttonMidiController(buttonMappings),
    routes{{
      {NOISE_KNOB_PARAM, NOISE_LEVEL_INPUT, NOISE_LEVEL, 10.f, &noiseClipTimer, nullptr},
      {OUTPUT_PAN_KNOB_PARAM, OUTPUT_PAN_INPUT, OUTPUT_PAN, 10.f, &outputPanClipTimer, nullptr},
      {RESONANCE_KNOB_PARAM, RESONANCE_INPUT, RESONANCE, 10.f, &resonanceClipTimer, dualLinearSwitch0_8},
      {FILTER_VCA_KNOB_PARAM, FILTER_VCA_INPUT, FILTER_VCA, 10.f, &outputVcaClipTimer, nullptr},
      {CUTOFF_KNOB_PARAM, CUTOFF_INPUT, CUTOFF, 10.f, &cutoffClipTimer, nullptr},
      {SOURCE_ONE_LEVEL_KNOB_PARAM, SOURCE_ONE_LEVEL_INPUT, SOURCE_ONE_LEVEL, 10.f, &sourceOneLevelClipTimer, nullptr},
      {SOURCE_TWO_LEVEL_KNOB_PARAM, SOURCE_TWO_LEVEL_INPUT, SOURCE_TWO_LEVEL, 10.f, &sourceTwoLevelClipTimer, nullptr},
      {MOD_AMOUNT_KNOB_PARAM, MOD_AMOUNT_INPUT, MOD_AMOUNT, 10.f, &modAmountClipTimer, nullptr} }} {

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
  }

};


struct Zoxnoxious3372Widget : ModuleWidget {
    Zoxnoxious3372Widget(Zoxnoxious3372* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Zoxnoxious3372.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(10.604, 21.598)), module, Zoxnoxious3372::SOURCE_ONE_DOWN_BUTTON_PARAM, Zoxnoxious3372::SOURCE_ONE_DOWN_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(21.965, 21.598)), module, Zoxnoxious3372::SOURCE_ONE_UP_BUTTON_PARAM, Zoxnoxious3372::SOURCE_ONE_UP_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(10.796, 71.655)), module, Zoxnoxious3372::SOURCE_TWO_DOWN_BUTTON_PARAM, Zoxnoxious3372::SOURCE_TWO_DOWN_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(22.157, 71.655)), module, Zoxnoxious3372::SOURCE_TWO_UP_BUTTON_PARAM, Zoxnoxious3372::SOURCE_TWO_UP_BUTTON_LIGHT));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(38.9018, 26.0712)), module, Zoxnoxious3372::MOD_AMOUNT_KNOB_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(23.4843, 117.3152)), module, Zoxnoxious3372::NOISE_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(16.4844, 37.8604)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(16.4767, 88.2485)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(54.3714, 103.3061)), module, Zoxnoxious3372::OUTPUT_PAN_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.401623, 69.93885)), module, Zoxnoxious3372::CUTOFF_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(54.241844, 70.2092)), module, Zoxnoxious3372::RESONANCE_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(71.198502, 70.2092)), module, Zoxnoxious3372::FILTER_VCA_KNOB_PARAM));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(56.341805, 27.8295)), module, Zoxnoxious3372::FILTER_MOD_SWITCH_PARAM, Zoxnoxious3372::FILTER_MOD_ENABLE_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(56.341805, 44.677322)), module, Zoxnoxious3372::REZ_MOD_SWITCH_PARAM, Zoxnoxious3372::REZ_MOD_ENABLE_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(69.608521, 27.829542)), module, Zoxnoxious3372::VCA_MOD_SWITCH_PARAM, Zoxnoxious3372::VCA_MOD_ENABLE_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(69.608521, 44.677322)), module, Zoxnoxious3372::PAN_MOD_SWITCH_PARAM, Zoxnoxious3372::PAN_MOD_ENABLE_LIGHT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.9018, 39.4336)), module, Zoxnoxious3372::MOD_AMOUNT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.1233, 117.3152)), module, Zoxnoxious3372::NOISE_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.4844, 51.2228)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.4767, 101.6110)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(54.3714, 116.6685)), module, Zoxnoxious3372::OUTPUT_PAN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.401623, 83.571655)), module, Zoxnoxious3372::CUTOFF_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(54.241844, 83.571655)), module, Zoxnoxious3372::RESONANCE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71.198502, 83.571655)), module, Zoxnoxious3372::FILTER_VCA_INPUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(42.8644, 32.0711)), module, Zoxnoxious3372::MOD_AMOUNT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.4471, 43.8603)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.2754, 94.2484)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(58.1394, 109.3060)), module, Zoxnoxious3372::OUTPUT_PAN_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(41.364269, 76.2091)), module, Zoxnoxious3372::CUTOFF_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(58.204483, 76.2091)), module, Zoxnoxious3372::RESONANCE_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(75.16114, 76.2091)), module, Zoxnoxious3372::FILTER_VCA_CLIP_LIGHT));

        addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(2.02, 8.219)), module, Zoxnoxious3372::LEFT_EXPANDER_LIGHT));
        addChild(createLightCentered<TriangleRightLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(79.507, 8.219)), module, Zoxnoxious3372::RIGHT_EXPANDER_LIGHT));

        source1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(6.8842, 13.838)));
        source1NameTextField->box.size = mm2px(Vec(18.0, 3.636));
        source1NameTextField->setText(module ? &module->source1NameString : NULL);
        addChild(source1NameTextField);

        source2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(7.477, 63.895)));
        source2NameTextField->box.size = mm2px(Vec(18.0, 3.636));
        source2NameTextField->setText(module ? &module->source2NameString : NULL);
        addChild(source2NameTextField);

        // mm2px(Vec(18.0, 3.636))
        output1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(32.619, 111.088)));
        output1NameTextField->box.size = mm2px(Vec(18.0, 3.636));
        output1NameTextField->setText(module ? &module->output1NameString : NULL);
        addChild(output1NameTextField);


        // mm2px(Vec(18.0, 3.636))
        output2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(58.827, 111.088)));
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
