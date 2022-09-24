#include "plugin.hpp"
#include "ZoxnoxiousExpander.hpp"

const static int num_audio_inputs = 6;


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
        LIGHTS_LEN
    };


    dsp::ClockDivider lightDivider;
    float freqClipTimer;
    float pulseWidthClipTimer;
    float linearClipTimer;
    float mix1TriangleVcaClipTimer;
    float syncPhaseClipTimer;
    float extModAmountClipTimer;

    // detect state changes so we can send a MIDI event.
    // Assume int_min is an invalid value.  On start, idea would be
    // to send current state via midi to the board is in sync with
    // the rack plugin.
    // The big hack here: a midiProgram is sent, and the program value
    // is indexed based on the button parameter value.
    struct buttonParamMidiProgram {
        enum ParamId button;
        int previousValue;
        uint8_t midiProgram[3];
    };

    struct buttonParamMidiProgram buttonParamToMidiProgramList[11] =
        {
            { SYNC_NEG_BUTTON_PARAM, INT_MIN, { 0, 1 } },
            { MIX1_PULSE_BUTTON_PARAM, INT_MIN, { 2, 3 } },
            { EXT_MOD_SELECT_SWITCH_PARAM, INT_MIN, { 4, 5 } },
            { MIX1_COMPARATOR_BUTTON_PARAM, INT_MIN, { 6, 7 } },
            { MIX2_PULSE_BUTTON_PARAM, INT_MIN, { 8, 9 } },
            { EXT_MOD_PWM_BUTTON_PARAM, INT_MIN, { 10, 11 } },
            { EXP_FM_BUTTON_PARAM, INT_MIN, { 12, 13 } },
            { LINEAR_FM_BUTTON_PARAM, INT_MIN, { 14, 15 } },
            { MIX2_SAW_BUTTON_PARAM, INT_MIN, { 16, 17 } },
            { MIX1_SAW_LEVEL_SELECTOR_PARAM, INT_MIN, { 18, 19, 20 } },
            { SYNC_POS_BUTTON_PARAM, INT_MIN, { 22, 23 } }
        };

    Zoxnoxious3340() {

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

        lightDivider.setDivision(512);
    }




    void process(const ProcessArgs& args) override {
        processExpander(args);

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


        // Any buttons params pushed need to send midi events
        for (int i = 0; i < (int) (sizeof(buttonParamToMidiProgramList) / sizeof(struct buttonParamMidiProgram)); ++i) {
            int newValue = (int) (params[ buttonParamToMidiProgramList[i].button ].getValue() + 0.5f);

            if (buttonParamToMidiProgramList[i].previousValue != newValue) {
                buttonParamToMidiProgramList[i].previousValue = newValue;
                // midi program to send is index by the (integer)


                //TODO: send midi message
                //midiOutput.sendProgramChange(buttonParamToMidiProgramList[i].midiProgram[newValue]);
            }
        }



        dsp::Frame<num_audio_inputs> inputFrame = {};
        float v;
        const float clipTime = 0.25f;



        // linear
        v = params[LINEAR_INPUT].getValue() + inputs[LINEAR_KNOB_PARAM].getVoltageSum() / 10.f;
        inputFrame.samples[5] = clamp(v, 0.f, 1.f);
        if (inputFrame.samples[5] != v) {
            linearClipTimer = clipTime;
        }
                    

        // external mod amount
        v = params[EXT_MOD_AMOUNT_INPUT].getValue() + inputs[EXT_MOD_AMOUNT_KNOB_PARAM].getVoltageSum() / 10.f;
        inputFrame.samples[4] = clamp(v, 0.f, 1.f);
        if (inputFrame.samples[4] != v) {
            extModAmountClipTimer = clipTime;
        }


        // mix1 triangle
        v = params[MIX1_TRIANGLE_VCA_INPUT].getValue() + inputs[MIX1_TRIANGLE_KNOB_PARAM].getVoltageSum() / 10.f;
        inputFrame.samples[3] = clamp(v, 0.f, 1.f);
        if (inputFrame.samples[3] != v) {
            mix1TriangleVcaClipTimer = clipTime;
        }
            
        // pulse width
        v = params[PULSE_WIDTH_INPUT].getValue() + inputs[PULSE_WIDTH_KNOB_PARAM].getVoltageSum() / 10.f;
        inputFrame.samples[2] = clamp(v, 0.f, 1.f);
        if (inputFrame.samples[2] != v) {
            pulseWidthClipTimer = clipTime;
        }


        // sync phase
        v = params[SYNC_PHASE_INPUT].getValue() + inputs[SYNC_PHASE_KNOB_PARAM].getVoltageSum() / 10.f;
        inputFrame.samples[1] = clamp(v, 0.f, 1.f);
        if (inputFrame.samples[1] != v) {
            syncPhaseClipTimer = clipTime;
        }


        // frequency
        v = params[FREQ_INPUT].getValue() + inputs[FREQ_KNOB_PARAM].getVoltageSum() / 10.f;
        inputFrame.samples[0] = clamp(v, 0.f, 1.f);
        if (inputFrame.samples[0] != v) {
            freqClipTimer = clipTime;
        }



        if (lightDivider.process()) {
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
        }
    }


};





struct Zoxnoxious3340Widget : ModuleWidget {

    Zoxnoxious3340Widget(Zoxnoxious3340* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/zoxnoxious3340.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.732, 31.0)), module, Zoxnoxious3340::FREQ_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.481, 31.0)), module, Zoxnoxious3340::PULSE_WIDTH_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(41.381, 31.0)), module, Zoxnoxious3340::LINEAR_KNOB_PARAM));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(59.943, 27.744)), module, Zoxnoxious3340::MIX1_PULSE_BUTTON_PARAM, Zoxnoxious3340::MIX1_PULSE_BUTTON_LIGHT));


        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(59.943, 43.567)), module, Zoxnoxious3340::MIX1_TRIANGLE_KNOB_PARAM));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(37.66, 93.07)), module, Zoxnoxious3340::SYNC_PHASE_KNOB_PARAM));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(31.71, 68.61)), module, Zoxnoxious3340::SYNC_POS_BUTTON_PARAM, Zoxnoxious3340::SYNC_POS_ENABLE_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(43.25, 68.61)), module, Zoxnoxious3340::SYNC_NEG_BUTTON_PARAM, Zoxnoxious3340::SYNC_NEG_ENABLE_LIGHT));


        //addParam(createParamCentered<CKSSThreeHorizontal>(mm2px(Vec(59.943, 69.61)), module, Zoxnoxious3340::MIX1_SAW_LEVEL_SELECTOR_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(59.943, 69.61)), module, Zoxnoxious3340::MIX1_SAW_LEVEL_SELECTOR_PARAM));

        addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(6.65, 66.2)), module, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_PARAM));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(59.943, 87.07)), module, Zoxnoxious3340::MIX1_COMPARATOR_BUTTON_PARAM, Zoxnoxious3340::MIX1_COMPARATOR_BUTTON_LIGHT));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.76, 93.07)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_KNOB_PARAM));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(59.943, 104.804)), module, Zoxnoxious3340::MIX2_PULSE_BUTTON_PARAM, Zoxnoxious3340::MIX2_PULSE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(8.509, 118.567)), module, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_PARAM, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(24.607, 118.567)), module, Zoxnoxious3340::EXP_FM_BUTTON_PARAM, Zoxnoxious3340::EXP_FM_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(40.019, 118.567)), module, Zoxnoxious3340::LINEAR_FM_BUTTON_PARAM, Zoxnoxious3340::LINEAR_FM_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(59.943, 117.049)), module, Zoxnoxious3340::MIX2_SAW_BUTTON_PARAM, Zoxnoxious3340::MIX2_SAW_BUTTON_LIGHT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.732, 42.0)), module, Zoxnoxious3340::FREQ_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.481, 42.0)), module, Zoxnoxious3340::PULSE_WIDTH_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(41.381, 42.0)), module, Zoxnoxious3340::LINEAR_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(59.943, 53.383)), module, Zoxnoxious3340::MIX1_TRIANGLE_VCA_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.66, 102.70)), module, Zoxnoxious3340::SYNC_PHASE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.76, 102.70)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_INPUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(8.732, 22.2)), module, Zoxnoxious3340::FREQ_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(25.481, 22.2)), module, Zoxnoxious3340::PULSE_WIDTH_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(41.381, 22.2)), module, Zoxnoxious3340::LINEAR_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.59, 39.52)), module, Zoxnoxious3340::MIX1_TRIANGLE_VCA_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.61, 84.09)), module, Zoxnoxious3340::SYNC_PHASE_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.76, 84.09)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_CLIP_LIGHT));

    }


};


Model* modelZoxnoxious3340 = createModel<Zoxnoxious3340, Zoxnoxious3340Widget>("Zoxnoxious3340");
