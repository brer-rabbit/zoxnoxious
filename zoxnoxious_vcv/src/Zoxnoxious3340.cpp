#include "plugin.hpp"
#include "zcomponentlib.hpp"
#include "ZoxnoxiousExpander.hpp"

const static int num_audio_inputs = 6;
const static int midiMessageQueueMaxSize = 16;

struct Zoxnoxious3340 : ZoxnoxiousModule {
    // the ParamId ordering *is* relevant.
    // this order should reflect the channel mapping (WHY?)
    // TODO: fix this, ordering of params shouldn't map to channels
    enum ParamId {
        FREQ_KNOB_PARAM,
        SYNC_PHASE_KNOB_PARAM,
        PULSE_WIDTH_KNOB_PARAM,
        MIX1_TRIANGLE_KNOB_PARAM,
        EXT_MOD_AMOUNT_KNOB_PARAM,
        LINEAR_KNOB_PARAM,
        SYNC_POS_BUTTON_PARAM,
        MIX1_PULSE_BUTTON_PARAM,
        MIX1_SAW_LEVEL_SELECTOR_PARAM,
        EXT_MOD_SELECT_SWITCH_PARAM,
        MIX1_COMPARATOR_BUTTON_PARAM,
        MIX2_PULSE_BUTTON_PARAM,
        EXT_MOD_PWM_BUTTON_PARAM,
        EXP_FM_BUTTON_PARAM,
        LINEAR_FM_BUTTON_PARAM,
        MIX2_SAW_BUTTON_PARAM,
        SYNC_NEG_BUTTON_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        FREQ_INPUT,
        SYNC_PHASE_INPUT,
        PULSE_WIDTH_INPUT,
        MIX1_TRIANGLE_VCA_INPUT,
        EXT_MOD_AMOUNT_INPUT,
        LINEAR_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUTS_LEN
    };
    enum LightId {
        MIX1_PULSE_BUTTON_LIGHT,
        MIX1_COMPARATOR_BUTTON_LIGHT,
        EXT_MOD_PWM_BUTTON_LIGHT,
        EXP_FM_BUTTON_LIGHT,
        LINEAR_FM_BUTTON_LIGHT,
        MIX2_PULSE_BUTTON_LIGHT,
        MIX2_SAW_BUTTON_LIGHT,
        SYNC_NEG_ENABLE_LIGHT,
	FREQ_CLIP_LIGHT,
	PULSE_WIDTH_CLIP_LIGHT,
	LINEAR_CLIP_LIGHT,
	MIX1_TRIANGLE_VCA_CLIP_LIGHT,
	SYNC_PHASE_CLIP_LIGHT,
	EXT_MOD_AMOUNT_CLIP_LIGHT,
        SYNC_POS_ENABLE_LIGHT,
        ENUMS(LEFT_EXPANDER_LIGHT, 3),
        ENUMS(RIGHT_EXPANDER_LIGHT, 3),
        LIGHTS_LEN
    };


    dsp::ClockDivider lightDivider;
    float freqClipTimer;
    float pulseWidthClipTimer;
    float linearClipTimer;
    float mix1TriangleVcaClipTimer;
    float syncPhaseClipTimer;
    float extModAmountClipTimer;

    std::deque<midi::Message> midiMessageQueue;

    std::string output1NameString;
    std::string output2NameString;
    std::string modulationInputNameString;
    int modulationInputParamPrevValue; // detect whether it changed

    // detect state changes so we can send a MIDI event.
    // Assume int_min is an invalid value.  On start, idea would be
    // to send current state via midi to the board is in sync with
    // the rack plugin.
    // The big hack here: a midiProgram is sent, and the program value
    // is indexed based on the button parameter value.
    struct buttonParamMidiProgram {
        enum ParamId button;
        int previousValue;
        uint8_t midiProgram[8];
    } buttonParamToMidiProgramList[11] =
      {
          { SYNC_NEG_BUTTON_PARAM, INT_MIN, { 0, 1 } },
          { MIX1_PULSE_BUTTON_PARAM, INT_MIN, { 2, 3 } },
          { MIX1_COMPARATOR_BUTTON_PARAM, INT_MIN, { 4, 5 } },
          { MIX2_PULSE_BUTTON_PARAM, INT_MIN, { 6, 7 } },
          { EXT_MOD_PWM_BUTTON_PARAM, INT_MIN, { 8, 9 } },
          { EXP_FM_BUTTON_PARAM, INT_MIN, { 10, 11 } },
          { LINEAR_FM_BUTTON_PARAM, INT_MIN, { 12, 13 } },
          { MIX2_SAW_BUTTON_PARAM, INT_MIN, { 14, 15 } },
          { SYNC_POS_BUTTON_PARAM, INT_MIN, { 16, 17 } },
          { MIX1_SAW_LEVEL_SELECTOR_PARAM, INT_MIN, { 18, 19, 20 } },
          { EXT_MOD_SELECT_SWITCH_PARAM, INT_MIN, { 21, 22, 23, 24, 25, 26, 27, 28 } }
      };

    Zoxnoxious3340() :
        freqClipTimer(0.f), pulseWidthClipTimer(0.f), linearClipTimer(0.f),
        mix1TriangleVcaClipTimer(0.f), syncPhaseClipTimer(0.f),
        extModAmountClipTimer(0.f),
        output1NameString(invalidCardOutputName),
        output2NameString(invalidCardOutputName),
        modulationInputNameString(invalidCardOutputName),
        modulationInputParamPrevValue(-1) {

        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(FREQ_KNOB_PARAM, 0.f, 1.f, 0.5f, "Frequency", " V", 0.f, 10.f);
        configParam(PULSE_WIDTH_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pulse Width", "%", 0.f, 100.f);
        configParam(LINEAR_KNOB_PARAM, 0.f, 1.f, 0.5f, "Linear Mod", " V", 0.f, 10.f, -5.f);

        configSwitch(MIX1_PULSE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Pulse", {"Off", "On"});
        configParam(MIX1_TRIANGLE_KNOB_PARAM, 0.f, 1.f, 0.f, "Mix1 Triangle Level", "%", 0.f, 100.f);
        configSwitch(MIX1_SAW_LEVEL_SELECTOR_PARAM, 0.f, 3.f, 0.f, "Level", {"Off", "Low", "Med", "High" });
        configSwitch(MIX1_COMPARATOR_BUTTON_PARAM, 0.f, 1.f, 0.f, "Mix1 Comparator", {"Off", "On"});

        configSwitch(MIX2_PULSE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Mix2 Pulse", {"Off", "On"});
        configSwitch(MIX2_SAW_BUTTON_PARAM, 0.f, 1.f, 0.f, "Mix2 Saw", {"Off", "On"});

        configSwitch(EXT_MOD_SELECT_SWITCH_PARAM, 0.f, 7.f, 0.f, "Ext Signal", {"A1", "A2", "B1", "B2", "C1", "C2", "D1", "D2" });
        configParam(EXT_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 1.f, "External Mod Level", "%", 0.f, 100.f);
        configSwitch(EXT_MOD_PWM_BUTTON_PARAM, 0.f, 1.f, 0.f, "Ext Mod to PWM", {"Off", "On"});
        configSwitch(EXP_FM_BUTTON_PARAM, 0.f, 1.f, 0.f, "Ext Mod to Exp FM", {"Off", "On"});
        configSwitch(LINEAR_FM_BUTTON_PARAM, 0.f, 1.f, 0.f, "Ext Mod to Linear FM", {"Off", "On"});

        configParam(SYNC_PHASE_KNOB_PARAM, 0.f, 1.f, 0.5f, "Sync Phase", "%", 0.f, 100.f);
        configSwitch(SYNC_NEG_BUTTON_PARAM, 0.f, 1.f, 0.f, "Neg", {"Off", "On"});
        configSwitch(SYNC_POS_BUTTON_PARAM, 0.f, 1.f, 0.f, "Pos", {"Off", "On"});

        configInput(SYNC_PHASE_INPUT, "Sync Phase");
        configInput(FREQ_INPUT, "Pitch CV");
        configInput(PULSE_WIDTH_INPUT, "Pulse Width Modulation");
        configInput(LINEAR_INPUT, "Linear FM");
        configInput(MIX1_TRIANGLE_VCA_INPUT, "Mix1 Triangle Level");
        configInput(EXT_MOD_AMOUNT_INPUT, "External Modulation Amount");

        configLight(LEFT_EXPANDER_LIGHT, "Connection Status");
        configLight(RIGHT_EXPANDER_LIGHT, "Connection Status");

        lightDivider.setDivision(512);
    }




    void process(const ProcessArgs& args) override {
        processExpander(args);

        if (lightDivider.process()) {
            bool sync_neg = params[SYNC_NEG_BUTTON_PARAM].getValue() > 0.f;
            lights[SYNC_NEG_ENABLE_LIGHT].setBrightness(sync_neg);

            bool sync_pos = params[SYNC_POS_BUTTON_PARAM].getValue() > 0.f;
            lights[SYNC_POS_ENABLE_LIGHT].setBrightness(sync_pos);

            bool mix1_pulse = params[MIX1_PULSE_BUTTON_PARAM].getValue() > 0.f;
            lights[MIX1_PULSE_BUTTON_LIGHT].setBrightness(mix1_pulse);

            bool mix1_comparator = params[MIX1_COMPARATOR_BUTTON_PARAM].getValue() > 0.f;
            lights[MIX1_COMPARATOR_BUTTON_LIGHT].setBrightness(mix1_comparator);

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


            // clipping light timer
            const float lightTime = args.sampleTime * lightDivider.getDivision();
            const float brightnessDeltaTime = 1 / lightTime;

            freqClipTimer -= lightTime;
            lights[FREQ_CLIP_LIGHT].setBrightnessSmooth(freqClipTimer > 0.f, brightnessDeltaTime);

            pulseWidthClipTimer -= lightTime;
            lights[PULSE_WIDTH_CLIP_LIGHT].setBrightnessSmooth(pulseWidthClipTimer > 0.f, brightnessDeltaTime);

            linearClipTimer -= lightTime;
            lights[LINEAR_CLIP_LIGHT].setBrightnessSmooth(linearClipTimer > 0.f, brightnessDeltaTime);

            mix1TriangleVcaClipTimer -= lightTime;
            lights[MIX1_TRIANGLE_VCA_CLIP_LIGHT].setBrightnessSmooth(mix1TriangleVcaClipTimer > 0.f, brightnessDeltaTime);

            syncPhaseClipTimer -= lightTime;
            lights[SYNC_PHASE_CLIP_LIGHT].setBrightnessSmooth(syncPhaseClipTimer > 0.f, brightnessDeltaTime);

            extModAmountClipTimer -= lightTime;
            lights[EXT_MOD_AMOUNT_CLIP_LIGHT].setBrightnessSmooth(extModAmountClipTimer > 0.f, brightnessDeltaTime);

            setLeftExpanderLight(LEFT_EXPANDER_LIGHT);
            setRightExpanderLight(RIGHT_EXPANDER_LIGHT);

            // TODO: rework this without string copy.
            // Could do something like:
            // if (params[EXT_MOD_SELECT_SWITCH_PARAM].getValue() != modulationInputParamPrevValue)...
            // but then the field isn't populated on connection.  sigh.
            //
            // oddly (well, by design) this is how the mux is wired up:
            // CardA_Out1, CardA_Out2, CardB_Out1, CardC_Out1,
            // CardD_Out1, CardE_Out1, CardF_Out1, CardG_Out1,
            int modulationParam = static_cast<int>(params[EXT_MOD_SELECT_SWITCH_PARAM].getValue());
            // this handles the cases 0-7 for the mux
            if (modulationParam > 2) {
                modulationParam = (modulationParam - 1) * 2;
            }

            if (modulationParam != modulationInputParamPrevValue) {
                modulationInputNameString = cardOutputNames[modulationParam];
                // send midi command
            }
        }
    }



    /** processZoxnoxiousControl
     *
     * add our control voltage values to the control message.  Add or queue any MIDI message.
     *
     */
    void processZoxnoxiousControl(ZoxnoxiousControlMsg *controlMsg) override {

        if (!hasChannelAssignment) {
            return;
        }

        // if we have any queued midi messages, send them if possible
        if (controlMsg->midiMessageSet == false && midiMessageQueue.size() > 0) {
            //INFO("zoxnoxious3340: clock %" PRId64 " : bus is open, popping MIDI message from queue", APP->engine->getFrame());
            controlMsg->midiMessageSet = true;
            controlMsg->midiMessage = midiMessageQueue.front();
            midiMessageQueue.pop_front();
        }


        // Any buttons params pushed need to send midi events.  Send directly or queue.
        for (int i = 0; i < (int) (sizeof(buttonParamToMidiProgramList) / sizeof(struct buttonParamMidiProgram)); ++i) {
            int newValue = (int) (params[ buttonParamToMidiProgramList[i].button ].getValue() + 0.5f);

            if (buttonParamToMidiProgramList[i].previousValue != newValue) {
                buttonParamToMidiProgramList[i].previousValue = newValue;
                if (controlMsg->midiMessageSet == false) {
                    // send direct
                    controlMsg->midiMessage.setSize(2);
                    controlMsg->midiMessage.setChannel(midiChannel);
                    controlMsg->midiMessage.setStatus(midiProgramChangeStatus);
                    controlMsg->midiMessage.setNote(buttonParamToMidiProgramList[i].midiProgram[newValue]);
                    controlMsg->midiMessageSet = true;
                    INFO("zoxnoxious3340: clock %" PRId64 " :  MIDI message direct midi channel %d", APP->engine->getFrame(), midiChannel);
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
                    INFO("Zoxnoxioius3340: dropping MIDI message, bus full and queue full");
                }
            }
        }

        float v;
        const float clipTime = 0.25f;

        // linear
        v = params[LINEAR_KNOB_PARAM].getValue() + inputs[LINEAR_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[cvChannelOffset + 5] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[cvChannelOffset + 5] != v) {
            linearClipTimer = clipTime;
        }
                    
        // external mod amount
        v = params[EXT_MOD_AMOUNT_KNOB_PARAM].getValue() + inputs[EXT_MOD_AMOUNT_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[cvChannelOffset + 4] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[cvChannelOffset + 4] != v) {
            extModAmountClipTimer = clipTime;
        }

        // mix1 triangle
        v = params[MIX1_TRIANGLE_KNOB_PARAM].getValue() + inputs[MIX1_TRIANGLE_VCA_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[cvChannelOffset + 3] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[cvChannelOffset + 3] != v) {
            mix1TriangleVcaClipTimer = clipTime;
        }
            
        // pulse width
        v = params[PULSE_WIDTH_KNOB_PARAM].getValue() + inputs[PULSE_WIDTH_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[cvChannelOffset + 2] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[cvChannelOffset + 2] != v) {
            pulseWidthClipTimer = clipTime;
        }

        // sync phase
        v = params[SYNC_PHASE_KNOB_PARAM].getValue() + inputs[SYNC_PHASE_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[cvChannelOffset + 1] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[cvChannelOffset + 1] != v) {
            syncPhaseClipTimer = clipTime;
        }

        // frequency
        v = params[FREQ_KNOB_PARAM].getValue() + inputs[FREQ_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[cvChannelOffset + 0] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[cvChannelOffset + 0] != v) {
            freqClipTimer = clipTime;
        }
    }



    /** getCardHardwareId
     * return the hardware Id of the 3340 card
     */
    static const uint8_t hardwareId = 0x02;
    uint8_t getHardwareId() override {
        return hardwareId;
    }


    void onChannelAssignmentEstablished(ZoxnoxiousCommandMsg *zCommand) override {
        ZoxnoxiousModule::onChannelAssignmentEstablished(zCommand);
        output1NameString = getCardOutputName(hardwareId, 1, slot);
        output2NameString = getCardOutputName(hardwareId, 2, slot);
    }

    void onChannelAssignmentLost() override {
        ZoxnoxiousModule::onChannelAssignmentLost();
        output1NameString = invalidCardOutputName;
        output2NameString = invalidCardOutputName;
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

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.626, 28.981)), module, Zoxnoxious3340::FREQ_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(43.276, 28.981)), module, Zoxnoxious3340::PULSE_WIDTH_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(26.663, 28.981)), module, Zoxnoxious3340::LINEAR_KNOB_PARAM));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(62.243, 27.701)), module, Zoxnoxious3340::MIX1_PULSE_BUTTON_PARAM, Zoxnoxious3340::MIX1_PULSE_BUTTON_LIGHT));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.321, 43.444)), module, Zoxnoxious3340::MIX1_TRIANGLE_KNOB_PARAM));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(39.49, 92.628)), module, Zoxnoxious3340::SYNC_PHASE_KNOB_PARAM));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(33.559, 68.099)), module, Zoxnoxious3340::SYNC_POS_BUTTON_PARAM, Zoxnoxious3340::SYNC_POS_ENABLE_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(45.19, 68.099)), module, Zoxnoxious3340::SYNC_NEG_BUTTON_PARAM, Zoxnoxious3340::SYNC_NEG_ENABLE_LIGHT));


        addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(62.488, 69.012)), module, Zoxnoxious3340::MIX1_SAW_LEVEL_SELECTOR_PARAM));

        addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(10.627, 64.985)), module, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_PARAM));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(62.488, 86.757)), module, Zoxnoxious3340::MIX1_COMPARATOR_BUTTON_PARAM, Zoxnoxious3340::MIX1_COMPARATOR_BUTTON_LIGHT));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.175, 92.628)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_KNOB_PARAM));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(62.083, 105.766)), module, Zoxnoxious3340::MIX2_PULSE_BUTTON_PARAM, Zoxnoxious3340::MIX2_PULSE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(10.626, 118.524)), module, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_PARAM, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(26.663, 118.524)), module, Zoxnoxious3340::EXP_FM_BUTTON_PARAM, Zoxnoxious3340::EXP_FM_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(43.276, 118.524)), module, Zoxnoxious3340::LINEAR_FM_BUTTON_PARAM, Zoxnoxious3340::LINEAR_FM_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(62.488, 118.524)), module, Zoxnoxious3340::MIX2_SAW_BUTTON_PARAM, Zoxnoxious3340::MIX2_SAW_BUTTON_LIGHT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.626, 38.646)), module, Zoxnoxious3340::FREQ_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(43.276, 38.646)), module, Zoxnoxious3340::PULSE_WIDTH_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(26.663, 38.646)), module, Zoxnoxious3340::LINEAR_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.488, 54.616)), module, Zoxnoxious3340::MIX1_TRIANGLE_VCA_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(39.49, 102.293)), module, Zoxnoxious3340::SYNC_PHASE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.175, 102.293)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_INPUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.626, 19.897)), module, Zoxnoxious3340::FREQ_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(43.276, 19.897)), module, Zoxnoxious3340::PULSE_WIDTH_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(26.663, 19.897)), module, Zoxnoxious3340::LINEAR_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(55.19, 50.994)), module, Zoxnoxious3340::MIX1_TRIANGLE_VCA_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(39.49, 83.544)), module, Zoxnoxious3340::SYNC_PHASE_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(14.175, 83.544)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_CLIP_LIGHT));

        addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(2.02, 8.219)), module, Zoxnoxious3340::LEFT_EXPANDER_LIGHT));
        addChild(createLightCentered<TriangleRightLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(74.427, 8.219)), module, Zoxnoxious3340::RIGHT_EXPANDER_LIGHT));

        mix1OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(52.791, 12.989)));
        mix1OutputTextField->box.size = (mm2px(Vec(19.124, 3.636)));
        mix1OutputTextField->setText(module ? &module->output1NameString : NULL);
        addChild(mix1OutputTextField);

        mix2OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(52.791, 94.213)));
        mix2OutputTextField->box.size = (mm2px(Vec(19.124, 3.636)));
        mix2OutputTextField->setText(module ? &module->output2NameString : NULL);
        addChild(mix2OutputTextField);

        modulationInputTextField = createWidget<CardTextDisplay>(mm2px(Vec(6.286, 53.135)));
        modulationInputTextField->box.size = (mm2px(Vec(22.438, 3.636)));
        modulationInputTextField->setText(module ? &module->modulationInputNameString  : NULL);
        addChild(modulationInputTextField);
    }

    CardTextDisplay *mix1OutputTextField;
    CardTextDisplay *mix2OutputTextField;
    CardTextDisplay *modulationInputTextField;

};


Model* modelZoxnoxious3340 = createModel<Zoxnoxious3340, Zoxnoxious3340Widget>("Zoxnoxious3340");
