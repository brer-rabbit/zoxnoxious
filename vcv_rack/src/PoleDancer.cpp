#include "plugin.hpp"
#include "zcomponentlib.hpp"
#include "ZoxnoxiousExpander.hpp"

static const int midiMessageQueueMaxSize = 16;
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
  POLE4_LEVEL,
  POLE2_LEVEL,
  POLE3_LEVEL,
  POLE1_LEVEL,
  Q_VCA,
  DRY_LEVEL
};

static const std::string rezCompModes[] = { "Uncompensated", "Bandpass 4P", "Alt Mode 1", "Alt Mode 2" };


struct PoleDancer : ZoxnoxiousModule {
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
    ENUMS(LEFT_EXPANDER_LIGHT, 3),
    ENUMS(RIGHT_EXPANDER_LIGHT, 3),
    LIGHTS_LEN
  };


  dsp::ClockDivider lightDivider;
  float sourceOneLevelClipTimer;
  float sourceOneModAmountClipTimer;
  float sourceTwoLevelClipTimer;
  float sourceTwoModAmountClipTimer;
  float cutoffClipTimer;
  float resonanceClipTimer;
  float filterVcaClipTimer;

  std::deque<midi::Message> midiMessageQueue;

  std::string source1NameString;
  std::string source2NameString;
  std::string output1NameString;
  std::string output2NameString;
  std::string rezCompModeNameString;

  struct buttonParamMidiProgram {
      enum ParamId button;
      int previousValue;
      uint8_t midiProgram[8];
  } buttonParamToMidiProgramList[6] =
    {
      { SOURCE_ONE_VALUE_HIDDEN_PARAM, INT_MIN, { 0, 1, 2, 3, 4, 5, 6, 7 } },
      { SOURCE_TWO_VALUE_HIDDEN_PARAM, INT_MIN, { 8, 9, 10, 11, 12, 13, 14, 15 } },
      { REZ_COMP_VALUE_HIDDEN_PARAM, INT_MIN, { 24, 25, 26, 27 } }
    };


  PoleDancer() :
    sourceOneLevelClipTimer(0.f), sourceOneModAmountClipTimer(0.f),
    sourceTwoLevelClipTimer(0.f), sourceTwoModAmountClipTimer(0.f),
    cutoffClipTimer(0.f), resonanceClipTimer(0.f),
    filterVcaClipTimer(0.f),
    source1NameString(invalidCardOutputName), source2NameString(invalidCardOutputName),
    output1NameString(invalidCardOutputName), output2NameString(invalidCardOutputName),
    rezCompModeNameString(rezCompModes[0])
    {

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
    lightDivider.setDivision(512);
  }

  void process(const ProcessArgs& args) override {
    processExpander(args);

    if (lightDivider.process()) {
      // light up any buttons
      lights[SOURCE_ONE_DOWN_BUTTON_LIGHT].setBrightness(params[SOURCE_ONE_DOWN_BUTTON_PARAM].getValue());
      lights[SOURCE_ONE_UP_BUTTON_LIGHT].setBrightness(params[SOURCE_ONE_UP_BUTTON_PARAM].getValue());
      lights[SOURCE_TWO_DOWN_BUTTON_LIGHT].setBrightness(params[SOURCE_TWO_DOWN_BUTTON_PARAM].getValue());
      lights[SOURCE_TWO_UP_BUTTON_LIGHT].setBrightness(params[SOURCE_TWO_UP_BUTTON_PARAM].getValue());
      lights[REZ_COMP_DOWN_BUTTON_LIGHT].setBrightness(params[REZ_COMP_DOWN_BUTTON_PARAM].getValue());
      lights[REZ_COMP_UP_BUTTON_LIGHT].setBrightness(params[REZ_COMP_UP_BUTTON_PARAM].getValue());

      // clipping
      const float lightTime = args.sampleTime * lightDivider.getDivision();
      const float brightnessDeltaTime = 1 / lightTime;


      // for clipping lights just keep subtracting every cycle.  Timer gets set if needed in processZoxnoxiousControl
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

      setLeftExpanderLight(LEFT_EXPANDER_LIGHT);
      setRightExpanderLight(RIGHT_EXPANDER_LIGHT);

      // add/subtract the up/down buttons and set a string that
      // the UI can use.  There ought to be some todos here to
      // make/fix this.
      if (params[ SOURCE_ONE_UP_BUTTON_PARAM ].getValue()) {
        params[ SOURCE_ONE_UP_BUTTON_PARAM ].setValue(0.f);
        int sourceOneInt = static_cast<int>(std::round(params[ SOURCE_ONE_VALUE_HIDDEN_PARAM ].getValue()));
        sourceOneInt = sourceOneInt == 7 ? 0 : sourceOneInt + 1;
        params[ SOURCE_ONE_VALUE_HIDDEN_PARAM ].setValue(sourceOneInt);
        source1NameString = cardOutputNames[ source1Sources[sourceOneInt] ];
      }
      if (params[ SOURCE_ONE_DOWN_BUTTON_PARAM ].getValue()) {
        params[ SOURCE_ONE_DOWN_BUTTON_PARAM ].setValue(0);
        int sourceOneInt = static_cast<int>(std::round(params[ SOURCE_ONE_VALUE_HIDDEN_PARAM ].getValue()));
        sourceOneInt = sourceOneInt == 0 ? 7 : sourceOneInt - 1;
        params[ SOURCE_ONE_VALUE_HIDDEN_PARAM ].setValue(sourceOneInt);
        source1NameString = cardOutputNames[ source1Sources[sourceOneInt] ];
      }

      if (params[ SOURCE_TWO_UP_BUTTON_PARAM ].getValue()) {
        params[ SOURCE_TWO_UP_BUTTON_PARAM ].setValue(0);
        int sourceTwoInt = static_cast<int>(std::round(params[ SOURCE_TWO_VALUE_HIDDEN_PARAM ].getValue()));
        sourceTwoInt = sourceTwoInt == 7 ? 0 : sourceTwoInt + 1;
        params[ SOURCE_TWO_VALUE_HIDDEN_PARAM ].setValue(sourceTwoInt);
        source2NameString = cardOutputNames[ source2Sources[sourceTwoInt] ];
      }
      if (params[ SOURCE_TWO_DOWN_BUTTON_PARAM ].getValue()) {
        params[ SOURCE_TWO_DOWN_BUTTON_PARAM ].setValue(0);
        int sourceTwoInt = static_cast<int>(std::round(params[ SOURCE_TWO_VALUE_HIDDEN_PARAM ].getValue()));
        sourceTwoInt = sourceTwoInt == 0 ? 7 : sourceTwoInt - 1;
        params[ SOURCE_TWO_VALUE_HIDDEN_PARAM ].setValue(sourceTwoInt);
        source2NameString = cardOutputNames[ source2Sources[sourceTwoInt] ];
      }

      // rez compensation mode selection buttons
      if (params[ REZ_COMP_UP_BUTTON_PARAM ].getValue()) {
        params[ REZ_COMP_UP_BUTTON_PARAM ].setValue(0);
        int rezCompModeInt = static_cast<int>(std::round(params[ REZ_COMP_VALUE_HIDDEN_PARAM ].getValue()));
        rezCompModeInt = rezCompModeInt == 3 ? 0 : rezCompModeInt + 1;
        params[ REZ_COMP_VALUE_HIDDEN_PARAM ].setValue(rezCompModeInt);
        rezCompModeNameString = rezCompModes[ rezCompModeInt ];
      }
      if (params[ REZ_COMP_DOWN_BUTTON_PARAM ].getValue()) {
        params[ REZ_COMP_DOWN_BUTTON_PARAM ].setValue(0);
        int rezCompModeInt = static_cast<int>(std::round(params[ REZ_COMP_VALUE_HIDDEN_PARAM ].getValue()));
        rezCompModeInt = rezCompModeInt == 0 ? 3 : rezCompModeInt - 1;
        params[ REZ_COMP_VALUE_HIDDEN_PARAM ].setValue(rezCompModeInt);
        rezCompModeNameString = rezCompModes[ rezCompModeInt ];
      }

    }

  }


  void processZoxnoxiousControl(ZoxnoxiousControlMsg *controlMsg) override {

    if (!hasChannelAssignment) {
      return;
    }

    float v;
    const float clipTime = 0.25f;

    // vcf cutoff
    v = params[CUTOFF_KNOB_PARAM].getValue() + inputs[CUTOFF_INPUT].getVoltage() / 10.f;
    controlMsg->frame[outputDeviceId].samples[cvChannelOffset + VCF_CUTOFF] = clamp(v, 0.f, 1.f);
    if (controlMsg->frame[outputDeviceId].samples[cvChannelOffset + VCF_CUTOFF] != v) {
      cutoffClipTimer = clipTime;
    }

    // source one audio
    v = params[SOURCE_ONE_LEVEL_KNOB_PARAM].getValue() + inputs[SOURCE_ONE_LEVEL_INPUT].getVoltage() / 10.f;
    controlMsg->frame[outputDeviceId].samples[cvChannelOffset + SOURCE_ONE_LEVEL] = clamp(v, 0.f, 1.f);
    if (controlMsg->frame[outputDeviceId].samples[cvChannelOffset + SOURCE_ONE_LEVEL] != v) {
      sourceOneLevelClipTimer = clipTime;
    }

    // source two audio
    v = params[SOURCE_TWO_LEVEL_KNOB_PARAM].getValue() + inputs[SOURCE_TWO_LEVEL_INPUT].getVoltage() / 10.f;
    controlMsg->frame[outputDeviceId].samples[cvChannelOffset + SOURCE_TWO_LEVEL] = clamp(v, 0.f, 1.f);
    if (controlMsg->frame[outputDeviceId].samples[cvChannelOffset + SOURCE_TWO_LEVEL] != v) {
      sourceTwoLevelClipTimer = clipTime;
    }

    // source one modulation
    v = params[SOURCE_ONE_MOD_AMOUNT_KNOB_PARAM].getValue() + inputs[SOURCE_ONE_MOD_AMOUNT_INPUT].getVoltage() / 10.f;
    controlMsg->frame[outputDeviceId].samples[cvChannelOffset + SOURCE_ONE_MOD_AMOUNT] = clamp(v, 0.f, 1.f);
    if (controlMsg->frame[outputDeviceId].samples[cvChannelOffset + SOURCE_ONE_MOD_AMOUNT] != v) {
      sourceOneModAmountClipTimer = clipTime;
    }

    // source two modulation
    v = params[SOURCE_TWO_MOD_AMOUNT_KNOB_PARAM].getValue() + inputs[SOURCE_TWO_MOD_AMOUNT_INPUT].getVoltage() / 10.f;
    controlMsg->frame[outputDeviceId].samples[cvChannelOffset + SOURCE_TWO_MOD_AMOUNT] = clamp(v, 0.f, 1.f);
    if (controlMsg->frame[outputDeviceId].samples[cvChannelOffset + SOURCE_TWO_MOD_AMOUNT] != v) {
      sourceTwoModAmountClipTimer = clipTime;
    }

    // qvca
    v = params[RESONANCE_KNOB_PARAM].getValue() + inputs[RESONANCE_INPUT].getVoltage() / 10.f;
    // Resonance slope & max will vary depending on Resonance Compensation mode
    // for consistency between modes, try to get oscillation to start around 80%.
    // From 80% then ramp up to the max allowable value.
    switch( int(params[ REZ_COMP_VALUE_HIDDEN_PARAM ].getValue() + 0.5f) ) {
    case 2: // modified 2P-bandpass
        // oscillation starts at 16%, max rez is at 33%
        // map 80% --> 16% and 100% --> 33%
        v = v < 0.80f ? 0.20f * v : 0.85f * v - 0.52f;
        controlMsg->frame[outputDeviceId].samples[cvChannelOffset + Q_VCA] = clamp(v, 0.f, 0.34f);
        break;
    case 3: // oddball comp
        // oscillation starts at 26%, max rez is at 50%
        // map 80% --> 26% and 100% --> 50%
        v = v < 0.80f ? 0.325f * v : 1.2f * v - 0.7f;
        controlMsg->frame[outputDeviceId].samples[cvChannelOffset + Q_VCA] = clamp(v, 0.f, 0.51f);
        break;
    default: // uncomp and 4P-bandpass
        // map 80% --> 70% and 100% --> 100%
        v = v < 0.80f ? 0.875f * v : 1.5f * v - 0.5f;
        controlMsg->frame[outputDeviceId].samples[cvChannelOffset + Q_VCA] = clamp(v, 0.f, 1.f);
        break;
    }

    if (controlMsg->frame[outputDeviceId].samples[cvChannelOffset + Q_VCA] != v) {
        resonanceClipTimer = clipTime;
    }


    // pole levels:
    // these are scaled by the Filter VCA, so get that first
    v = params[FILTER_VCA_KNOB_PARAM].getValue() + inputs[FILTER_VCA_INPUT].getVoltage() / 10.f;
    float filterVcaGain = clamp(v, 0.f, 1.f);
    if (filterVcaGain != v) {
      filterVcaClipTimer = clipTime;
    }


    if (inputs[POLE_MIX_INPUT].isConnected()) {
      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + DRY_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(0) / 10.f) * filterVcaGain;

      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + POLE1_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(1) / 10.f) * filterVcaGain;

      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + POLE2_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(2) / 10.f) * filterVcaGain;

      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + POLE3_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(3) / 10.f) * filterVcaGain;

      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + POLE4_LEVEL] =
        (inputs[POLE_MIX_INPUT].getVoltage(4) / 10.f) * filterVcaGain;
    } else {
      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + DRY_LEVEL] = 0.f;
      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + POLE1_LEVEL] = 0.f;
      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + POLE2_LEVEL] = 0.f;
      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + POLE3_LEVEL] = 0.f;
      controlMsg->frame[outputDeviceId].samples[cvChannelOffset + POLE4_LEVEL] = filterVcaGain;
    }



    // if we have any queued midi messages, send them
    if (controlMsg->midiMessageSet == false) {
      if (midiMessageQueue.size() > 0) {
        controlMsg->midiMessageSet = true;
        controlMsg->midiMessage = midiMessageQueue.front();
        midiMessageQueue.pop_front();
      }
    }


    for (int i = 0; i < (int) (sizeof(buttonParamToMidiProgramList) / sizeof(struct buttonParamMidiProgram)); ++i) {
      int newValue = static_cast<int>(std::round(params[ buttonParamToMidiProgramList[i].button ].getValue()));

      if (buttonParamToMidiProgramList[i].previousValue != newValue) {
        buttonParamToMidiProgramList[i].previousValue = newValue;
        if (controlMsg->midiMessageSet == false) {
          // send direct
          controlMsg->midiMessage.setSize(2);
          controlMsg->midiMessage.setChannel(midiChannel);
          controlMsg->midiMessage.setStatus(midiProgramChangeStatus);
          controlMsg->midiMessage.setNote(buttonParamToMidiProgramList[i].midiProgram[newValue]);
          controlMsg->midiMessageSet = true;
          INFO("poledancer: clock %" PRId64 " :  MIDI message direct midi channel %d", APP->engine->getFrame(), midiChannel);
        }
        else if (midiMessageQueue.size() < midiMessageQueueMaxSize) {
          midi::Message queuedMessage;
          queuedMessage.setSize(2);
          queuedMessage.setChannel(midiChannel);
          queuedMessage.setStatus(midiProgramChangeStatus);
          queuedMessage.setNote(buttonParamToMidiProgramList[i].midiProgram[newValue]);
          midiMessageQueue.push_back(queuedMessage);
        }
        else {
          INFO("poledancer: dropping MIDI message, bus full and queue full");
        }
      }
    }

  }


  /** getCardHardwareId
   * return the hardware Id of the poledancer card
   */
  static const uint8_t hardwareId = 0x06;
  uint8_t getHardwareId() override {
    return hardwareId;
  }


  void onChannelAssignmentEstablished(ZoxnoxiousCommandMsg *zCommand) override {
    ZoxnoxiousModule::onChannelAssignmentEstablished(zCommand);
    output1NameString = getCardOutputName(hardwareId, 1, slot);
    output2NameString = getCardOutputName(hardwareId, 2, slot);
    int sourceOneInt = static_cast<int>(std::round(params[ SOURCE_ONE_VALUE_HIDDEN_PARAM ].getValue()));
    source1NameString = cardOutputNames[ source1Sources[sourceOneInt] ];
    int sourceTwoInt = static_cast<int>(std::round(params[ SOURCE_TWO_VALUE_HIDDEN_PARAM ].getValue()));
    source2NameString = cardOutputNames[ source2Sources[sourceTwoInt] ];
  }

  void onChannelAssignmentLost() override {
    ZoxnoxiousModule::onChannelAssignmentLost();
    output1NameString = invalidCardOutputName;
    output2NameString = invalidCardOutputName;
    // should do something better with source{1,2}NameString here since
    // user can still click on the buttons to change them
    source1NameString = invalidCardOutputName;
    source2NameString = invalidCardOutputName;
  }


  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    return rootJ;
  }

  void dataFromJson(json_t* rootJ) override {
    int rezCompModeInt = static_cast<int>(std::round(params[ REZ_COMP_VALUE_HIDDEN_PARAM ].getValue()));
    rezCompModeNameString = rezCompModes[ rezCompModeInt ];
    INFO("poledancer: setting comp mode to %s", rezCompModeNameString.c_str());
  }
};


struct PoleDancerWidget : ModuleWidget {
  PoleDancerWidget(PoleDancer* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/PoleDancer.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH / 2, 0)));
    //addChild(createWidget<ScrewSilver>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(10.72, 21.599)), module, PoleDancer::SOURCE_ONE_DOWN_BUTTON_PARAM, PoleDancer::SOURCE_ONE_DOWN_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(22.326, 21.599)), module, PoleDancer::SOURCE_ONE_UP_BUTTON_PARAM, PoleDancer::SOURCE_ONE_UP_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(47.604, 21.599)), module, PoleDancer::SOURCE_TWO_DOWN_BUTTON_PARAM, PoleDancer::SOURCE_TWO_DOWN_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(58.965, 21.599)), module, PoleDancer::SOURCE_TWO_UP_BUTTON_PARAM, PoleDancer::SOURCE_TWO_UP_BUTTON_LIGHT));

    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(15.131, 110.166)), module, PoleDancer::REZ_COMP_DOWN_BUTTON_PARAM, PoleDancer::REZ_COMP_DOWN_BUTTON_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(26.737, 110.166)), module, PoleDancer::REZ_COMP_UP_BUTTON_PARAM, PoleDancer::REZ_COMP_UP_BUTTON_LIGHT));

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(13.639, 39.831)), module, PoleDancer::SOURCE_ONE_LEVEL_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(29.210, 39.831)), module, PoleDancer::SOURCE_ONE_MOD_AMOUNT_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.462, 39.831)), module, PoleDancer::SOURCE_TWO_LEVEL_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(68.8, 39.831)), module, PoleDancer::SOURCE_TWO_MOD_AMOUNT_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.37, 77.109)), module, PoleDancer::CUTOFF_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(29.21, 77.379)), module, PoleDancer::RESONANCE_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(68.474, 77.415)), module, PoleDancer::FILTER_VCA_KNOB_PARAM));


    /*
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(43.666, 76.946)), module, PoleDancer::REZ_COMP_P1_SWITCH_PARAM, PoleDancer::REZ_COMP_P1_SWITCH_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(52.616, 76.946)), module, PoleDancer::REZ_COMP_P2_SWITCH_PARAM, PoleDancer::REZ_COMP_P2_SWITCH_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(43.666, 91.072)), module, PoleDancer::REZ_COMP_P3_SWITCH_PARAM, PoleDancer::REZ_COMP_P3_SWITCH_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(52.616, 91.072)), module, PoleDancer::REZ_COMP_INV_SWITCH_PARAM, PoleDancer::REZ_COMP_INV_SWITCH_LIGHT));

    addParam(createParamCentered<Trimpot>(mm2px(Vec(23.754, 109.044)), module, PoleDancer::DRY_MIX_KNOB_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(35.331, 109.044)), module, PoleDancer::POLE1_MIX_KNOB_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(46.907, 109.044)), module, PoleDancer::POLE2_MIX_KNOB_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(58.483, 109.044)), module, PoleDancer::POLE3_MIX_KNOB_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(70.06, 109.044)), module, PoleDancer::POLE4_MIX_KNOB_PARAM));
    */

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.639, 53.193)), module, PoleDancer::SOURCE_ONE_LEVEL_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.210, 53.193)), module, PoleDancer::SOURCE_ONE_MOD_AMOUNT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.462, 53.194)), module, PoleDancer::SOURCE_TWO_LEVEL_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(68.8, 53.194)), module, PoleDancer::SOURCE_TWO_MOD_AMOUNT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.37, 90.742)), module, PoleDancer::CUTOFF_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.21, 90.742)), module, PoleDancer::RESONANCE_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(68.47, 90.742)), module, PoleDancer::FILTER_VCA_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(68.473, 108.151)), module, PoleDancer::POLE_MIX_INPUT));

    addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(2.02, 8.219)), module, PoleDancer::LEFT_EXPANDER_LIGHT));
    addChild(createLightCentered<TriangleRightLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(79.507, 8.219)), module, PoleDancer::RIGHT_EXPANDER_LIGHT));

    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.687, 45.831)), module, PoleDancer::SOURCE_ONE_LEVEL_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(33.172, 45.831)), module, PoleDancer::SOURCE_ONE_MOD_AMOUNT_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(54.425, 45.831)), module, PoleDancer::SOURCE_TWO_LEVEL_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(72.763, 45.831)), module, PoleDancer::SOURCE_TWO_MOD_AMOUNT_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.332, 83.379)), module, PoleDancer::CUTOFF_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(33.173, 83.379)), module, PoleDancer::RESONANCE_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(72.436, 83.379)), module, PoleDancer::FILTER_VCA_CLIP_LIGHT));

    // Text fields
    source1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(7.185, 13.839)));
    source1NameTextField->box.size = mm2px(Vec(18.388, 3.636));
    source1NameTextField->setText(module ? &module->source1NameString : NULL);
    addChild(source1NameTextField);

    source2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(44.142, 13.839)));
    source2NameTextField->box.size = mm2px(Vec(18.388, 3.636));
    source2NameTextField->setText(module ? &module->source2NameString : NULL);
    addChild(source2NameTextField);

    output1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(56.024, 115.355)));
    output1NameTextField->box.size = mm2px(Vec(20.0, 3.636));
    output1NameTextField->setText(module ? &module->output1NameString : NULL);
    addChild(output1NameTextField);

    output2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(18.135, 115.355)));
    output2NameTextField->box.size = mm2px(Vec(20.0, 3.636));
    output2NameTextField->setText(module ? &module->output2NameString : NULL);
    addChild(output2NameTextField);

    rezCompTextField = createWidget<CardTextDisplay>(mm2px(Vec(7.283, 102.406)));
    rezCompTextField->box.size = mm2px(Vec(25.0, 3.636));
    rezCompTextField->setText(module ? &module->rezCompModeNameString : NULL);
    addChild(rezCompTextField);
  }


  CardTextDisplay *source1NameTextField;
  CardTextDisplay *source2NameTextField;
  CardTextDisplay *output1NameTextField;
  CardTextDisplay *output2NameTextField;
  CardTextDisplay *rezCompTextField;

};


Model* modelPoleDancer = createModel<PoleDancer, PoleDancerWidget>("PoleDancer");
