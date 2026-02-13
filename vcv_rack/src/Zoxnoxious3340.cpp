#include "plugin.hpp"
#include "common.hpp"
#include "zcomponentlib.hpp"
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


    float syncPhaseClipTimer;
    float freqClipTimer;
    float mix1PulseVcaClipTimer;
    float extModAmountClipTimer;
    float mix1TriangleVcaClipTimer;
    float mix1SawVcaClipTimer;
    float pulseWidthClipTimer;
    float linearClipTimer;

    std::deque<midi::Message> midiMessageQueue;

    std::string output1NameString;
    std::string output2NameString;
    std::string modulationInputNameString;
    int modulationInputParamPrevValue; // detect whether it changed

    // detect state changes so we can send a MIDI event.
    // Assume int_min is an invalid value.  On start, idea would be
    // to send current state via midi so the board is in sync with
    // the rack plugin.
    // The program value to send over midi  is indexed based on the
    // button parameter value.
    // The order must agree with the z3340 rpi driver
    struct buttonParamMidiProgram {
        enum ParamId button;
        int previousValue;
        uint8_t midiProgram[13];
    } buttonParamToMidiProgramList[10] =
      {
          { SYNC_HARD_BUTTON_PARAM, INT_MIN, { 0, 1 } },
          { EXT_MOD_PWM_BUTTON_PARAM, INT_MIN, { 2, 3 } },
          { SYNC_NEG_BUTTON_PARAM, INT_MIN, { 4, 5 } },
          { SYNC_SOFT_BUTTON_PARAM, INT_MIN, { 6, 7 } },
          { SYNC_POS_BUTTON_PARAM, INT_MIN, { 8, 9 } },
          { LINEAR_FM_BUTTON_PARAM, INT_MIN, { 10, 11 } },
          { MIX2_SAW_BUTTON_PARAM, INT_MIN, { 12, 13 } },
          { MIX2_PULSE_BUTTON_PARAM, INT_MIN, { 14, 15 } },
          { EXP_FM_BUTTON_PARAM, INT_MIN, { 16, 17 } },
          { EXT_MOD_SELECT_SWITCH_UP_PARAM, INT_MIN, { 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 } } // special handling
      };

    // index of EXT_MOD_SELECT_SWITCH_UP_PARAM
    const int extModSelectSwitchIndex =
      sizeof(buttonParamToMidiProgramList) / sizeof(struct buttonParamMidiProgram) - 1;

    ParticipantLifecycle lifecycle;

    int extModSelectSwitchValue;
    bool extModSelectChanged;

    Zoxnoxious3340() :
        syncPhaseClipTimer(0.f), freqClipTimer(0.f),
        mix1PulseVcaClipTimer(0.f), extModAmountClipTimer(0.f),
        mix1TriangleVcaClipTimer(0.f), mix1SawVcaClipTimer(0.f),
        pulseWidthClipTimer(0.f), linearClipTimer(0.f),
        output1NameString(invalidCardOutputName),
        output2NameString(invalidCardOutputName),
        modulationInputNameString(invalidCardOutputName),
        modulationInputParamPrevValue(-1),
        extModSelectSwitchValue(0),
        extModSelectChanged(false) {

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

    }



/*
    void process(const ProcessArgs& args) override {

        //setLeftExpanderLight(LEFT_EXPANDER_LIGHT);
        //setRightExpanderLight(RIGHT_EXPANDER_LIGHT);

        // TODO: rework this without string copy.
        // Could do something like:
        // if (params[EXT_MOD_SELECT_SWITCH_PARAM].getValue() != modulationInputParamPrevValue)...
        // but then the field isn't populated on connection.  sigh.
        //
        // oddly (well, by design) this is how the mux is wired up:
        // CardA_Out1, CardA_Out2, CardB_Out1, CardC_Out1,
        // CardD_Out1, CardE_Out1, CardF_Out1, CardG_Out1,

        //modulationInputNameString = cardOutputNames[extModSelectSwitchValue];
      }
    }
*/



    /* Participant interface */
    int64_t getModuleId() override {
      return getId();
    }

    void pullSamples(const rack::engine::Module::ProcessArgs &args, dsp::Frame<maxAudioChannels> &sharedFrame, int offset) override {
    }

    bool pullMidi(const rack::engine::Module::ProcessArgs &args, uint32_t clockDivision, int midiChannel, midi::Message &midiMessage) override {
      INFO("zoxnoxious3340 %" PRId64 " pulling midi", id);



      // UI related updates:

      // Lights - we need lights.
      // First, button lights.
      bool sync_pos = params[SYNC_POS_BUTTON_PARAM].getValue() > 0.f;
      lights[SYNC_POS_BUTTON_LIGHT].setBrightness(sync_pos);

      bool mix2_pulse = params[MIX2_PULSE_BUTTON_PARAM].getValue() > 0.f;
      lights[MIX2_PULSE_BUTTON_LIGHT].setBrightness(mix2_pulse);

      bool ext_mod_pwm = params[EXT_MOD_PWM_BUTTON_PARAM].getValue() > 0.f;
      lights[EXT_MOD_PWM_BUTTON_LIGHT].setBrightness(ext_mod_pwm);

      bool exp_fm = params[EXP_FM_BUTTON_PARAM].getValue() > 0.f;
      lights[EXP_FM_BUTTON_LIGHT].setBrightness(exp_fm);

      bool linear_fm = params[LINEAR_FM_BUTTON_PARAM].getValue() > 0.f;
      lights[LINEAR_FM_BUTTON_LIGHT].setBrightness(linear_fm);

      bool mix2_saw = params[MIX2_SAW_BUTTON_PARAM].getValue() > 0.f;
      lights[MIX2_SAW_BUTTON_LIGHT].setBrightness(mix2_saw);

      bool sync_neg = params[SYNC_NEG_BUTTON_PARAM].getValue() > 0.f;
      lights[SYNC_NEG_BUTTON_LIGHT].setBrightness(sync_neg);

      bool sync_hard = params[SYNC_HARD_BUTTON_PARAM].getValue() > 0.f;
      lights[SYNC_HARD_BUTTON_LIGHT].setBrightness(sync_hard);

      bool sync_soft = params[SYNC_SOFT_BUTTON_PARAM].getValue() > 0.f;
      lights[SYNC_SOFT_BUTTON_LIGHT].setBrightness(sync_soft);

      lights[EXT_MOD_SELECT_SWITCH_UP_LIGHT].setBrightness(params[EXT_MOD_SELECT_SWITCH_UP_PARAM].getValue());
      lights[EXT_MOD_SELECT_SWITCH_DOWN_LIGHT].setBrightness(params[EXT_MOD_SELECT_SWITCH_DOWN_PARAM].getValue());

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

      return false;
    }




    /** getCardHardwareId
     * return the hardware Id of the 3340 card
     * This MUST match the ROM Id on the card
     */
    static const uint8_t hardwareId = 0x02;
    uint8_t getHardwareId() const override {
        return hardwareId;
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

      addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
      addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
      addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
      addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

      addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(9.7834, 24.553)), module, Zoxnoxious3340::FREQ_KNOB_PARAM));
      addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(43.275, 24.553)), module, Zoxnoxious3340::PULSE_WIDTH_KNOB_PARAM));
      addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(27.020, 24.553)), module, Zoxnoxious3340::LINEAR_KNOB_PARAM));

//        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(62.243, 27.700)), module, Zoxnoxious3340::MIX1_PULSE_BUTTON_PARAM, Zoxnoxious3340::MIX1_PULSE_BUTTON_LIGHT));

      addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.243, 24.584)), module, Zoxnoxious3340::MIX1_PULSE_KNOB_PARAM));
      addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.314, 51.003)), module, Zoxnoxious3340::MIX1_TRIANGLE_KNOB_PARAM));
      addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.300, 78.103)), module, Zoxnoxious3340::MIX1_SAW_KNOB_PARAM));

      addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(39.480, 87.501)), module, Zoxnoxious3340::SYNC_PHASE_KNOB_PARAM));

      addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(33.559, 62.255)), module, Zoxnoxious3340::SYNC_HARD_BUTTON_PARAM, Zoxnoxious3340::SYNC_HARD_BUTTON_LIGHT));
      addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(45.19, 62.255)), module, Zoxnoxious3340::SYNC_SOFT_BUTTON_PARAM, Zoxnoxious3340::SYNC_SOFT_BUTTON_LIGHT));

      addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(33.559, 72.994)), module, Zoxnoxious3340::SYNC_POS_BUTTON_PARAM, Zoxnoxious3340::SYNC_POS_BUTTON_LIGHT));
      addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(45.19, 72.995)), module, Zoxnoxious3340::SYNC_NEG_BUTTON_PARAM, Zoxnoxious3340::SYNC_NEG_BUTTON_LIGHT));


//        addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(62.487, 69.011)), module, Zoxnoxious3340::MIX1_SAW_LEVEL_SELECTOR_PARAM));

      addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(21.625, 64.0)), module, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_UP_PARAM, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_UP_LIGHT));
      addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(9.994, 64.0)), module, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_DOWN_PARAM, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_DOWN_LIGHT));

      addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.184, 87.501)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_KNOB_PARAM));
      addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(57.112, 118.52)), module, Zoxnoxious3340::MIX2_PULSE_BUTTON_PARAM, Zoxnoxious3340::MIX2_PULSE_BUTTON_LIGHT));
      addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(10.626, 118.52)), module, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_PARAM, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_LIGHT));
      addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(26.663, 118.52)), module, Zoxnoxious3340::EXP_FM_BUTTON_PARAM, Zoxnoxious3340::EXP_FM_BUTTON_LIGHT));
      addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(43.275, 118.52)), module, Zoxnoxious3340::LINEAR_FM_BUTTON_PARAM, Zoxnoxious3340::LINEAR_FM_BUTTON_LIGHT));
      addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(68.019, 118.52)), module, Zoxnoxious3340::MIX2_SAW_BUTTON_PARAM, Zoxnoxious3340::MIX2_SAW_BUTTON_LIGHT));

      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.7834, 37.915)), module, Zoxnoxious3340::FREQ_INPUT));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(43.275, 37.915)), module, Zoxnoxious3340::PULSE_WIDTH_INPUT));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(27.020, 37.915)), module, Zoxnoxious3340::LINEAR_INPUT));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.314, 36.719)), module, Zoxnoxious3340::MIX1_PULSE_VCA_INPUT));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.314, 63.048)), module, Zoxnoxious3340::MIX1_TRIANGLE_VCA_INPUT));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.314, 91.137)), module, Zoxnoxious3340::MIX1_SAW_VCA_INPUT));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(39.480, 101.925)), module, Zoxnoxious3340::SYNC_PHASE_INPUT));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.184, 101.924)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_INPUT));

      addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(13.783, 30.553)), module, Zoxnoxious3340::FREQ_CLIP_LIGHT));
      addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(47.793, 30.553)), module, Zoxnoxious3340::PULSE_WIDTH_CLIP_LIGHT));
      addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(31.020, 30.553)), module, Zoxnoxious3340::LINEAR_CLIP_LIGHT));

      addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(66.313, 31.761)), module, Zoxnoxious3340::MIX1_PULSE_CLIP_LIGHT));
      addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(66.313, 57.003)), module, Zoxnoxious3340::MIX1_TRIANGLE_CLIP_LIGHT));
      addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(66.313, 85.095)), module, Zoxnoxious3340::MIX1_SAW_CLIP_LIGHT));
      addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(43.480, 93.501)), module, Zoxnoxious3340::SYNC_PHASE_CLIP_LIGHT));
      addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(18.184, 93.501)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_CLIP_LIGHT));

      addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(2.020, 8.2185)), module, Zoxnoxious3340::LEFT_EXPANDER_LIGHT));
      addChild(createLightCentered<TriangleRightLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(74.427, 8.219)), module, Zoxnoxious3340::RIGHT_EXPANDER_LIGHT));

      mix1OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(53.378, 12.989)));
      mix1OutputTextField->box.size = (mm2px(Vec(18.5, 3.636)));
      mix1OutputTextField->setText(module ? &module->output1NameString : NULL);
      addChild(mix1OutputTextField);

      mix2OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(53.250, 102.833)));
      mix2OutputTextField->box.size = (mm2px(Vec(18.5, 3.636)));
      mix2OutputTextField->setText(module ? &module->output2NameString : NULL);
      addChild(mix2OutputTextField);

      modulationInputTextField = createWidget<CardTextDisplay>(mm2px(Vec(6.0, 56.119)));
      modulationInputTextField->box.size = (mm2px(Vec(20.0, 3.636)));
      modulationInputTextField->setText(module ? &module->modulationInputNameString  : NULL);
      addChild(modulationInputTextField);
    }

    CardTextDisplay *mix1OutputTextField;
    CardTextDisplay *mix2OutputTextField;
    CardTextDisplay *modulationInputTextField;

  };

} // namespace zox

Model* modelZoxnoxious3340 = createModel<zox::Zoxnoxious3340, zox::Zoxnoxious3340Widget>("Zoxnoxious3340");

