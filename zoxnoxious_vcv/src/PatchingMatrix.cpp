#include "plugin.hpp"
#include "zcomponentlib.hpp"

struct PatchingMatrix : Module {
	enum ParamId {
		MIX_LEFT_SELECT_PARAM,
		CARD_A_MIX1_CARD_A_BUTTON_PARAM,
		CARD_A_MIX1_CARD_B_BUTTON_PARAM,
		CARD_A_MIX1_CARD_C_BUTTON_PARAM,
		CARD_A_MIX1_CARD_D_BUTTON_PARAM,
		CARD_A_MIX1_CARD_E_BUTTON_PARAM,
		CARD_A_MIX1_CARD_F_BUTTON_PARAM,
		CARD_A_MIX1_OUTPUT_BUTTON_PARAM,
		CARD_A_MIX2_CARD_A_BUTTON_PARAM,
		CARD_A_MIX2_CARD_B_BUTTON_PARAM,
		CARD_A_MIX2_CARD_C_BUTTON_PARAM,
		CARD_A_MIX2_CARD_D_BUTTON_PARAM,
		CARD_A_MIX2_CARD_E_BUTTON_PARAM,
		CARD_A_MIX2_CARD_F_BUTTON_PARAM,
		CARD_A_MIX2_OUTPUT_BUTTON_PARAM,
		LEFT_LEVEL_KNOB_PARAM,
		CARD_B_MIX1_CARD_A_BUTTON_PARAM,
		CARD_B_MIX1_CARD_B_BUTTON_PARAM,
		CARD_B_MIX1_CARD_C_BUTTON_PARAM,
		CARD_B_MIX1_CARD_D_BUTTON_PARAM,
		CARD_B_MIX1_CARD_E_BUTTON_PARAM,
		CARD_B_MIX1_CARD_F_BUTTON_PARAM,
		CARD_B_MIX1_OUTPUT_BUTTON_PARAM,
		CARD_B_MIX2_CARD_A_BUTTON_PARAM,
		CARD_B_MIX2_CARD_B_BUTTON_PARAM,
		CARD_B_MIX2_CARD_C_BUTTON_PARAM,
		CARD_B_MIX2_CARD_D_BUTTON_PARAM,
		CARD_B_MIX2_CARD_E_BUTTON_PARAM,
		CARD_B_MIX2_CARD_F_BUTTON_PARAM,
		CARD_B_MIX2_OUTPUT_BUTTON_PARAM,
		CARD_C_MIX1_CARD_A_BUTTON_PARAM,
		CARD_C_MIX1_CARD_B_BUTTON_PARAM,
		CARD_C_MIX1_CARD_C_BUTTON_PARAM,
		CARD_C_MIX1_CARD_D_BUTTON_PARAM,
		CARD_C_MIX1_CARD_E_BUTTON_PARAM,
		CARD_C_MIX1_CARD_F_BUTTON_PARAM,
		CARD_C_MIX1_OUTPUT_BUTTON_PARAM,
		CARD_C_MIX2_CARD_A_BUTTON_PARAM,
		CARD_C_MIX2_CARD_B_BUTTON_PARAM,
		CARD_C_MIX2_CARD_C_BUTTON_PARAM,
		CARD_C_MIX2_CARD_D_BUTTON_PARAM,
		CARD_C_MIX2_CARD_E_BUTTON_PARAM,
		CARD_C_MIX2_CARD_F_BUTTON_PARAM,
		CARD_C_MIX2_OUTPUT_BUTTON_PARAM,
		CARD_D_MIX1_CARD_A_BUTTON_PARAM,
		CARD_D_MIX1_CARD_B_BUTTON_PARAM,
		CARD_D_MIX1_CARD_C_BUTTON_PARAM,
		CARD_D_MIX1_CARD_D_BUTTON_PARAM,
		CARD_D_MIX1_CARD_E_BUTTON_PARAM,
		CARD_D_MIX1_CARD_F_BUTTON_PARAM,
		CARD_D_MIX1_OUTPUT_BUTTON_PARAM,
		CARD_D_MIX2_CARD_A_BUTTON_PARAM,
		CARD_D_MIX2_CARD_B_BUTTON_PARAM,
		CARD_D_MIX2_CARD_C_BUTTON_PARAM,
		CARD_D_MIX2_CARD_D_BUTTON_PARAM,
		CARD_D_MIX2_CARD_E_BUTTON_PARAM,
		CARD_D_MIX2_CARD_F_BUTTON_PARAM,
		CARD_D_MIX2_OUTPUT_BUTTON_PARAM,
		RIGHT_LEVEL_KNOB_PARAM,
		CARD_E_MIX1_CARD_A_BUTTON_PARAM,
		CARD_E_MIX1_CARD_B_BUTTON_PARAM,
		CARD_E_MIX1_CARD_C_BUTTON_PARAM,
		CARD_E_MIX1_CARD_D_BUTTON_PARAM,
		CARD_E_MIX1_CARD_E_BUTTON_PARAM,
		CARD_E_MIX1_CARD_F_BUTTON_PARAM,
		CARD_E_MIX1_OUTPUT_BUTTON_PARAM,
		CARD_E_MIX2_CARD_A_BUTTON_PARAM,
		CARD_E_MIX2_CARD_B_BUTTON_PARAM,
		CARD_E_MIX2_CARD_C_BUTTON_PARAM,
		CARD_E_MIX2_CARD_D_BUTTON_PARAM,
		CARD_E_MIX2_CARD_E_BUTTON_PARAM,
		CARD_E_MIX2_CARD_F_BUTTON_PARAM,
		CARD_E_MIX2_OUTPUT_BUTTON_PARAM,
		CARD_F_MIX1_CARD_A_BUTTON_PARAM,
		CARD_F_MIX1_CARD_B_BUTTON_PARAM,
		CARD_F_MIX1_CARD_C_BUTTON_PARAM,
		CARD_F_MIX1_CARD_D_BUTTON_PARAM,
		CARD_F_MIX1_CARD_E_BUTTON_PARAM,
		CARD_F_MIX1_CARD_F_BUTTON_PARAM,
		CARD_F_MIX1_OUTPUT_BUTTON_PARAM,
		CARD_F_MIX2_CARD_A_BUTTON_PARAM,
		CARD_F_MIX2_CARD_B_BUTTON_PARAM,
		CARD_F_MIX2_CARD_C_BUTTON_PARAM,
		CARD_F_MIX2_CARD_D_BUTTON_PARAM,
		CARD_F_MIX2_CARD_E_BUTTON_PARAM,
		CARD_F_MIX2_CARD_F_BUTTON_PARAM,
		CARD_F_MIX2_OUTPUT_BUTTON_PARAM,
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
		CARD_A_MIX1_CARD_A_BUTTON_LIGHT,
		CARD_A_MIX1_CARD_B_BUTTON_LIGHT,
		CARD_A_MIX1_CARD_C_BUTTON_LIGHT,
		CARD_A_MIX1_CARD_D_BUTTON_LIGHT,
		CARD_A_MIX1_CARD_E_BUTTON_LIGHT,
		CARD_A_MIX1_CARD_F_BUTTON_LIGHT,
		CARD_A_MIX1_OUTPUT_BUTTON_LIGHT,
		CARD_A_MIX2_CARD_A_BUTTON_LIGHT,
		CARD_A_MIX2_CARD_B_BUTTON_LIGHT,
		CARD_A_MIX2_CARD_C_BUTTON_LIGHT,
		CARD_A_MIX2_CARD_D_BUTTON_LIGHT,
		CARD_A_MIX2_CARD_E_BUTTON_LIGHT,
		CARD_A_MIX2_CARD_F_BUTTON_LIGHT,
		CARD_A_MIX2_OUTPUT_BUTTON_LIGHT,
		CARD_B_MIX1_CARD_A_BUTTON_LIGHT,
		CARD_B_MIX1_CARD_B_BUTTON_LIGHT,
		CARD_B_MIX1_CARD_C_BUTTON_LIGHT,
		CARD_B_MIX1_CARD_D_BUTTON_LIGHT,
		CARD_B_MIX1_CARD_E_BUTTON_LIGHT,
		CARD_B_MIX1_CARD_F_BUTTON_LIGHT,
		CARD_B_MIX1_OUTPUT_BUTTON_LIGHT,
		CARD_B_MIX2_CARD_A_BUTTON_LIGHT,
		CARD_B_MIX2_CARD_B_BUTTON_LIGHT,
		CARD_B_MIX2_CARD_C_BUTTON_LIGHT,
		CARD_B_MIX2_CARD_D_BUTTON_LIGHT,
		CARD_B_MIX2_CARD_E_BUTTON_LIGHT,
		CARD_B_MIX2_CARD_F_BUTTON_LIGHT,
		CARD_B_MIX2_OUTPUT_BUTTON_LIGHT,
		CARD_C_MIX1_CARD_A_BUTTON_LIGHT,
		CARD_C_MIX1_CARD_B_BUTTON_LIGHT,
		CARD_C_MIX1_CARD_C_BUTTON_LIGHT,
		CARD_C_MIX1_CARD_D_BUTTON_LIGHT,
		CARD_C_MIX1_CARD_E_BUTTON_LIGHT,
		CARD_C_MIX1_CARD_F_BUTTON_LIGHT,
		CARD_C_MIX1_OUTPUT_BUTTON_LIGHT,
		CARD_C_MIX2_CARD_A_BUTTON_LIGHT,
		CARD_C_MIX2_CARD_B_BUTTON_LIGHT,
		CARD_C_MIX2_CARD_C_BUTTON_LIGHT,
		CARD_C_MIX2_CARD_D_BUTTON_LIGHT,
		CARD_C_MIX2_CARD_E_BUTTON_LIGHT,
		CARD_C_MIX2_CARD_F_BUTTON_LIGHT,
		CARD_C_MIX2_OUTPUT_BUTTON_LIGHT,
		CARD_D_MIX1_CARD_A_BUTTON_LIGHT,
		CARD_D_MIX1_CARD_B_BUTTON_LIGHT,
		CARD_D_MIX1_CARD_C_BUTTON_LIGHT,
		CARD_D_MIX1_CARD_D_BUTTON_LIGHT,
		CARD_D_MIX1_CARD_E_BUTTON_LIGHT,
		CARD_D_MIX1_CARD_F_BUTTON_LIGHT,
		CARD_D_MIX1_OUTPUT_BUTTON_LIGHT,
		CARD_D_MIX2_CARD_A_BUTTON_LIGHT,
		CARD_D_MIX2_CARD_B_BUTTON_LIGHT,
		CARD_D_MIX2_CARD_C_BUTTON_LIGHT,
		CARD_D_MIX2_CARD_D_BUTTON_LIGHT,
		CARD_D_MIX2_CARD_E_BUTTON_LIGHT,
		CARD_D_MIX2_CARD_F_BUTTON_LIGHT,
		CARD_D_MIX2_OUTPUT_BUTTON_LIGHT,
		CARD_E_MIX1_CARD_A_BUTTON_LIGHT,
		CARD_E_MIX1_CARD_B_BUTTON_LIGHT,
		CARD_E_MIX1_CARD_C_BUTTON_LIGHT,
		CARD_E_MIX1_CARD_D_BUTTON_LIGHT,
		CARD_E_MIX1_CARD_E_BUTTON_LIGHT,
		CARD_E_MIX1_CARD_F_BUTTON_LIGHT,
		CARD_E_MIX1_OUTPUT_BUTTON_LIGHT,
		CARD_E_MIX2_CARD_A_BUTTON_LIGHT,
		CARD_E_MIX2_CARD_B_BUTTON_LIGHT,
		CARD_E_MIX2_CARD_C_BUTTON_LIGHT,
		CARD_E_MIX2_CARD_D_BUTTON_LIGHT,
		CARD_E_MIX2_CARD_E_BUTTON_LIGHT,
		CARD_E_MIX2_CARD_F_BUTTON_LIGHT,
		CARD_E_MIX2_OUTPUT_BUTTON_LIGHT,
		CARD_F_MIX1_CARD_A_BUTTON_LIGHT,
		CARD_F_MIX1_CARD_B_BUTTON_LIGHT,
		CARD_F_MIX1_CARD_C_BUTTON_LIGHT,
		CARD_F_MIX1_CARD_D_BUTTON_LIGHT,
		CARD_F_MIX1_CARD_E_BUTTON_LIGHT,
		CARD_F_MIX1_CARD_F_BUTTON_LIGHT,
		CARD_F_MIX1_OUTPUT_BUTTON_LIGHT,
		CARD_F_MIX2_CARD_A_BUTTON_LIGHT,
		CARD_F_MIX2_CARD_B_BUTTON_LIGHT,
		CARD_F_MIX2_CARD_C_BUTTON_LIGHT,
		CARD_F_MIX2_CARD_D_BUTTON_LIGHT,
		CARD_F_MIX2_CARD_E_BUTTON_LIGHT,
		CARD_F_MIX2_CARD_F_BUTTON_LIGHT,
		CARD_F_MIX2_OUTPUT_BUTTON_LIGHT,
		LIGHTS_LEN
	};

	PatchingMatrix() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MIX_LEFT_SELECT_PARAM, 0.f, 1.f, 0.f, "Left Output", { "Out1", "Out2" });
		configSwitch(MIX_RIGHT_SELECT_PARAM, 0.f, 1.f, 0.f, "Right Output", { "Out1", "Out2" });
		configSwitch(LEFT_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(RIGHT_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.f, "");

		configSwitch(CARD_A_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card A", { "Off", "On" });
		configSwitch(CARD_A_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card B");
		configSwitch(CARD_A_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card C");
		configSwitch(CARD_A_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card D");
		configSwitch(CARD_A_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card E");
		configSwitch(CARD_A_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card F");
		configSwitch(CARD_A_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Audio Out");
		configSwitch(CARD_A_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card A");
		configSwitch(CARD_A_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card B");
		configSwitch(CARD_A_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card C");
		configSwitch(CARD_A_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card D");
		configSwitch(CARD_A_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card E");
		configSwitch(CARD_A_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card F");
		configSwitch(CARD_A_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_B_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card A");
		configSwitch(CARD_B_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card B");
		configSwitch(CARD_B_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card C");
		configSwitch(CARD_B_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card D");
		configSwitch(CARD_B_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card E");
		configSwitch(CARD_B_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card F");
		configSwitch(CARD_B_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_B_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card A");
		configSwitch(CARD_B_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card B");
		configSwitch(CARD_B_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card C");
		configSwitch(CARD_B_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card D");
		configSwitch(CARD_B_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card E");
		configSwitch(CARD_B_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card F");
		configSwitch(CARD_B_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_C_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 1 to Card A");
		configSwitch(CARD_C_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 1 to Card B");
		configSwitch(CARD_C_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 1 to Card C");
		configSwitch(CARD_C_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 1 to Card D");
		configSwitch(CARD_C_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 1 to Card E");
		configSwitch(CARD_C_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 1 to Card F");
		configSwitch(CARD_C_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_C_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 2 to Card A");
		configSwitch(CARD_C_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 2 to Card B");
		configSwitch(CARD_C_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 2 to Card C");
		configSwitch(CARD_C_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 2 to Card D");
		configSwitch(CARD_C_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 2 to Card E");
		configSwitch(CARD_C_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 2 to Card F");
		configSwitch(CARD_C_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_D_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card A");
		configSwitch(CARD_D_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card B");
		configSwitch(CARD_D_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card C");
		configSwitch(CARD_D_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card D");
		configSwitch(CARD_D_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card E");
		configSwitch(CARD_D_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card F");
		configSwitch(CARD_D_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_D_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card A");
		configSwitch(CARD_D_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card B");
		configSwitch(CARD_D_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card C");
		configSwitch(CARD_D_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card D");
		configSwitch(CARD_D_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card E");
		configSwitch(CARD_D_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card F");
		configSwitch(CARD_D_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_E_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card A");
		configSwitch(CARD_E_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card B");
		configSwitch(CARD_E_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card C");
		configSwitch(CARD_E_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card D");
		configSwitch(CARD_E_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card E");
		configSwitch(CARD_E_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card F");
		configSwitch(CARD_E_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_E_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card A");
		configSwitch(CARD_E_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card B");
		configSwitch(CARD_E_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card C");
		configSwitch(CARD_E_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card D");
		configSwitch(CARD_E_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card E");
		configSwitch(CARD_E_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card F");
		configSwitch(CARD_E_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_F_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card A");
		configSwitch(CARD_F_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card B");
		configSwitch(CARD_F_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card C");
		configSwitch(CARD_F_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card D");
		configSwitch(CARD_F_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card E");
		configSwitch(CARD_F_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card F");
		configSwitch(CARD_F_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configSwitch(CARD_F_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card A");
		configSwitch(CARD_F_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card B");
		configSwitch(CARD_F_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card C");
		configSwitch(CARD_F_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card D");
		configSwitch(CARD_F_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card E");
		configSwitch(CARD_F_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card F");
		configSwitch(CARD_F_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "");

		configInput(LEFT_LEVEL_INPUT_INPUT, "");
		configInput(RIGHT_LEVEL_INPUT_INPUT, "");
	}

	void process(const ProcessArgs& args) override {
            lights[CARD_A_MIX1_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX1_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX1_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX1_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX1_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX1_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX1_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX1_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX1_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX1_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX1_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX1_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX2_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX2_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX2_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX2_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX2_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX2_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX2_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX2_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX2_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX2_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX2_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX2_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_A_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX1_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX1_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX1_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX1_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX1_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX1_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX1_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX1_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX1_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX1_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX1_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX1_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX2_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX2_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX2_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX2_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX2_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX2_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX2_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX2_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX2_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX2_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX2_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX2_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_B_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX1_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX1_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX1_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX1_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX1_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX1_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX1_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX1_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX1_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX1_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX1_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX1_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX2_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX2_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX2_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX2_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX2_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX2_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX2_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX2_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX2_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX2_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX2_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX2_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_C_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX1_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX1_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX1_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX1_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX1_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX1_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX1_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX1_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX1_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX1_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX1_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX1_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX2_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX2_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX2_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX2_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX2_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX2_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX2_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX2_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX2_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX2_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX2_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX2_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_D_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX1_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX1_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX1_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX1_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX1_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX1_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX1_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX1_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX1_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX1_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX1_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX1_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX2_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX2_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX2_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX2_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX2_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX2_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX2_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX2_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX2_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX2_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX2_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX2_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_E_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX1_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX1_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX1_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX1_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX1_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX1_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX1_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX1_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX1_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX1_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX1_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX1_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX2_CARD_A_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX2_CARD_A_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX2_CARD_B_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX2_CARD_B_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX2_CARD_C_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX2_CARD_C_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX2_CARD_D_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX2_CARD_D_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX2_CARD_E_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX2_CARD_E_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX2_CARD_F_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX2_CARD_F_BUTTON_PARAM].getValue() > 0.f);
            lights[CARD_F_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
	}
};


struct PatchingMatrixWidget : ModuleWidget {
    PatchingMatrixWidget(PatchingMatrix* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/PatchingMatrix.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.0, 38.587)), module, PatchingMatrix::CARD_A_MIX1_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX1_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.6, 38.587)), module, PatchingMatrix::CARD_A_MIX1_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX1_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.199, 38.587)), module, PatchingMatrix::CARD_A_MIX1_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX1_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.798, 38.587)), module, PatchingMatrix::CARD_A_MIX1_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX1_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.398, 38.587)), module, PatchingMatrix::CARD_A_MIX1_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX1_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(98.997, 38.587)), module, PatchingMatrix::CARD_A_MIX1_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX1_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(113.465, 38.587)), module, PatchingMatrix::CARD_A_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.067, 43.278)), module, PatchingMatrix::CARD_A_MIX2_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX2_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.666, 43.278)), module, PatchingMatrix::CARD_A_MIX2_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX2_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.266, 43.278)), module, PatchingMatrix::CARD_A_MIX2_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX2_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.865, 43.278)), module, PatchingMatrix::CARD_A_MIX2_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX2_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.465, 43.278)), module, PatchingMatrix::CARD_A_MIX2_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX2_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(99.064, 43.278)), module, PatchingMatrix::CARD_A_MIX2_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX2_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(125.017, 43.278)), module, PatchingMatrix::CARD_A_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.0, 51.648)), module, PatchingMatrix::CARD_B_MIX1_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX1_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.6, 51.648)), module, PatchingMatrix::CARD_B_MIX1_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX1_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.199, 51.648)), module, PatchingMatrix::CARD_B_MIX1_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX1_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.798, 51.648)), module, PatchingMatrix::CARD_B_MIX1_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX1_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.398, 51.648)), module, PatchingMatrix::CARD_B_MIX1_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX1_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(98.997, 51.648)), module, PatchingMatrix::CARD_B_MIX1_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX1_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(113.465, 51.648)), module, PatchingMatrix::CARD_B_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.067, 56.835)), module, PatchingMatrix::CARD_B_MIX2_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX2_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.666, 56.835)), module, PatchingMatrix::CARD_B_MIX2_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX2_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.266, 56.835)), module, PatchingMatrix::CARD_B_MIX2_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX2_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.865, 56.835)), module, PatchingMatrix::CARD_B_MIX2_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX2_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.465, 56.835)), module, PatchingMatrix::CARD_B_MIX2_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX2_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(99.064, 56.835)), module, PatchingMatrix::CARD_B_MIX2_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX2_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(125.017, 56.835)), module, PatchingMatrix::CARD_B_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.0, 64.71)), module, PatchingMatrix::CARD_C_MIX1_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX1_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.6, 64.71)), module, PatchingMatrix::CARD_C_MIX1_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX1_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.199, 64.71)), module, PatchingMatrix::CARD_C_MIX1_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX1_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.798, 64.71)), module, PatchingMatrix::CARD_C_MIX1_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX1_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.398, 64.71)), module, PatchingMatrix::CARD_C_MIX1_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX1_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(98.997, 64.71)), module, PatchingMatrix::CARD_C_MIX1_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX1_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(113.465, 64.71)), module, PatchingMatrix::CARD_C_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.067, 69.897)), module, PatchingMatrix::CARD_C_MIX2_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX2_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.666, 69.897)), module, PatchingMatrix::CARD_C_MIX2_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX2_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.266, 69.897)), module, PatchingMatrix::CARD_C_MIX2_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX2_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.865, 69.897)), module, PatchingMatrix::CARD_C_MIX2_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX2_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.465, 69.897)), module, PatchingMatrix::CARD_C_MIX2_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX2_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(99.064, 69.897)), module, PatchingMatrix::CARD_C_MIX2_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX2_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(125.017, 69.897)), module, PatchingMatrix::CARD_C_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.0, 77.771)), module, PatchingMatrix::CARD_D_MIX1_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX1_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.6, 77.771)), module, PatchingMatrix::CARD_D_MIX1_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX1_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.199, 77.771)), module, PatchingMatrix::CARD_D_MIX1_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX1_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.798, 77.771)), module, PatchingMatrix::CARD_D_MIX1_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX1_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.398, 77.771)), module, PatchingMatrix::CARD_D_MIX1_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX1_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(98.997, 77.771)), module, PatchingMatrix::CARD_D_MIX1_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX1_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(113.465, 77.771)), module, PatchingMatrix::CARD_D_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.067, 82.958)), module, PatchingMatrix::CARD_D_MIX2_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX2_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.666, 82.958)), module, PatchingMatrix::CARD_D_MIX2_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX2_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.266, 82.958)), module, PatchingMatrix::CARD_D_MIX2_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX2_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.865, 82.958)), module, PatchingMatrix::CARD_D_MIX2_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX2_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.465, 82.958)), module, PatchingMatrix::CARD_D_MIX2_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX2_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(99.064, 82.958)), module, PatchingMatrix::CARD_D_MIX2_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX2_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(125.017, 82.958)), module, PatchingMatrix::CARD_D_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.0, 90.833)), module, PatchingMatrix::CARD_E_MIX1_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX1_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.6, 90.833)), module, PatchingMatrix::CARD_E_MIX1_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX1_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.199, 90.833)), module, PatchingMatrix::CARD_E_MIX1_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX1_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.798, 90.833)), module, PatchingMatrix::CARD_E_MIX1_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX1_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.398, 90.833)), module, PatchingMatrix::CARD_E_MIX1_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX1_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(98.997, 90.833)), module, PatchingMatrix::CARD_E_MIX1_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX1_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(113.465, 90.833)), module, PatchingMatrix::CARD_E_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.067, 96.02)), module, PatchingMatrix::CARD_E_MIX2_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX2_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.666, 96.02)), module, PatchingMatrix::CARD_E_MIX2_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX2_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.266, 96.02)), module, PatchingMatrix::CARD_E_MIX2_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX2_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.865, 96.02)), module, PatchingMatrix::CARD_E_MIX2_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX2_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.465, 96.02)), module, PatchingMatrix::CARD_E_MIX2_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX2_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(99.064, 96.02)), module, PatchingMatrix::CARD_E_MIX2_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX2_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(125.017, 96.02)), module, PatchingMatrix::CARD_E_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.0, 103.894)), module, PatchingMatrix::CARD_F_MIX1_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX1_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.6, 103.894)), module, PatchingMatrix::CARD_F_MIX1_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX1_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.199, 103.894)), module, PatchingMatrix::CARD_F_MIX1_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX1_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.798, 103.894)), module, PatchingMatrix::CARD_F_MIX1_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX1_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.398, 103.894)), module, PatchingMatrix::CARD_F_MIX1_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX1_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(98.997, 103.894)), module, PatchingMatrix::CARD_F_MIX1_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX1_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(113.465, 103.894)), module, PatchingMatrix::CARD_F_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.067, 109.081)), module, PatchingMatrix::CARD_F_MIX2_CARD_A_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_CARD_A_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.666, 109.081)), module, PatchingMatrix::CARD_F_MIX2_CARD_B_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_CARD_B_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(61.266, 109.081)), module, PatchingMatrix::CARD_F_MIX2_CARD_C_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_CARD_C_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(73.865, 109.081)), module, PatchingMatrix::CARD_F_MIX2_CARD_D_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_CARD_D_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(86.465, 109.081)), module, PatchingMatrix::CARD_F_MIX2_CARD_E_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_CARD_E_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(99.064, 109.081)), module, PatchingMatrix::CARD_F_MIX2_CARD_F_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_CARD_F_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(125.017, 109.081)), module, PatchingMatrix::CARD_F_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_OUTPUT_BUTTON_LIGHT));


        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.6, 47.501)), module, PatchingMatrix::LEFT_LEVEL_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.6, 89.669)), module, PatchingMatrix::RIGHT_LEVEL_KNOB_PARAM));


        addParam(createParamCentered<CKSS>(mm2px(Vec(139.661, 17.478)), module, PatchingMatrix::MIX_LEFT_SELECT_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(139.661, 117.692)), module, PatchingMatrix::MIX_RIGHT_SELECT_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.6, 57.166)), module, PatchingMatrix::LEFT_LEVEL_INPUT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.6, 99.334)), module, PatchingMatrix::RIGHT_LEVEL_INPUT_INPUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(147.6, 38.417)), module, PatchingMatrix::LEFT_LEVEL_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(147.6, 80.585)), module, PatchingMatrix::RIGHT_LEVEL_CLIP_LIGHT));

        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 36.769))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 41.46))));
        // mm2px(Vec(14.42, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(5.417, 43.624))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 49.83))));
        // mm2px(Vec(14.42, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(14.326, 52.533))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 55.017))));
        // mm2px(Vec(14.42, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(23.235, 61.442))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 62.892))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 68.078))));
        // mm2px(Vec(14.42, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(32.144, 70.352))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 75.953))));
        // mm2px(Vec(14.42, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(41.053, 79.261))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 81.14))));
        // mm2px(Vec(14.42, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(49.962, 88.17))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 89.015))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 94.201))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 102.076))));
        // mm2px(Vec(27.171, 3.636))
        addChild(createWidget<Widget>(mm2px(Vec(4.17, 107.263))));
    }
};


Model* modelPatchingMatrix = createModel<PatchingMatrix, PatchingMatrixWidget>("PatchingMatrix");
