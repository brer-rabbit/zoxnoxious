#include "plugin.hpp"
#include "zcomponentlib.hpp"
#include "ZoxnoxiousExpander.hpp"

const static int midiMessageQueueMaxSize = 16;

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
int source1Sources[] = { 0, 1, 3, 4, 7, 8, 11, 12 };

// card6 out1
// card5 out2
// card4 out1
// card3 out2
// card2 out2
// card2 out1
// card1 out2
// card1 out1
int source2Sources[] = { 0, 1, 2, 3, 5, 6, 9, 10 };


static const float noiseVcaThresholdGateLevel = 0.f;

struct Zoxnoxious3372 : ZoxnoxiousModule {
    enum ParamId {
        CUTOFF_KNOB_PARAM,
        OUTPUT_PAN_KNOB_PARAM,
        MOD_AMOUNT_KNOB_PARAM,
        SOURCE_ONE_LEVEL_KNOB_PARAM,
        SOURCE_TWO_LEVEL_KNOB_PARAM,
        RESONANCE_KNOB_PARAM,
        VCA_MOD_SWITCH_PARAM,
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
        VCA_MOD_ENABLE_LIGHT,
        FILTER_MOD_ENABLE_LIGHT,
        REZ_MOD_ENABLE_LIGHT,
        SOURCE_ONE_DOWN_BUTTON_LIGHT,
        SOURCE_ONE_UP_BUTTON_LIGHT,
        SOURCE_TWO_DOWN_BUTTON_LIGHT,
        SOURCE_TWO_UP_BUTTON_LIGHT,
        ENUMS(LEFT_EXPANDER_LIGHT, 3),
        ENUMS(RIGHT_EXPANDER_LIGHT, 3),
        LIGHTS_LEN
    };


    dsp::ClockDivider lightDivider;
    float modAmountClipTimer;
    float sourceOneLevelClipTimer;
    float sourceTwoLevelClipTimer;
    float outputPanClipTimer;
    float cutoffClipTimer;
    float resonanceClipTimer;

    std::deque<midi::Message> midiMessageQueue;

    std::string source1NameString;
    std::string source2NameString;
    std::string output1NameString;
    std::string output2NameString;

    // detect state changes so we can send a MIDI event.
    struct buttonParamMidiProgram {
        enum ParamId button;
        int previousValue;
        uint8_t midiProgram[8];
    } buttonParamToMidiProgramList[5] =
      {
          { FILTER_MOD_SWITCH_PARAM, INT_MIN, { 0, 1 } },
          { VCA_MOD_SWITCH_PARAM, INT_MIN, { 2, 3 } },
          { SOURCE_ONE_VALUE_HIDDEN_PARAM, INT_MIN, { 4, 5, 6, 7, 8, 9, 10, 11 } },
          { SOURCE_TWO_VALUE_HIDDEN_PARAM, INT_MIN, { 12, 13, 14, 15, 16, 17, 18, 19 } },
          { REZ_MOD_SWITCH_PARAM, INT_MIN, { 20, 21 } }
      };

    // midi program changes for enable/disable noise are not mapped to switches
    // these follow sequentially from the end of the buttonParamToMidiProgramList
    static const int disable_noise_switch = 22;
    static const int enable_noise_switch = 23;
    bool noise_enabled_prev_state = false;
    bool noise_enabled = false;

    Zoxnoxious3372() :
        modAmountClipTimer(0.f), sourceOneLevelClipTimer(0.f), sourceTwoLevelClipTimer(0.f),
        outputPanClipTimer(0.f), cutoffClipTimer(0.f), resonanceClipTimer(0.f),
        source1NameString(invalidCardOutputName), source2NameString(invalidCardOutputName),
        output1NameString(invalidCardOutputName), output2NameString(invalidCardOutputName) {

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
        configParam(CUTOFF_KNOB_PARAM, 0.f, 1.f, 1.f, "Cutoff", "%", 0.f, 100.f);
        configParam(RESONANCE_KNOB_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);

        configSwitch(FILTER_MOD_SWITCH_PARAM, 0.f, 1.f, 0.f, "Filter Mod", {"Off", "On"});
        configSwitch(VCA_MOD_SWITCH_PARAM, 0.f, 1.f, 0.f, "VCA Mod", {"Off", "On"});
        configSwitch(REZ_MOD_SWITCH_PARAM, 0.f, 1.f, 0.f, "Rez Mod", {"Off", "On"});

        configInput(MOD_AMOUNT_INPUT, "Modulation Amount");
        configInput(NOISE_LEVEL_INPUT, "Noise Level");
        configInput(SOURCE_ONE_LEVEL_INPUT, "Source One Level");
        configInput(SOURCE_TWO_LEVEL_INPUT, "Source Two Level");
        configInput(OUTPUT_PAN_INPUT, "Pan");
        configInput(CUTOFF_INPUT, "Cutoff");
        configInput(RESONANCE_INPUT, "Resonance");

        // no UI elements for these
        configSwitch(SOURCE_ONE_VALUE_HIDDEN_PARAM, 0.f, 7.f, 0.f, "Source One", {"0", "1", "2", "3", "4", "5", "6", "7"} );
        configSwitch(SOURCE_TWO_VALUE_HIDDEN_PARAM, 0.f, 7.f, 0.f, "Source Two", {"0", "1", "2", "3", "4", "5", "6", "7"} );

        lightDivider.setDivision(512);
    }

    void process(const ProcessArgs& args) override {
        processExpander(args);


        if (lightDivider.process()) {
            // buttons
            lights[VCA_MOD_ENABLE_LIGHT].setBrightness( params[VCA_MOD_SWITCH_PARAM].getValue() > 0.f );
            lights[FILTER_MOD_ENABLE_LIGHT].setBrightness( params[FILTER_MOD_SWITCH_PARAM].getValue() > 0.f );
            lights[REZ_MOD_ENABLE_LIGHT].setBrightness( params[REZ_MOD_SWITCH_PARAM].getValue() > 0.f );


            lights[SOURCE_ONE_DOWN_BUTTON_LIGHT].setBrightness(params[SOURCE_ONE_DOWN_BUTTON_PARAM].getValue());
            lights[SOURCE_ONE_UP_BUTTON_LIGHT].setBrightness(params[SOURCE_ONE_UP_BUTTON_PARAM].getValue());
            lights[SOURCE_TWO_DOWN_BUTTON_LIGHT].setBrightness(params[SOURCE_TWO_DOWN_BUTTON_PARAM].getValue());
            lights[SOURCE_TWO_UP_BUTTON_LIGHT].setBrightness(params[SOURCE_TWO_UP_BUTTON_PARAM].getValue());

            // clipping
            const float lightTime = args.sampleTime * lightDivider.getDivision();
            const float brightnessDeltaTime = 1 / lightTime;

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


        float v;
        const float clipTime = 0.25f;
        int channel = 0;

        // cutoff
        v = params[CUTOFF_KNOB_PARAM].getValue() + inputs[CUTOFF_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + channel] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + channel] != v) {
            cutoffClipTimer = clipTime;
        }

        channel++;
        v = params[OUTPUT_PAN_KNOB_PARAM].getValue() + inputs[OUTPUT_PAN_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + channel] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + channel] != v) {
            outputPanClipTimer = clipTime;
        }

        channel++;
        v = params[NOISE_KNOB_PARAM].getValue() + inputs[NOISE_LEVEL_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + channel] = clamp(v, 0.f, 1.f);
        noise_enabled = controlMsg->frame[outputDeviceId][cvChannelOffset + channel] > noiseVcaThresholdGateLevel ? true : false;
        // no clip LED for noise level

        channel++;
        v = params[RESONANCE_KNOB_PARAM].getValue() + inputs[RESONANCE_INPUT].getVoltageSum() / 10.f;
        v = v < 0.8f ? v * 0.6f : 2.6f * v - 1.6f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + channel] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + channel] != v) {
            resonanceClipTimer = clipTime;
        }

        channel++;
        v = params[SOURCE_ONE_LEVEL_KNOB_PARAM].getValue() + inputs[SOURCE_ONE_LEVEL_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + channel] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + channel] != v) {
            sourceOneLevelClipTimer = clipTime;
        }

        channel++;
        v = params[SOURCE_TWO_LEVEL_KNOB_PARAM].getValue() + inputs[SOURCE_TWO_LEVEL_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + channel] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + channel] != v) {
            sourceTwoLevelClipTimer = clipTime;
        }

        channel++;
        v = params[MOD_AMOUNT_KNOB_PARAM].getValue() + inputs[MOD_AMOUNT_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + channel] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + channel] != v) {
            modAmountClipTimer = clipTime;
        }



        // if we have any queued midi messages, send them if possible
        if (controlMsg->midiMessageSet == false) {
            if (midiMessageQueue.size() > 0) {
                //INFO("z3372: clock %" PRId64 " : bus is open, popping MIDI message from queue", APP->engine->getFrame());
                controlMsg->midiMessageSet = true;
                controlMsg->midiMessage = midiMessageQueue.front();
                midiMessageQueue.pop_front();
            }
            else if (noise_enabled != noise_enabled_prev_state) {
                controlMsg->midiMessage.setSize(2);
                controlMsg->midiMessage.setChannel(midiChannel);
                controlMsg->midiMessage.setStatus(midiProgramChangeStatus);
                controlMsg->midiMessage.setNote(noise_enabled ? enable_noise_switch : disable_noise_switch);
                controlMsg->midiMessageSet = true;
                INFO("zoxnoxious3372: clock %" PRId64 " :  noise switch toggle MIDI message direct midi channel %d", APP->engine->getFrame(), midiChannel);
                noise_enabled_prev_state = noise_enabled;
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
                    INFO("zoxnoxious3372: clock %" PRId64 " :  MIDI message direct midi channel %d", APP->engine->getFrame(), midiChannel);
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

    }


    /** getCardHardwareId
     * return the hardware Id of the 3340 card
     */
    static const uint8_t hardwareId = 0x03;
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
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(39.1350, 69.9388)), module, Zoxnoxious3372::CUTOFF_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(64.4240, 70.2092)), module, Zoxnoxious3372::RESONANCE_KNOB_PARAM));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(64.424, 21.475)), module, Zoxnoxious3372::FILTER_MOD_SWITCH_PARAM, Zoxnoxious3372::FILTER_MOD_ENABLE_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(64.424, 33.934)), module, Zoxnoxious3372::REZ_MOD_SWITCH_PARAM, Zoxnoxious3372::REZ_MOD_ENABLE_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(64.424, 45.839)), module, Zoxnoxious3372::VCA_MOD_SWITCH_PARAM, Zoxnoxious3372::VCA_MOD_ENABLE_LIGHT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.9018, 39.4336)), module, Zoxnoxious3372::MOD_AMOUNT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.1233, 117.3152)), module, Zoxnoxious3372::NOISE_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.4844, 51.2228)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.4767, 101.6110)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(54.3714, 116.6685)), module, Zoxnoxious3372::OUTPUT_PAN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(39.135, 83.401)), module, Zoxnoxious3372::CUTOFF_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(64.424, 83.401)), module, Zoxnoxious3372::RESONANCE_INPUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(42.8644, 32.0711)), module, Zoxnoxious3372::MOD_AMOUNT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.4471, 43.8603)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.2754, 94.2484)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(58.1394, 109.3060)), module, Zoxnoxious3372::OUTPUT_PAN_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(43.0976, 75.9387)), module, Zoxnoxious3372::CUTOFF_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(68.3866, 76.2091)), module, Zoxnoxious3372::RESONANCE_CLIP_LIGHT));

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
        output1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(58.827, 111.088)));
        output1NameTextField->box.size = mm2px(Vec(18.0, 3.636));
        output1NameTextField->setText(module ? &module->output1NameString : NULL);
        addChild(output1NameTextField);


        // mm2px(Vec(18.0, 3.636))
        output2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(32.619, 111.088)));
        output2NameTextField->box.size = mm2px(Vec(18.0, 3.636));
        output2NameTextField->setText(module ? &module->output2NameString : NULL);
        addChild(output2NameTextField);
    }

    CardTextDisplay *source1NameTextField;
    CardTextDisplay *source2NameTextField;
    CardTextDisplay *output1NameTextField;
    CardTextDisplay *output2NameTextField;

};


Model* modelZoxnoxious3372 = createModel<Zoxnoxious3372, Zoxnoxious3372Widget>("Zoxnoxious3372");
