#include "plugin.hpp"
#include "common.hpp"
#include "zcomponentlib.hpp"
#include "TurnsCountingKnob.hpp"
#include "ParticipantAdapter.hpp"

namespace zox {
  

// relative cv channel for the control voltage being sent over the wire
enum cvChannel {
    FREQ_CHANNEL = 0,
    SYNC_PHASE_CHANNEL,
    MIX1_PULSE_VCA_CHANNEL,
    EXT_MOD_AMOUNT_CHANNEL,
    MIX1_TRIANGLE_VCA_CHANNEL,
    MIX1_SAW_VCA_CHANNEL,
    PULSE_WIDTH_CHANNEL,
    LINEAR_CHANNEL
};

struct Zoxnoxious3340 final : ParticipantAdapter, Participant {

  enum ParamId {
    FREQ_KNOB_PARAM,
    SYNC_PHASE_KNOB_PARAM,
    MIX1_PULSE_KNOB_PARAM,
    EXT_MOD_AMOUNT_KNOB_PARAM,
    MIX1_TRIANGLE_KNOB_PARAM,
    MIX1_SAW_KNOB_PARAM,
    PULSE_WIDTH_KNOB_PARAM,
    LINEAR_KNOB_PARAM,
    SYNC_POS_BUTTON_PARAM,
    EXT_MOD_SELECT_SWITCH_UP_PARAM,
    EXT_MOD_SELECT_SWITCH_DOWN_PARAM,
    MIX2_PULSE_BUTTON_PARAM,
    EXT_MOD_PWM_BUTTON_PARAM,
    EXP_FM_BUTTON_PARAM,
    LINEAR_FM_BUTTON_PARAM,
    MIX2_SAW_BUTTON_PARAM,
    SYNC_NEG_BUTTON_PARAM,
    SYNC_HARD_BUTTON_PARAM,
    SYNC_SOFT_BUTTON_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    FREQ_INPUT,
    SYNC_PHASE_INPUT,
    MIX1_PULSE_VCA_INPUT,
    EXT_MOD_AMOUNT_INPUT,
    MIX1_TRIANGLE_VCA_INPUT,
    MIX1_SAW_VCA_INPUT,
    PULSE_WIDTH_INPUT,
    LINEAR_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    OUTPUTS_LEN
  };
  enum LightId {
    SYNC_POS_BUTTON_LIGHT,
    EXT_MOD_SELECT_SWITCH_UP_LIGHT,
    EXT_MOD_SELECT_SWITCH_DOWN_LIGHT,
    MIX2_PULSE_BUTTON_LIGHT,
    EXT_MOD_PWM_BUTTON_LIGHT,
    EXP_FM_BUTTON_LIGHT,
    LINEAR_FM_BUTTON_LIGHT,
    MIX2_SAW_BUTTON_LIGHT,
    SYNC_NEG_BUTTON_LIGHT,
    SYNC_HARD_BUTTON_LIGHT,
    SYNC_SOFT_BUTTON_LIGHT,

    SYNC_PHASE_CLIP_LIGHT,
    FREQ_CLIP_LIGHT,
    MIX1_PULSE_CLIP_LIGHT,
    EXT_MOD_AMOUNT_CLIP_LIGHT,
    MIX1_TRIANGLE_CLIP_LIGHT,
    MIX1_SAW_CLIP_LIGHT,
    PULSE_WIDTH_CLIP_LIGHT,
    LINEAR_CLIP_LIGHT,

    ENUMS(LEFT_EXPANDER_LIGHT, 3),
    ENUMS(RIGHT_EXPANDER_LIGHT, 3),
    LIGHTS_LEN
  };


  float syncPhaseClipTimer = 0.f;
  float freqClipTimer = 0.f;
  float mix1PulseVcaClipTimer = 0.f;
  float extModAmountClipTimer = 0.f;
  float mix1TriangleVcaClipTimer = 0.f;
  float mix1SawVcaClipTimer = 0.f;
  float pulseWidthClipTimer = 0.f;
  float linearClipTimer = 0.f;

  std::string output1NameString;
  std::string output2NameString;
  std::string modulationInputNameString;


  // index corresponds on both vectors for tracking button pushes and outgoing MIDI msg
  static const std::vector<ButtonMapping<Zoxnoxious3340> > buttonMappings;
  ButtonMidiController<Zoxnoxious3340> buttonMidiController;

  // the stateful selector is handled a bit differently than toggle buttons.
  // This is the MIDI program changes it sends and how it is tracked.
  static constexpr int8_t extModSelectMidiPrograms[] = { 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 };
  int extModSelectSwitchValue = 0; // current index to above array
  bool extModSelectChanged = false;
  std::array<CvRoute,8> routes;


  Zoxnoxious3340() :
    buttonMidiController(buttonMappings),
    routes{{
      {LINEAR_KNOB_PARAM, LINEAR_INPUT, LINEAR_CHANNEL, 10.f, &linearClipTimer, nullptr, CvOperation::Add},
      {PULSE_WIDTH_KNOB_PARAM, PULSE_WIDTH_INPUT, PULSE_WIDTH_CHANNEL, 10.f, &pulseWidthClipTimer, nullptr, CvOperation::Add},
      {MIX1_SAW_KNOB_PARAM, MIX1_SAW_VCA_INPUT, MIX1_SAW_VCA_CHANNEL, 10.f, &mix1SawVcaClipTimer, nullptr, CvOperation::MultiplyNormalled},
      {MIX1_TRIANGLE_KNOB_PARAM, MIX1_TRIANGLE_VCA_INPUT, MIX1_TRIANGLE_VCA_CHANNEL, 10.f, &mix1TriangleVcaClipTimer, nullptr, CvOperation::MultiplyNormalled},
      {EXT_MOD_AMOUNT_KNOB_PARAM, EXT_MOD_AMOUNT_INPUT, EXT_MOD_AMOUNT_CHANNEL, 10.f, &extModAmountClipTimer, nullptr, CvOperation::MultiplyNormalled},
      {MIX1_PULSE_KNOB_PARAM, MIX1_PULSE_VCA_INPUT, MIX1_PULSE_VCA_CHANNEL, 10.f, &mix1PulseVcaClipTimer, nullptr, CvOperation::MultiplyNormalled},
      {SYNC_PHASE_KNOB_PARAM, SYNC_PHASE_INPUT, SYNC_PHASE_CHANNEL, 10.f, &syncPhaseClipTimer, nullptr, CvOperation::Add},
      {FREQ_KNOB_PARAM, FREQ_INPUT, FREQ_CHANNEL, 8.f, &freqClipTimer, nullptr, CvOperation::Add}
    }} {

    setParticipant(this);
    setLightEnum(RIGHT_EXPANDER_LIGHT);

    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(FREQ_KNOB_PARAM, 0.f, 1.f, 0.5f, "Frequency", " V", 0.f, 8.f);
    configParam(PULSE_WIDTH_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pulse Width", "%", 0.f, 100.f);
    configParam(LINEAR_KNOB_PARAM, 0.f, 1.f, 0.5f, "Linear Mod", " V", 0.f, 10.f, -5.f);

    configParam(MIX1_PULSE_KNOB_PARAM, 0.f, 1.f, 0.f, "Mix1 Pulse", "%", 0.f, 100.f);
    configParam(MIX1_TRIANGLE_KNOB_PARAM, 0.f, 1.f, 0.f, "Mix1 Triangle", "%", 0.f, 100.f);
    configParam(MIX1_SAW_KNOB_PARAM, 0.f, 1.f, 0.f, "Mix1 Saw", "%", 0.f, 100.f);

    configSwitch(MIX2_PULSE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Mix2 Pulse", {"Off", "On"});
    configSwitch(MIX2_SAW_BUTTON_PARAM, 0.f, 1.f, 0.f, "Mix2 Saw", {"Off", "On"});

    configButton(EXT_MOD_SELECT_SWITCH_UP_PARAM, "Next");
    configButton(EXT_MOD_SELECT_SWITCH_DOWN_PARAM, "Previous");
    configParam(EXT_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 1.f, "External Mod Level", "%", 0.f, 100.f);
    configSwitch(EXT_MOD_PWM_BUTTON_PARAM, 0.f, 1.f, 0.f, "Ext Mod to PWM", {"Off", "On"});
    configSwitch(EXP_FM_BUTTON_PARAM, 0.f, 1.f, 0.f, "Ext Mod to Exp FM", {"Off", "On"});
    configSwitch(LINEAR_FM_BUTTON_PARAM, 0.f, 1.f, 0.f, "Ext Mod to Linear FM", {"Off", "On"});

    configParam(SYNC_PHASE_KNOB_PARAM, 0.f, 1.f, 0.5f, "Sync Phase", "%", 0.f, 100.f);
    configSwitch(SYNC_NEG_BUTTON_PARAM, 0.f, 1.f, 0.f, "Neg", {"Off", "On"});
    configSwitch(SYNC_POS_BUTTON_PARAM, 0.f, 1.f, 0.f, "Pos", {"Off", "On"});
    configSwitch(SYNC_HARD_BUTTON_PARAM, 0.f, 1.f, 0.f, "Hard", {"Off", "On"});
    configSwitch(SYNC_SOFT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Soft", {"Off", "On"});

    configInput(SYNC_PHASE_INPUT, "Sync Phase");
    configInput(FREQ_INPUT, "Pitch CV");
    configInput(MIX1_PULSE_VCA_INPUT, "Mix1 Pulse");
    configInput(EXT_MOD_AMOUNT_INPUT, "External Modulation Amount");
    configInput(MIX1_TRIANGLE_VCA_INPUT, "Mix1 Triangle");
    configInput(MIX1_SAW_VCA_INPUT, "Mix1 Saw");
    configInput(PULSE_WIDTH_INPUT, "Pulse Width Modulation");
    configInput(LINEAR_INPUT, "Linear FM");

    configLight(LEFT_EXPANDER_LIGHT, "Connection Status");
    configLight(RIGHT_EXPANDER_LIGHT, "Connection Status");

    output1NameString.reserve(16);
    output1NameString = invalidCardOutputName;
    output2NameString.reserve(16);
    output2NameString = invalidCardOutputName;
    modulationInputNameString.reserve(16);
    modulationInputNameString = invalidCardOutputName;
  }



  /* Participant interface */
  int64_t getModuleId() override {
    return getId();
  }

  void pullSamples(const rack::engine::Module::ProcessArgs &args, dsp::Frame<maxAudioChannels> &sharedFrame, int offset) override {
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
    bool midiMsgSet = false;

    // check if selector gets the midi message.  If not the button controller runs.
    // actually -- maybe that's backwards.  Try button controller first.

    // the last entry of buttonParamToMidiProgramList is handled here:
    // add/subtract the up/down buttons
    int up_param = params[EXT_MOD_SELECT_SWITCH_UP_PARAM].getValue();
    int down_param = params[EXT_MOD_SELECT_SWITCH_DOWN_PARAM].getValue();
    // lights will get lit for one frame-- once they're set they
    // are immediately unset on the next pullMidi()
    lights[EXT_MOD_SELECT_SWITCH_UP_LIGHT].setBrightness(up_param);
    lights[EXT_MOD_SELECT_SWITCH_DOWN_LIGHT].setBrightness(down_param);

    if (up_param) {
      params[EXT_MOD_SELECT_SWITCH_UP_PARAM].setValue(0);
      extModSelectSwitchValue =
        extModSelectSwitchValue >= 12 ? 0 : extModSelectSwitchValue + 1;
      extModSelectChanged = true;
    }
    if (down_param) {
      params[EXT_MOD_SELECT_SWITCH_DOWN_PARAM].setValue(0);
      extModSelectSwitchValue =
        extModSelectSwitchValue <= 0 ? 12 : extModSelectSwitchValue - 1;
      extModSelectChanged = true;
    }

    if (extModSelectChanged) {
      extModSelectChanged = false;
      setMidiProgramChangeMessage(midiMessage, midiChannel, extModSelectMidiPrograms[extModSelectSwitchValue]);
      midiMsgSet = true;
      if (lifecycle.nameService != nullptr) {
        modulationInputNameString = *lifecycle.nameService->getNamePtr(extModSelectSwitchValue);
      }
    }


    // all buttons outside of the above selector are handled here
    if (!midiMsgSet) {
      midiMsgSet = buttonMidiController.process(this, midiChannel, midiMessage);
    }
    // lights may lead midi messages if above buttons are skipped on this clock.  oh well.
    buttonMidiController.updateLights(this);


    // Then clipping lights.
    // clipping light timer
    const float lightTime = args.sampleTime * clockDivision;
    const float brightnessDeltaTime = 1 / lightTime;

    syncPhaseClipTimer -= lightTime;
    lights[SYNC_PHASE_CLIP_LIGHT].setBrightnessSmooth(syncPhaseClipTimer > 0.f, brightnessDeltaTime);

    freqClipTimer -= lightTime;
    lights[FREQ_CLIP_LIGHT].setBrightnessSmooth(freqClipTimer > 0.f, brightnessDeltaTime);

    mix1PulseVcaClipTimer -= lightTime;
    lights[MIX1_PULSE_CLIP_LIGHT].setBrightnessSmooth(mix1PulseVcaClipTimer > 0.f, brightnessDeltaTime);

    extModAmountClipTimer -= lightTime;
    lights[EXT_MOD_AMOUNT_CLIP_LIGHT].setBrightnessSmooth(extModAmountClipTimer > 0.f, brightnessDeltaTime);

    mix1TriangleVcaClipTimer -= lightTime;
    lights[MIX1_TRIANGLE_CLIP_LIGHT].setBrightnessSmooth(mix1TriangleVcaClipTimer > 0.f, brightnessDeltaTime);

    mix1SawVcaClipTimer -= lightTime;
    lights[MIX1_SAW_CLIP_LIGHT].setBrightnessSmooth(mix1SawVcaClipTimer > 0.f, brightnessDeltaTime);

    pulseWidthClipTimer -= lightTime;
    lights[PULSE_WIDTH_CLIP_LIGHT].setBrightnessSmooth(pulseWidthClipTimer > 0.f, brightnessDeltaTime);

    linearClipTimer -= lightTime;
    lights[LINEAR_CLIP_LIGHT].setBrightnessSmooth(linearClipTimer > 0.f, brightnessDeltaTime);

    return midiMsgSet;
  }


  /** getCardHardwareId
   * return the hardware Id of the 3340 card
   * This MUST match the ROM Id on the card
   */
  static constexpr uint8_t hardwareId = 0x02;
  uint8_t getHardwareId() const override {
    return hardwareId;
  }


  void onAttach() override {
    if (lifecycle.nameService == nullptr) {
      return;
    }
    auto *ptr1 = lifecycle.nameService->getNamePtr(lifecycle.slotNum * 2);
    auto *ptr2 = lifecycle.nameService->getNamePtr(lifecycle.slotNum * 2 + 1);
    auto *ptrMod = lifecycle.nameService->getNamePtr(extModSelectSwitchValue);
    output1NameString = ptr1 ? *ptr1 : invalidCardOutputName;
    output2NameString = ptr2 ? *ptr2 : invalidCardOutputName;
    modulationInputNameString = ptrMod ? *ptrMod : invalidCardOutputName;
  }



private:

  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "extModSelectSwitch", json_integer(extModSelectSwitchValue));
    return rootJ;
  }

  void dataFromJson(json_t* rootJ) override {
    json_t* extModSelectSwitchJ = json_object_get(rootJ, "extModSelectSwitch");
    if (extModSelectSwitchJ) {
      extModSelectChanged = true;
      extModSelectSwitchValue = json_integer_value(extModSelectSwitchJ);
    }
  }

};






struct Zoxnoxious3340Widget : ModuleWidget {

  Zoxnoxious3340Widget(Zoxnoxious3340* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Zoxnoxious3340.svg")));

    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    //addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.5, 26.112)), module, Zoxnoxious3340::FREQ_KNOB_PARAM));
    auto* knob = createParamCentered<TurnsCountingKnob>(
      mm2px(Vec(14.5, 26.112)),   // centre position on panel (mm)
      module,
      Zoxnoxious3340::FREQ_KNOB_PARAM);
    knob->setTurns(8);
    addParam(knob);

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.5, 92.978)), module, Zoxnoxious3340::PULSE_WIDTH_KNOB_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(14.5, 58.11)), module, Zoxnoxious3340::LINEAR_KNOB_PARAM));

    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(88.75, 23.3)), module, Zoxnoxious3340::MIX1_PULSE_KNOB_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(88.75, 64.369)), module, Zoxnoxious3340::MIX1_TRIANGLE_KNOB_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(88.75, 43.649)), module, Zoxnoxious3340::MIX1_SAW_KNOB_PARAM));

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(66.5, 58.21)), module, Zoxnoxious3340::SYNC_PHASE_KNOB_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(40.5, 58.11)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_KNOB_PARAM));

    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(60.84, 99.296)), module, Zoxnoxious3340::SYNC_HARD_BUTTON_PARAM, Zoxnoxious3340::SYNC_HARD_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(60.84, 110.696)), module, Zoxnoxious3340::SYNC_POS_BUTTON_PARAM, Zoxnoxious3340::SYNC_POS_BUTTON_LIGHT));

    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(72.16, 99.296)), module, Zoxnoxious3340::SYNC_SOFT_BUTTON_PARAM, Zoxnoxious3340::SYNC_SOFT_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(72.16, 110.696)), module, Zoxnoxious3340::SYNC_NEG_BUTTON_PARAM, Zoxnoxious3340::SYNC_NEG_BUTTON_LIGHT));


    addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(56.374, 35.791)), module, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_UP_PARAM, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_UP_LIGHT));
    addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(43.304, 35.791)), module, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_DOWN_PARAM, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_DOWN_LIGHT));

    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(88.704, 99.297)), module, Zoxnoxious3340::MIX2_PULSE_BUTTON_PARAM, Zoxnoxious3340::MIX2_PULSE_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(40.5, 113.757)), module, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_PARAM, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(40.5, 91.458)), module, Zoxnoxious3340::EXP_FM_BUTTON_PARAM, Zoxnoxious3340::EXP_FM_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(40.5, 102.92)), module, Zoxnoxious3340::LINEAR_FM_BUTTON_PARAM, Zoxnoxious3340::LINEAR_FM_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(101.892, 99.297)), module, Zoxnoxious3340::MIX2_SAW_BUTTON_PARAM, Zoxnoxious3340::MIX2_SAW_BUTTON_LIGHT));

    addInput(createInputCentered<BNCPort>(mm2px(Vec(14.5, 40.112)), module, Zoxnoxious3340::FREQ_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(14.5, 106.978)), module, Zoxnoxious3340::PULSE_WIDTH_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(14.5, 72.11)), module, Zoxnoxious3340::LINEAR_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(103.0, 23.3)), module, Zoxnoxious3340::MIX1_PULSE_VCA_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(103.0, 64.369)), module, Zoxnoxious3340::MIX1_TRIANGLE_VCA_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(103.0, 43.649)), module, Zoxnoxious3340::MIX1_SAW_VCA_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(66.5, 72.11)), module, Zoxnoxious3340::SYNC_PHASE_INPUT));
    addInput(createInputCentered<BNCPort>(mm2px(Vec(40.5, 72.11)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_INPUT));

    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(22.645, 35.112)), module, Zoxnoxious3340::FREQ_CLIP_LIGHT));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(22.645, 102.123)), module, Zoxnoxious3340::PULSE_WIDTH_CLIP_LIGHT));
    //addChild(createLightCentered<SmallLight<RedLight>>(mm2px(ABCD), module, Zoxnoxious3340::LINEAR_CLIP_LIGHT));
    //addChild(createLightCentered<SmallLight<RedLight>>(mm2px(ABCD), module, Zoxnoxious3340::MIX1_PULSE_CLIP_LIGHT));
    //addChild(createLightCentered<SmallLight<RedLight>>(mm2px(ABCD), module, Zoxnoxious3340::MIX1_TRIANGLE_CLIP_LIGHT));
    //addChild(createLightCentered<SmallLight<RedLight>>(mm2px(ABCD), module, Zoxnoxious3340::MIX1_SAW_CLIP_LIGHT));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(73.538, 66.175)), module, Zoxnoxious3340::SYNC_PHASE_CLIP_LIGHT));
    //addChild(createLightCentered<SmallLight<RedLight>>(mm2px(ABCD), module, Zoxnoxious3340::EXT_MOD_AMOUNT_CLIP_LIGHT));

    //addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(ABCD), module, Zoxnoxious3340::LEFT_EXPANDER_LIGHT));
    addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(4.468, 121.583)), module, Zoxnoxious3340::RIGHT_EXPANDER_LIGHT));

    mix1OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(85.023, 74.2)));
    mix1OutputTextField->setNumChars(11);
    mix1OutputTextField->box.size = (mm2px(Vec(19.0, 3.136)));
    mix1OutputTextField->setText(module ? &module->output1NameString : NULL);
    addChild(mix1OutputTextField);

    mix2OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(85.023, 104.6)));
    mix2OutputTextField->setNumChars(11);
    mix2OutputTextField->box.size = (mm2px(Vec(19.0, 3.136)));
    mix2OutputTextField->setText(module ? &module->output2NameString : NULL);
    addChild(mix2OutputTextField);

    modulationInputTextField = createWidget<CardTextDisplay>(mm2px(Vec(40.181, 25.012)));
    modulationInputTextField->setNumChars(14);
    modulationInputTextField->box.size = (mm2px(Vec(24.0, 3.136)));
    modulationInputTextField->setText(module ? &module->modulationInputNameString  : NULL);
    addChild(modulationInputTextField);
  }

  CardTextDisplay *mix1OutputTextField;
  CardTextDisplay *mix2OutputTextField;
  CardTextDisplay *modulationInputTextField;

};

const std::vector<ButtonMapping<Zoxnoxious3340> > Zoxnoxious3340::buttonMappings = {
  { SYNC_HARD_BUTTON_PARAM, SYNC_HARD_BUTTON_LIGHT, {0,1} },
  { EXT_MOD_PWM_BUTTON_PARAM, EXT_MOD_PWM_BUTTON_LIGHT, {2,3} },
  { SYNC_NEG_BUTTON_PARAM, SYNC_NEG_BUTTON_LIGHT, {4,5} },
  { SYNC_SOFT_BUTTON_PARAM, SYNC_SOFT_BUTTON_LIGHT, {6,7} },
  { SYNC_POS_BUTTON_PARAM, SYNC_POS_BUTTON_LIGHT, {8,9} },
  { LINEAR_FM_BUTTON_PARAM, LINEAR_FM_BUTTON_LIGHT, {10,11} },
  { MIX2_SAW_BUTTON_PARAM, MIX2_SAW_BUTTON_LIGHT, {12,13} },
  { MIX2_PULSE_BUTTON_PARAM, MIX2_PULSE_BUTTON_LIGHT, {14,15} },
  { EXP_FM_BUTTON_PARAM, EXP_FM_BUTTON_LIGHT, {16,17} }
};

constexpr int8_t Zoxnoxious3340::extModSelectMidiPrograms[];

} // namespace zox

Model* modelZoxnoxious3340 = createModel<zox::Zoxnoxious3340, zox::Zoxnoxious3340Widget>("Zoxnoxious3340");

