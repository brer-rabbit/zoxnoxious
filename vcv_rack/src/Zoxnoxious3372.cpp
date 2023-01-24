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
const static int sourceOneToSignalMap[8] = { 12, 11, 8, 7, 4, 3, 1, 0 };

// card6 out1
// card5 out2
// card4 out1
// card3 out2
// card2 out2
// card2 out1
// card1 out2
// card1 out1
const static int sourceTwoToSignalMap[8] = { 10, 9, 6, 5, 3, 2, 1, 0 };

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
        PARAMS_LEN
    };
    enum InputId {
        CUTOFF_INPUT,
        OUTPUT_PAN_INPUT,
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
        LEFT_EXPANDER_LIGHT,
        RIGHT_EXPANDER_LIGHT,
        MOD_AMOUNT_CLIP_LIGHT,
        SOURCE_ONE_LEVEL_CLIP_LIGHT,
        SOURCE_TWO_LEVEL_CLIP_LIGHT,
        OUTPUT_PAN_CLIP_LIGHT,
        CUTOFF_CLIP_LIGHT,
        RESONANCE_CLIP_LIGHT,
        VCA_MOD_ENABLE_LIGHT,
        FILTER_MOD_ENABLE_LIGHT,
        SOURCE_ONE_DOWN_BUTTON_LIGHT,
        SOURCE_ONE_UP_BUTTON_LIGHT,
        SOURCE_TWO_DOWN_BUTTON_LIGHT,
        SOURCE_TWO_UP_BUTTON_LIGHT,
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

        configParam(MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Modulation Amount");
        configParam(NOISE_KNOB_PARAM, 0.f, 1.f, 0.f, "White Noise");
        configParam(SOURCE_ONE_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Level");
        configParam(SOURCE_TWO_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Level");
        configParam(OUTPUT_PAN_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pan");
        configParam(CUTOFF_KNOB_PARAM, 0.f, 1.f, 1.f, "Cutoff");
        configParam(RESONANCE_KNOB_PARAM, 0.f, 1.f, 0.f, "Resonance");

        configSwitch(FILTER_MOD_SWITCH_PARAM, 0.f, 1.f, 0.f, "Filter Mod", {"Off", "On"});
        configSwitch(VCA_MOD_SWITCH_PARAM, 0.f, 1.f, 0.f, "VCA Mod", {"Off", "On"});

        configInput(MOD_AMOUNT_INPUT, "Modulation Amount");
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

            // set string names to make available to ui
            int source1Index = static_cast<int>(params[SOURCE_ONE_VALUE_HIDDEN_PARAM].getValue());
            source1NameString = source1Index >= 0 && source1Index < 8 ?
              cardOutputNames[ sourceOneToSignalMap[source1Index] ] : invalidCardOutputName;

            int source2Index = static_cast<int>(params[SOURCE_TWO_VALUE_HIDDEN_PARAM].getValue());
            source2NameString = source2Index >= 0 && source2Index < 8 ?
              cardOutputNames[ sourceTwoToSignalMap[source2Index] ] : invalidCardOutputName;


        }

    }



    /** processZoxnoxiousControl
     *
     * add our control voltage values to the control message.  Add or queue any MIDI message.
     *
     */
    void processZoxnoxiousControl(ZoxnoxiousControlMsg *controlMsg) override {
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
    }

    void onChannelAssignmentLost() override {
        ZoxnoxiousModule::onChannelAssignmentLost();
        output1NameString = invalidCardOutputName;
        output2NameString = invalidCardOutputName;
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.604, 21.598)), module, Zoxnoxious3372::SOURCE_ONE_DOWN_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.965, 21.598)), module, Zoxnoxious3372::SOURCE_ONE_UP_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.991, 21.598)), module, Zoxnoxious3372::SOURCE_TWO_DOWN_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(43.754, 21.598)), module, Zoxnoxious3372::SOURCE_TWO_UP_BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(64.865, 26.1)), module, Zoxnoxious3372::MOD_AMOUNT_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(16.284, 34.017)), module, Zoxnoxious3372::NOISE_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(16.284, 50.707)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(38.873, 50.707)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(64.865, 53.776)), module, Zoxnoxious3372::FILTER_MOD_SWITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(64.865, 66.519)), module, Zoxnoxious3372::VCA_MOD_SWITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(64.841, 88.405)), module, Zoxnoxious3372::OUTPUT_PAN_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(16.26, 91.978)), module, Zoxnoxious3372::CUTOFF_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(38.849, 91.978)), module, Zoxnoxious3372::RESONANCE_KNOB_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(64.865, 39.463)), module, Zoxnoxious3372::MOD_AMOUNT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.284, 64.069)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.873, 64.069)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(64.841, 101.767)), module, Zoxnoxious3372::OUTPUT_PAN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.26, 105.34)), module, Zoxnoxious3372::CUTOFF_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.849, 105.34)), module, Zoxnoxious3372::RESONANCE_INPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(2.02, 8.219)), module, Zoxnoxious3372::LEFT_EXPANDER_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(79.507, 8.219)), module, Zoxnoxious3372::RIGHT_EXPANDER_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(68.374, 32.1)), module, Zoxnoxious3372::MOD_AMOUNT_CLIP_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.247, 56.707)), module, Zoxnoxious3372::SOURCE_ONE_LEVEL_CLIP_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(42.919, 56.707)), module, Zoxnoxious3372::SOURCE_TWO_LEVEL_CLIP_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(68.609, 94.405)), module, Zoxnoxious3372::OUTPUT_PAN_CLIP_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.223, 97.978)), module, Zoxnoxious3372::CUTOFF_CLIP_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(42.812, 97.978)), module, Zoxnoxious3372::RESONANCE_CLIP_LIGHT));

		// mm2px(Vec(18.0, 3.637))
		addChild(createWidget<Widget>(mm2px(Vec(6.784, 13.838))));
		// mm2px(Vec(18.0, 3.637))
		addChild(createWidget<Widget>(mm2px(Vec(29.373, 14.029))));
		// mm2px(Vec(18.0, 3.636))
		addChild(createWidget<Widget>(mm2px(Vec(55.841, 108.573))));
		// mm2px(Vec(18.0, 3.636))
		addChild(createWidget<Widget>(mm2px(Vec(55.841, 114.334))));
	}
};


Model* modelZoxnoxious3372 = createModel<Zoxnoxious3372, Zoxnoxious3372Widget>("Zoxnoxious3372");
