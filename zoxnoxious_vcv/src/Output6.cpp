#include "plugin.hpp"


struct Output6 : Module {
	enum ParamId {
		MIX_LEFT_SELECT_PARAM,
		CARD_A_MIX2_BUTTON_PARAM,
		CARD_A_MIX1_BUTTON_PARAM,
		LEFT_LEVEL_KNOB_PARAM,
		CARD_B_MIX2_BUTTON_PARAM,
		CARD_B_MIX1_BUTTON_PARAM,
		CARD_C_MIX2_BUTTON_PARAM,
		CARD_C_MIX1_BUTTON_PARAM,
		CARD_D_MIX2_BUTTON_PARAM,
		CARD_D_MIX1_BUTTON_PARAM,
		RIGHT_LEVEL_KNOB_PARAM,
		CARD_E_MIX2_BUTTON_PARAM,
		CARD_E_MIX1_BUTTON_PARAM,
		CARD_F_MIX2_BUTTON_PARAM,
		CARD_F_MIX1_BUTTON_PARAM,
		MIX_RIGHT_SELECT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		LEFT_LEVEL_INPUT_INPUT,
		RIGHT_LEVEL_INPUT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		LEFT_LEVEL_CLIP_LIGHT,
		RIGHT_LEVEL_CLIP_LIGHT,
		CARD_A_MIX2_BUTTON_LIGHT,
		CARD_A_MIX1_BUTTON_LIGHT,
		CARD_B_MIX2_BUTTON_LIGHT,
		CARD_B_MIX1_BUTTON_LIGHT,
		CARD_C_MIX2_BUTTON_LIGHT,
		CARD_C_MIX1_BUTTON_LIGHT,
		CARD_D_MIX2_BUTTON_LIGHT,
		CARD_D_MIX1_BUTTON_LIGHT,
		CARD_E_MIX2_BUTTON_LIGHT,
		CARD_E_MIX1_BUTTON_LIGHT,
		CARD_F_MIX2_BUTTON_LIGHT,
		CARD_F_MIX1_BUTTON_LIGHT,
		LIGHTS_LEN
	};

	Output6() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configSwitch(CARD_A_MIX2_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2", { "Off", "On" });
		configSwitch(CARD_A_MIX1_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1", { "Off", "On" });
		configSwitch(CARD_B_MIX2_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2", { "Off", "On" });
		configSwitch(CARD_B_MIX1_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1", { "Off", "On" });
		configSwitch(CARD_C_MIX2_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 2", { "Off", "On" });
		configSwitch(CARD_C_MIX1_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 1", { "Off", "On" });
		configSwitch(CARD_D_MIX2_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2", { "Off", "On" });
		configSwitch(CARD_D_MIX1_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1", { "Off", "On" });
		configSwitch(CARD_E_MIX2_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2", { "Off", "On" });
		configSwitch(CARD_E_MIX1_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1", { "Off", "On" });
		configSwitch(CARD_F_MIX2_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2", { "Off", "On" });
		configSwitch(CARD_F_MIX1_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1", { "Off", "On" });

		configSwitch(MIX_LEFT_SELECT_PARAM, 0.f, 1.f, 0.f, "Left Output", { "Out 1", "Out2" });
		configParam(LEFT_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.f, "");
		configInput(LEFT_LEVEL_INPUT_INPUT, "");

		configSwitch(MIX_RIGHT_SELECT_PARAM, 0.f, 1.f, 0.f, "Right Output", { "Out1", "Out2" });
		configParam(RIGHT_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.f, "");
		configInput(RIGHT_LEVEL_INPUT_INPUT, "");
	}

	void process(const ProcessArgs& args) override {

            lights[CARD_A_MIX2_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX2_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX1_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX1_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX2_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX2_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX1_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX1_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX2_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX2_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX1_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX1_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX2_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX2_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX1_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX1_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX2_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX2_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX1_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX1_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX2_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX2_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX1_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX1_BUTTON_PARAM].getValue() > 0.f);


	}
};


struct Output6Widget : ModuleWidget {
    Output6Widget(Output6* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Output6.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(44.346, 38.587)), module, Output6::CARD_A_MIX2_BUTTON_PARAM, Output6::CARD_A_MIX2_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(26.953, 38.831)), module, Output6::CARD_A_MIX1_BUTTON_PARAM, Output6::CARD_A_MIX1_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(44.346, 51.648)), module, Output6::CARD_B_MIX2_BUTTON_PARAM, Output6::CARD_B_MIX2_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(26.953, 51.893)), module, Output6::CARD_B_MIX1_BUTTON_PARAM, Output6::CARD_B_MIX1_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(44.346, 64.71)), module, Output6::CARD_C_MIX2_BUTTON_PARAM, Output6::CARD_C_MIX2_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(26.953, 64.954)), module, Output6::CARD_C_MIX1_BUTTON_PARAM, Output6::CARD_C_MIX1_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(44.346, 77.771)), module, Output6::CARD_D_MIX2_BUTTON_PARAM, Output6::CARD_D_MIX2_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(26.953, 78.016)), module, Output6::CARD_D_MIX1_BUTTON_PARAM, Output6::CARD_D_MIX1_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(44.346, 90.833)), module, Output6::CARD_E_MIX2_BUTTON_PARAM, Output6::CARD_E_MIX2_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(26.953, 91.077)), module, Output6::CARD_E_MIX1_BUTTON_PARAM, Output6::CARD_E_MIX1_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(44.346, 103.894)), module, Output6::CARD_F_MIX2_BUTTON_PARAM, Output6::CARD_F_MIX2_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(26.953, 104.139)), module, Output6::CARD_F_MIX1_BUTTON_PARAM, Output6::CARD_F_MIX1_BUTTON_LIGHT));


        addParam(createParamCentered<CKSS>(mm2px(Vec(58.989, 17.478)), module, Output6::MIX_LEFT_SELECT_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(66.928, 47.501)), module, Output6::LEFT_LEVEL_KNOB_PARAM));


        addParam(createParamCentered<CKSS>(mm2px(Vec(58.989, 117.692)), module, Output6::MIX_RIGHT_SELECT_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(66.928, 89.669)), module, Output6::RIGHT_LEVEL_KNOB_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(66.928, 57.166)), module, Output6::LEFT_LEVEL_INPUT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(66.928, 99.334)), module, Output6::RIGHT_LEVEL_INPUT_INPUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(66.928, 38.417)), module, Output6::LEFT_LEVEL_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(66.928, 80.585)), module, Output6::RIGHT_LEVEL_CLIP_LIGHT));

        // mm2px(Vec(17.694, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(5.294, 36.769))));
        // mm2px(Vec(17.694, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(5.294, 49.83))));
        // mm2px(Vec(17.694, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(5.294, 62.892))));
        // mm2px(Vec(17.694, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(5.294, 75.953))));
        // mm2px(Vec(17.694, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(5.294, 89.015))));
        // mm2px(Vec(17.694, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(5.294, 102.076))));
    }
};


Model* modelOutput6 = createModel<Output6, Output6Widget>("Output6");
