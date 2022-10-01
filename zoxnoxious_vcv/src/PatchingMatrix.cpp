#include "plugin.hpp"

#include "AudioMidi.hpp"
#include "zcomponentlib.hpp"
#include "ZoxnoxiousExpander.hpp"

struct PatchingMatrix : ZoxnoxiousModule {
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
        LEFT_LEVEL_INPUT,
        RIGHT_LEVEL_INPUT,
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

    ZoxnoxiousAudioPort audioPort;
    ZoxnoxiousMidiOutput midiOutput;
    std::deque<midi::Message> midiMessageQueue;

    dsp::ClockDivider lightDivider;
    float leftLevelClipTimer;
    float rightLevelClipTimer;


    PatchingMatrix() : audioPort(this) {
        setExpanderPrimary();

        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configSwitch(MIX_LEFT_SELECT_PARAM, 0.f, 1.f, 0.f, "Left Output", { "Out1", "Out2" });
        configSwitch(MIX_RIGHT_SELECT_PARAM, 0.f, 1.f, 0.f, "Right Output", { "Out1", "Out2" });
        configParam(LEFT_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Left Level", " V", 0.f, 10.f);
        configParam(RIGHT_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Right Level", " V", 0.f, 10.f);

        configSwitch(CARD_A_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card A", { "Off", "On" });
        configSwitch(CARD_A_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card B");
        configSwitch(CARD_A_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card C");
        configSwitch(CARD_A_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card D");
        configSwitch(CARD_A_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card E");
        configSwitch(CARD_A_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Card F");

        configSwitch(CARD_A_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card A");
        configSwitch(CARD_A_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card B");
        configSwitch(CARD_A_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card C");
        configSwitch(CARD_A_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card D");
        configSwitch(CARD_A_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card E");
        configSwitch(CARD_A_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Card F");

        configSwitch(CARD_B_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card A");
        configSwitch(CARD_B_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card B");
        configSwitch(CARD_B_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card C");
        configSwitch(CARD_B_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card D");
        configSwitch(CARD_B_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card E");
        configSwitch(CARD_B_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Card F");

        configSwitch(CARD_B_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card A");
        configSwitch(CARD_B_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card B");
        configSwitch(CARD_B_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card C");
        configSwitch(CARD_B_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card D");
        configSwitch(CARD_B_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card E");
        configSwitch(CARD_B_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Card F");

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

        configSwitch(CARD_D_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card A");
        configSwitch(CARD_D_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card B");
        configSwitch(CARD_D_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card C");
        configSwitch(CARD_D_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card D");
        configSwitch(CARD_D_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card E");
        configSwitch(CARD_D_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Card F");
        configSwitch(CARD_D_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card A");
        configSwitch(CARD_D_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card B");
        configSwitch(CARD_D_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card C");
        configSwitch(CARD_D_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card D");
        configSwitch(CARD_D_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card E");
        configSwitch(CARD_D_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Card F");

        configSwitch(CARD_E_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card A");
        configSwitch(CARD_E_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card B");
        configSwitch(CARD_E_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card C");
        configSwitch(CARD_E_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card D");
        configSwitch(CARD_E_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card E");
        configSwitch(CARD_E_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Card F");
        configSwitch(CARD_E_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card A");
        configSwitch(CARD_E_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card B");
        configSwitch(CARD_E_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card C");
        configSwitch(CARD_E_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card D");
        configSwitch(CARD_E_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card E");
        configSwitch(CARD_E_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Card F");

        configSwitch(CARD_F_MIX1_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card A");
        configSwitch(CARD_F_MIX1_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card B");
        configSwitch(CARD_F_MIX1_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card C");
        configSwitch(CARD_F_MIX1_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card D");
        configSwitch(CARD_F_MIX1_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card E");
        configSwitch(CARD_F_MIX1_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Card F");

        configSwitch(CARD_F_MIX2_CARD_A_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card A");
        configSwitch(CARD_F_MIX2_CARD_B_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card B");
        configSwitch(CARD_F_MIX2_CARD_C_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card C");
        configSwitch(CARD_F_MIX2_CARD_D_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card D");
        configSwitch(CARD_F_MIX2_CARD_E_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card E");
        configSwitch(CARD_F_MIX2_CARD_F_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Card F");


        configSwitch(CARD_A_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 1 to Audio Out");
        configSwitch(CARD_A_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card A Out 2 to Audio Out");
        configSwitch(CARD_B_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 1 to Audio Out");
        configSwitch(CARD_B_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card B Out 2 to Audio Out");
        configSwitch(CARD_C_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 1 to Audio Out");
        configSwitch(CARD_C_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card C Out 2 to Audio Out");
        configSwitch(CARD_D_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 1 to Audio Out");
        configSwitch(CARD_D_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card D Out 2 to Audio Out");
        configSwitch(CARD_E_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 1 to Audio Out");
        configSwitch(CARD_E_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card E Out 2 to Audio Out");
        configSwitch(CARD_F_MIX1_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 1 to Audio Out");
        configSwitch(CARD_F_MIX2_OUTPUT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Card F Out 2 to Audio Out");

        configInput(LEFT_LEVEL_INPUT, "");
        configInput(RIGHT_LEVEL_INPUT, "");

        onReset();
    }


    ~PatchingMatrix() {
        audioPort.setDriverId(-1);
    }

    void onReset() override {
        audioPort.setDriverId(-1);
        midiOutput.reset();
    }


    void onSampleRateChange(const SampleRateChangeEvent& e) override {
        audioPort.engineInputBuffer.clear();
        audioPort.engineOutputBuffer.clear();
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


        // this stuff ought to go in the parent class...
        ZoxnoxiousControlMsg *leftExpanderConsumerMessage;
        if (leftExpander.module != NULL && validLeftExpander) {
            leftExpanderConsumerMessage = static_cast<ZoxnoxiousControlMsg*>(leftExpander.module->rightExpander.consumerMessage);
        }

        // Push inputs to buffer
        //if (audioPort.deviceNumOutputs != maxChannels) {
        //if (APP->engine->getFrame() % 120000 == 0) {
        //INFO("PatchingMatrix unhappy on number of channels");
        //}
        //}

        if (audioPort.deviceNumOutputs > 0) {
            dsp::Frame<maxChannels> inputFrame = {};
            float v;
            const float clipTime = 0.25f;

            // copy expander control voltages to Frame
            //for (int i = 0; i < audioPort.deviceNumOutputs; ++i) {
            for (int i = 0; i < maxChannels; ++i) {
                inputFrame.samples[i] = leftExpanderConsumerMessage->frame[i];
            }

            // add in local control voltages
            switch (audioPort.deviceNumOutputs) {
            default:
            case 2:
                // left level
                v = params[LEFT_LEVEL_KNOB_PARAM].getValue() + inputs[LEFT_LEVEL_INPUT].getVoltageSum() / 10.f;
                inputFrame.samples[cvChannelOffset + 1] = clamp(v, 0.f, 1.f);
                if (inputFrame.samples[cvChannelOffset + 1] != v) {
                    leftLevelClipTimer = clipTime;
                }

                if (APP->engine->getFrame() % 60000 == 0) {
                    INFO("PatchingMatrix: Left: params[LEFT_LEVEL_INPUT].getValue() = %f ; inputs[LEFT_LEVEL_KNOB_PARAM].getVoltageSum() / 10.f = %f ; sum %f",
                         params[LEFT_LEVEL_KNOB_PARAM].getValue(),
                         inputs[LEFT_LEVEL_INPUT].getVoltageSum() / 10.f,
                         v);
                }
                // fall through
            case 1:
                // right level
                v = params[RIGHT_LEVEL_KNOB_PARAM].getValue();
                inputFrame.samples[cvChannelOffset + 0] = clamp(v, 0.f, 1.f);
                if (inputFrame.samples[cvChannelOffset + 0] != v) {
                    rightLevelClipTimer = clipTime;
                }

                // fall through
            case 0:
                break;
            }

            if (!audioPort.engineInputBuffer.full()) {
                audioPort.engineInputBuffer.push(inputFrame);
            }
        }

        if (lightDivider.process()) {
            // slower moving stuff here
            const float lightTime = args.sampleTime * lightDivider.getDivision();
            const float brightnessDeltaTime = 1 / lightTime;

            leftLevelClipTimer -= lightTime;
            lights[LEFT_LEVEL_CLIP_LIGHT].setBrightnessSmooth(leftLevelClipTimer > 0.f, brightnessDeltaTime);

            rightLevelClipTimer -= lightTime;
            lights[RIGHT_LEVEL_CLIP_LIGHT].setBrightnessSmooth(rightLevelClipTimer > 0.f, brightnessDeltaTime);
        }

    }



    /* There really is no proper integration with the parent
     * class... just call this method the Command message is
     * essentially a constant/unchanging from the source.  It keeps
     * getting sent out to account for left-modules moving around an
     * re-positioning.
     */
    

    /** processZoxnoxiousControl
     *
     * intake a control message which contains CV data and possibly a midi message.
     */
    void processZoxnoxiousControl(ZoxnoxiousControlMsg *controlMsg) override {
        // this ought to be the case -- consider making this an assertion
        if (!hasChannelAssignment) {
            if (APP->engine->getFrame() % 60000 == 0) {
                INFO("zoxnoxious: module id %" PRId64 " no channel assignment", getId());
            }
            return;
        }

        // TODO
        // handle midi messaging.  We may have received one via controlMsg.
        // (1) Check buttons, add anything to local queue
        // (2) check control message, send it if received
        // (3) if no control msg, pop from our queue and send a midi message
        // if we have any queued midi messages, send them if possible
        if (controlMsg->midiMessageSet == false && midiMessageQueue.size() > 0) {
            INFO("PatchingMatrix: bus is open, popping MIDI message from queue");
            midiOutput.sendMidiMessage(midiMessageQueue.front());
            midiMessageQueue.pop_front();
        }
        else if (controlMsg->midiMessageSet) {
            midiOutput.sendMidiMessage(controlMsg->midiMessage);
        }

        // copy the control voltages to an output frame and add voltages in for our output
    }


    /** getCardHardwareId
     * return the hardware Id of the 3340 card
     */
    static const uint8_t hardwareId = 0x01;
    uint8_t getHardwareId() override {
        return hardwareId;
    }

    /** initCommandMsgState
     * override and provide data basd on discovery
     */
    void initCommandMsgState() override {
        zCommand_a.authoritativeSource = true;
        // TODO: this is hardcoded for now.  Figure out discovery.
        // channelAssignment data:
        // hardware cardId, channelOffset, assignmentOwned
        // hardcoded/mocked data for now, later this ought to be received
        // via midi from the controlling board
        zCommand_a.channelAssignments[0] = { getHardwareId(), 0, 1, false };
        zCommand_a.channelAssignments[1] = { 0x02, 3, 2, false };
        zCommand_a.channelAssignments[2] = { 0x02, 9, 3, false };
        zCommand_a.channelAssignments[3] = { 0x00, -1, -1, false };
        zCommand_a.channelAssignments[4] = { 0x00, -1, -1, false };
        zCommand_a.channelAssignments[5] = { 0x00, -1, -1, false };
        zCommand_a.channelAssignments[6] = { 0x00, -1, -1, false };
        zCommand_a.channelAssignments[7] = { 0x00, -1, -1, false };

        // take ownership of our card
        processZoxnoxiousCommand(&zCommand_a);

        zCommand_b = zCommand_a;
        INFO("PatchingMatrix: set command msg to authoritative");
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
//        ParamWidget *cardFMix2Output = createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(125.017, 109.081)), module, PatchingMatrix::CARD_F_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_OUTPUT_BUTTON_LIGHT);
//        addChild(cardFMix2Output);
//        cardFMix2Output->setVisible(false);
        

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.6, 47.501)), module, PatchingMatrix::LEFT_LEVEL_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.6, 89.669)), module, PatchingMatrix::RIGHT_LEVEL_KNOB_PARAM));


        addParam(createParamCentered<CKSS>(mm2px(Vec(139.661, 17.478)), module, PatchingMatrix::MIX_LEFT_SELECT_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(139.661, 117.692)), module, PatchingMatrix::MIX_RIGHT_SELECT_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.6, 57.166)), module, PatchingMatrix::LEFT_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.6, 99.334)), module, PatchingMatrix::RIGHT_LEVEL_INPUT));


        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(147.6, 38.417)), module, PatchingMatrix::LEFT_LEVEL_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(147.6, 80.585)), module, PatchingMatrix::RIGHT_LEVEL_CLIP_LIGHT));


        // mm2px(Vec(27.171, 3.636))
        cardAOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 36.769)));
        cardAOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardAOutput1TextField->setText("CardA Out1");
        addChild(cardAOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardAOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 41.46)));
        cardAOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardAOutput2TextField->setText("CardA Out2");
        addChild(cardAOutput2TextField);

        // mm2px(Vec(27.171, 3.636))
        cardBOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 49.83)));
        cardBOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardBOutput1TextField->setText("CardB Out1");
        addChild(cardBOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardBOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 55.017)));
        cardBOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardBOutput2TextField->setText("CardB Out2");
        addChild(cardBOutput2TextField);

        // mm2px(Vec(27.171, 3.636))
        cardCOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 62.892)));
        cardCOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardCOutput1TextField->setText("CardC Out1");
        addChild(cardCOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardCOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 68.078)));
        cardCOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardCOutput2TextField->setText("CardC Out2");
        addChild(cardCOutput2TextField);


        // mm2px(Vec(27.171, 3.636))
        cardDOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 75.953)));
        cardDOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardDOutput1TextField->setText("CardD Out1");
        addChild(cardDOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardDOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 81.14)));
        cardDOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardDOutput2TextField->setText("CardD Out2");
        addChild(cardDOutput2TextField);

        // mm2px(Vec(27.171, 3.636))
        cardEOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 89.015)));
        cardEOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardEOutput1TextField->setText("CardE Out1");
        addChild(cardEOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardEOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 94.201)));
        cardEOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardEOutput2TextField->setText("CardE Out1");
        addChild(cardEOutput2TextField);

        // mm2px(Vec(27.171, 3.636))
        cardFOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 102.076)));
        cardFOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardFOutput1TextField->setText("CardF Out1");
        addChild(cardFOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardFOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(4.17, 107.263)));
        cardFOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardFOutput2TextField->setText("CardF Out2");
        addChild(cardFOutput2TextField);



        // mm2px(Vec(14.42, 3.636))
        cardAInputTextField = createWidget<CardTextDisplay>(mm2px(Vec(33.851, 25.977)));
        cardAInputTextField->box.size = (mm2px(Vec(14.420, 3.636)));
        cardAInputTextField->setText("CardA In");
        cardAInputTextField->setRotation(-3.1416f / 4.f);
        addChild(cardAInputTextField);


        // mm2px(Vec(14.42, 3.636))
        cardBInputTextField = createWidget<CardTextDisplay>(mm2px(Vec(46.451, 25.977)));
        cardBInputTextField->box.size = (mm2px(Vec(14.420, 3.636)));
        cardBInputTextField->setText("CardB In");
        cardBInputTextField->setRotation(-3.1416f / 4.f);
        addChild(cardBInputTextField);


        // mm2px(Vec(14.42, 3.636))
        cardCInputTextField = createWidget<CardTextDisplay>(mm2px(Vec(59.050, 25.977)));
        cardCInputTextField->box.size = (mm2px(Vec(14.420, 3.636)));
        cardCInputTextField->setText("CardC In");
        cardCInputTextField->setRotation(-3.1416f / 4.f);
        addChild(cardCInputTextField);


        // mm2px(Vec(14.42, 3.636))
        cardDInputTextField = createWidget<CardTextDisplay>(mm2px(Vec(71.649, 25.977)));
        cardDInputTextField->box.size = (mm2px(Vec(14.420, 3.636)));
        cardDInputTextField->setText("CardD In");
        cardDInputTextField->setRotation(-3.1416f / 4.f);
        addChild(cardDInputTextField);

        // mm2px(Vec(14.42, 3.636))
        cardEInputTextField = createWidget<CardTextDisplay>(mm2px(Vec(84.248, 25.977)));;
        cardEInputTextField->box.size = (mm2px(Vec(14.420, 3.636)));
        cardEInputTextField->setText("CardE In");
        cardEInputTextField->setRotation(-3.1416f / 4.f);
        addChild(cardEInputTextField);

        // mm2px(Vec(14.42, 3.636))
        cardFInputTextField = createWidget<CardTextDisplay>(mm2px(Vec(96.847, 25.977)));
        cardFInputTextField->box.size = (mm2px(Vec(14.420, 3.636)));
        cardFInputTextField->setText("CardF In");
        cardFInputTextField->setRotation(-3.1416f / 4.f);
        addChild(cardFInputTextField);

    }



    void appendContextMenu(Menu *menu) override {
        PatchingMatrix *module = dynamic_cast<PatchingMatrix*>(this->module);

        menu->addChild(new MenuSeparator);
        menu->addChild(createSubmenuItem("MIDI Device", "",
                                         [=](Menu* menu) {
                                             appendMidiMenu(menu, &module->midiOutput);
                                         }));
        menu->addChild(new MenuSeparator);
        menu->addChild(createSubmenuItem("Audio Device", "",
                                         [=](Menu* menu) {
                                             appendAudioMenu(menu, &module->audioPort);
                                         }));
    }



    CardTextDisplay *cardAInputTextField;
    CardTextDisplay *cardAOutput1TextField;
    CardTextDisplay *cardAOutput2TextField;

    CardTextDisplay *cardBInputTextField;
    CardTextDisplay *cardBOutput1TextField;
    CardTextDisplay *cardBOutput2TextField;

    CardTextDisplay *cardCInputTextField;
    CardTextDisplay *cardCOutput1TextField;
    CardTextDisplay *cardCOutput2TextField;

    CardTextDisplay *cardDInputTextField;
    CardTextDisplay *cardDOutput1TextField;
    CardTextDisplay *cardDOutput2TextField;

    CardTextDisplay *cardEInputTextField;
    CardTextDisplay *cardEOutput1TextField;
    CardTextDisplay *cardEOutput2TextField;

    CardTextDisplay *cardFInputTextField;
    CardTextDisplay *cardFOutput1TextField;
    CardTextDisplay *cardFOutput2TextField;

};


Model* modelPatchingMatrix = createModel<PatchingMatrix, PatchingMatrixWidget>("PatchingMatrix");
