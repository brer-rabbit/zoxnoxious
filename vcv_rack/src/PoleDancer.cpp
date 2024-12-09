#include "plugin.hpp"
#include "zcomponentlib.hpp"

struct PoleDancer : Module {
	enum ParamId {
		SOURCE_ONE_DOWN_BUTTON_PARAM,
		SOURCE_ONE_UP_BUTTON_PARAM,
		SOURCE_TWO_DOWN_BUTTON_PARAM,
		SOURCE_TWO_UP_BUTTON_PARAM,
		SOURCE_ONE_LEVEL_KNOB_PARAM,
		SOURCE_ONE_MOD_AMOUNT_KNOB_PARAM,
		SOURCE_TWO_LEVEL_KNOB_PARAM,
		SOURCE_TWO_MOD_AMOUNT_KNOB_PARAM,
		CUTOFF_KNOB_PARAM,
		REZ_COMP_P1_SWITCH_PARAM,
		REZ_COMP_P2_SWITCH_PARAM,
		REZ_COMP_P3_SWITCH_PARAM,
		REZ_COMP_INV_SWITCH_PARAM,
		RESONANCE_KNOB_PARAM,
		FILTER_VCA_KNOB_PARAM,
		DRY_MIX_KNOB_PARAM,
		POLE1_MIX_KNOB_PARAM,
		POLE2_MIX_KNOB_PARAM,
		POLE3_MIX_KNOB_PARAM,
		POLE4_MIX_KNOB_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SOURCE_ONE_LEVEL_INPUT,
		SOURCE_ONE_MOD_AMOUNT_INPUT,
		SOURCE_TWO_LEVEL_INPUT,
		SOURCE_TWO_MOD_AMOUNT_INPUT,
		CUTOFF_INPUT,
		RESONANCE_INPUT,
		FILTER_VCA_INPUT,
		POLE_MIX_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		LEFT_EXPANDER_LIGHT,
		RIGHT_EXPANDER_LIGHT,
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
                REZ_COMP_P1_SWITCH_LIGHT,
                REZ_COMP_P2_SWITCH_LIGHT,
                REZ_COMP_P3_SWITCH_LIGHT,
                REZ_COMP_INV_SWITCH_LIGHT,
		LIGHTS_LEN
	};

	PoleDancer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

                // buttons for inputs select
		configButton(SOURCE_ONE_DOWN_BUTTON_PARAM, "Previous");
		configButton(SOURCE_ONE_UP_BUTTON_PARAM, "Next");
		configButton(SOURCE_TWO_DOWN_BUTTON_PARAM, "Previous");
		configButton(SOURCE_TWO_UP_BUTTON_PARAM, "Next");

		configParam(SOURCE_ONE_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Source One Level", "%", 0.f, 100.f);
		configParam(SOURCE_ONE_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Source One Mod", "%", 0.f, 100.f);
		configParam(SOURCE_TWO_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Source Two Level", "%", 0.f, 100.f);
		configParam(SOURCE_TWO_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Source Two Mod", "%", 0.f, 100.f);

		configParam(CUTOFF_KNOB_PARAM, 0.f, 1.f, 1.f, "Cutoff", "V", 0.f, 10.f, -1.f);
		configSwitch(REZ_COMP_P1_SWITCH_PARAM, 0.f, 1.f, 0.f, "Pole 1 Rez Comp", {"Off", "On"});
		configSwitch(REZ_COMP_P2_SWITCH_PARAM, 0.f, 1.f, 0.f, "Pole 2 Rez Comp", {"Off", "On"});
		configSwitch(REZ_COMP_P3_SWITCH_PARAM, 0.f, 1.f, 0.f, "Pole 3 Rez Comp", {"Off", "On"});
		configSwitch(REZ_COMP_INV_SWITCH_PARAM, 0.f, 1.f, 0.f, "Invert Rez Comp", {"Off", "On"});

		configParam(RESONANCE_KNOB_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
		configParam(FILTER_VCA_KNOB_PARAM, 0.f, 1.f, 0.5f, "Output VCA", "%", 0.f, 100.f);

		configParam(DRY_MIX_KNOB_PARAM, 0.f, 1.f, 0.f, "Dry Mix", "%", 0.f, 100.f);
		configParam(POLE1_MIX_KNOB_PARAM, 0.f, 1.f, 0.f, "Pole 1 Mix", "%", 0.f, 100.f);
		configParam(POLE2_MIX_KNOB_PARAM, 0.f, 1.f, 0.f, "Pole 2 Mix", "%", 0.f, 100.f);
		configParam(POLE3_MIX_KNOB_PARAM, 0.f, 1.f, 0.f, "Pole 3 Mix", "%", 0.f, 100.f);
		configParam(POLE4_MIX_KNOB_PARAM, 0.f, 1.f, 0.2f, "POle 4 Mix", "%", 0.f, 100.f);

		configInput(SOURCE_ONE_LEVEL_INPUT, "Source One Level");
		configInput(SOURCE_ONE_MOD_AMOUNT_INPUT, "Source One Mod Amount");
		configInput(SOURCE_TWO_LEVEL_INPUT, "Source Two Level");
		configInput(SOURCE_TWO_MOD_AMOUNT_INPUT, "Source Two Mod Amount");
		configInput(CUTOFF_INPUT, "Filter Cutoff");
		configInput(RESONANCE_INPUT, "Resonance");
		configInput(FILTER_VCA_INPUT, "Output VCA");
		configInput(POLE_MIX_INPUT, "Pole Mix");
	}

	void process(const ProcessArgs& args) override {
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

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(13.639, 39.831)), module, PoleDancer::SOURCE_ONE_LEVEL_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.373, 39.831)), module, PoleDancer::SOURCE_ONE_MOD_AMOUNT_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50.462, 39.831)), module, PoleDancer::SOURCE_TWO_LEVEL_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(68.8, 39.831)), module, PoleDancer::SOURCE_TWO_MOD_AMOUNT_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.37, 77.109)), module, PoleDancer::CUTOFF_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(29.21, 77.379)), module, PoleDancer::RESONANCE_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(66.982, 77.379)), module, PoleDancer::FILTER_VCA_KNOB_PARAM));

    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(43.666, 76.946)), module, PoleDancer::REZ_COMP_P1_SWITCH_PARAM, PoleDancer::REZ_COMP_P1_SWITCH_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(52.616, 76.946)), module, PoleDancer::REZ_COMP_P2_SWITCH_PARAM, PoleDancer::REZ_COMP_P2_SWITCH_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(43.666, 91.072)), module, PoleDancer::REZ_COMP_P3_SWITCH_PARAM, PoleDancer::REZ_COMP_P3_SWITCH_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(52.616, 91.072)), module, PoleDancer::REZ_COMP_INV_SWITCH_PARAM, PoleDancer::REZ_COMP_INV_SWITCH_LIGHT));


    addParam(createParamCentered<Trimpot>(mm2px(Vec(23.754, 109.044)), module, PoleDancer::DRY_MIX_KNOB_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(35.331, 109.044)), module, PoleDancer::POLE1_MIX_KNOB_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(46.907, 109.044)), module, PoleDancer::POLE2_MIX_KNOB_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(58.483, 109.044)), module, PoleDancer::POLE3_MIX_KNOB_PARAM));
    addParam(createParamCentered<Trimpot>(mm2px(Vec(70.06, 109.044)), module, PoleDancer::POLE4_MIX_KNOB_PARAM));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.639, 53.193)), module, PoleDancer::SOURCE_ONE_LEVEL_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.373, 53.193)), module, PoleDancer::SOURCE_ONE_MOD_AMOUNT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50.462, 53.194)), module, PoleDancer::SOURCE_TWO_LEVEL_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(68.8, 53.194)), module, PoleDancer::SOURCE_TWO_MOD_AMOUNT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.37, 90.742)), module, PoleDancer::CUTOFF_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(29.21, 90.742)), module, PoleDancer::RESONANCE_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(66.982, 90.742)), module, PoleDancer::FILTER_VCA_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.847, 109.044)), module, PoleDancer::POLE_MIX_INPUT));

    addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(2.02, 8.219)), module, PoleDancer::LEFT_EXPANDER_LIGHT));
    addChild(createLightCentered<TriangleRightLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(79.507, 8.219)), module, PoleDancer::RIGHT_EXPANDER_LIGHT));

    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.687, 45.831)), module, PoleDancer::SOURCE_ONE_LEVEL_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.421, 45.831)), module, PoleDancer::SOURCE_ONE_MOD_AMOUNT_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(54.425, 45.831)), module, PoleDancer::SOURCE_TWO_LEVEL_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(72.763, 45.831)), module, PoleDancer::SOURCE_TWO_MOD_AMOUNT_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.332, 83.379)), module, PoleDancer::CUTOFF_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(33.173, 83.379)), module, PoleDancer::RESONANCE_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(70.945, 83.379)), module, PoleDancer::FILTER_VCA_CLIP_LIGHT));

    // Text fields
    source1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(7.185, 13.839)));
    source1NameTextField->box.size = mm2px(Vec(18.388, 3.636));
    source1NameTextField->setText(NULL); // module ? &module->source1NameString : NULL);
    addChild(source1NameTextField);

    source2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(44.142, 13.839)));
    source2NameTextField->box.size = mm2px(Vec(18.388, 3.636));
    source2NameTextField->setText(NULL); // module ? &module->source2NameString : NULL);
    addChild(source2NameTextField);

    output1NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(57.788, 117.012)));
    output1NameTextField->box.size = mm2px(Vec(18.388, 3.636));
    output1NameTextField->setText(NULL); // module ? &module->output1NameString : NULL);
    addChild(output1NameTextField);

    output2NameTextField = createWidget<CardTextDisplay>(mm2px(Vec(19.135, 117.012)));
    output2NameTextField->box.size = mm2px(Vec(18.388, 3.636));
    output2NameTextField->setText(NULL); //module ? &module->output2NameString : NULL);
    addChild(output2NameTextField);
  }


  CardTextDisplay *source1NameTextField;
  CardTextDisplay *source2NameTextField;
  CardTextDisplay *output1NameTextField;
  CardTextDisplay *output2NameTextField;

};


Model* modelPoleDancer = createModel<PoleDancer, PoleDancerWidget>("PoleDancer");
