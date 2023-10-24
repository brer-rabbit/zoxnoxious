#include "plugin.hpp"
#include "zcomponentlib.hpp"
#include "ZoxnoxiousExpander.hpp"


const static int midiMessageQueueMaxSize = 16;

// VCO1 ==> SSI2130
// VCO2 ==> AS3394


struct Zoxnoxious5524 : ZoxnoxiousModule {
    enum ParamId {
        VCO_ONE_VOCT_KNOB_PARAM,
        VCO_ONE_PW_KNOB_PARAM,
        VCO_ONE_LINEAR_KNOB_PARAM,
        VCO_TWO_VOCT_KNOB_PARAM,
        VCO_TWO_PW_KNOB_PARAM,
        VCF_CUTOFF_KNOB_PARAM,
        VCF_RESONANCE_KNOB_PARAM,
        VCO_MIX_KNOB_PARAM,
        FINAL_GAIN_KNOB_PARAM,
        VCO_ONE_PULSE_KNOB_PARAM,
        VCO_ONE_TRIANGLE_KNOB_PARAM,
        VCO_ONE_SAW_KNOB_PARAM,
        VCO_TWO_WAVE_PULSE_BUTTON_PARAM,
        VCO_TWO_WAVE_SAW_BUTTON_PARAM,
        VCO_TWO_WAVE_TRI_BUTTON_PARAM,
        VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_PARAM,
        VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_PARAM,
        VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_PARAM,
        VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_PARAM,
        VCO_ONE_MOD_AMOUNT_KNOB_PARAM,
        VCO_TWO_MOD_AMOUNT_KNOB_PARAM,
        VCO_TWO_WAVESHAPE_TZFM_KNOB_PARAM,
        VCO_TWO_TRI_VCF_KNOB_PARAM,
        VCO_ONE_TO_PW_VCO_TWO_BUTTON_PARAM,
        VCO_ONE_TO_VCF_BUTTON_PARAM,
        VCO_TWO_TO_PW_VCO_ONE_BUTTON_PARAM,
        VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        // inputs are in DAC order from schematic
        // DAC AS3394 / SPI chip select 0
        VCO_MIX_INPUT,
        VCO_TWO_PW_INPUT,
        FINAL_GAIN_INPUT,
        VCO_TWO_TRI_VCF_INPUT,
        VCF_RESONANCE_INPUT,
        VCO_ONE_MOD_AMOUNT_INPUT,
        VCO_TWO_VOCT_INPUT,
        VCF_CUTOFF_INPUT,
        // DAC SSI2130 / SPI chip select 1
        VCO_ONE_TRIANGLE_INPUT,
        VCO_ONE_LINEAR_INPUT,
        VCO_ONE_VOCT_INPUT,
        VCO_ONE_PW_INPUT,
        VCO_TWO_WAVESHAPE_TZFM_INPUT,
        VCO_TWO_MOD_AMOUNT_INPUT,
        VCO_ONE_PULSE_INPUT,
        VCO_ONE_SAW_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUTS_LEN
    };
    enum LightId {
        VCO_ONE_VOCT_CLIP_LIGHT,
        VCO_ONE_PW_CLIP_LIGHT,
        VCO_ONE_LINEAR_CLIP_LIGHT,
        VCO_TWO_VOCT_CLIP_LIGHT,
        VCO_TWO_PW_CLIP_LIGHT,
        VCF_CUTOFF_CLIP_LIGHT,
        VCF_RESONANCE_CLIP_LIGHT,
        VCO_MIX_CLIP_LIGHT,
        FINAL_GAIN_CLIP_LIGHT,
        VCO_ONE_PULSE_CLIP_LIGHT,
        VCO_ONE_TRIANGLE_CLIP_LIGHT,
        VCO_ONE_SAW_CLIP_LIGHT,
        VCO_ONE_MOD_AMOUNT_CLIP_LIGHT,
        VCO_TWO_MOD_AMOUNT_CLIP_LIGHT,
        VCO_TWO_WAVESHAPE_TZFM_CLIP_LIGHT,
        VCO_TWO_TRI_VCF_CLIP_LIGHT,
        VCO_TWO_WAVE_PULSE_BUTTON_LIGHT,
        VCO_TWO_WAVE_SAW_BUTTON_LIGHT,
        VCO_TWO_WAVE_TRI_BUTTON_LIGHT,
        VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_LIGHT,
        VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_LIGHT,
        VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_LIGHT,
        VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_LIGHT,
        VCO_ONE_TO_PW_VCO_TWO_BUTTON_LIGHT,
        VCO_ONE_TO_VCF_BUTTON_LIGHT,
        VCO_TWO_TO_PW_VCO_ONE_BUTTON_LIGHT,
        VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_LIGHT,
        ENUMS(LEFT_EXPANDER_LIGHT, 3),
        ENUMS(RIGHT_EXPANDER_LIGHT, 3),
        LIGHTS_LEN
    };

    dsp::ClockDivider lightDivider;
    float vcoOneVoctClipTimer;
    float vcoOnePwClipTimer;
    float vcoOneLinearClipTimer;
    float vcoTwoVoctClipTimer;
    float vcoTwoPwClipTimer;
    float vcfCutoffClipTimer;
    float vcfResonanceClipTimer;
    float vcoMixClipTimer;
    float finalGainClipTimer;
    float vcoOnePulseClipTimer;
    float vcoOneTriangleClipTimer;
    float vcoOneSawClipTimer;
    float vcoOneModAmountClipTimer;
    float vcoTwoModAmountClipTimer;
    float vcoTwoWaveshapeTzfmClipTimer;
    float vcoTwoTriVcfClipTimer;


    std::deque<midi::Message> midiMessageQueue;

    std::string output1NameString;
    std::string output2NameString;


    // mapping button switches to send MIDI program changes.

    // VCO2 Saw and Tri params send a single MIDI prog change.
    // the const is the program change offset from zero for the program change.
    // Four program changes handle off/off, off/on, on/off, on/on.
    // With a value of 0, the prog changes for tri/saw are 0,1,2,3.
    const uint8_t vcoTwoSawTriMidiProgramOffset = 0;
    uint8_t vcoTwoTriSawPrevState = 255; // init to invalid state


    // Detect state changes by tracking previousValue, wiht INT_MIN
    // being an init value (all value will be detected to change first
    // clock cycle).  

    struct buttonParamMidiProgram {
        const enum ParamId button;
        int previousValue;
        const uint8_t midiProgram[8];
    } buttonParamToMidiProgramList[8] =
      {
          { VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_PARAM, INT_MIN, { 4, 5 } },
          { VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_PARAM, INT_MIN, { 6, 7 } },
          { VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_PARAM, INT_MIN, { 8, 9 } },
          { VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_PARAM, INT_MIN, { 10, 11 } },
          { VCO_ONE_TO_PW_VCO_TWO_BUTTON_PARAM, INT_MIN, { 12, 13 } },
          { VCO_ONE_TO_VCF_BUTTON_PARAM, INT_MIN, { 14, 15 } },
          { VCO_TWO_TO_PW_VCO_ONE_BUTTON_PARAM, INT_MIN, { 16, 17 } },
          { VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_PARAM, INT_MIN, { 18, 19 } }
      };

    // VCO Two pulse is enabled/disabled by setting the pulse width to
    // minimum value.  Track that outside of buttonParamToMidiProgramList.
    bool vcoTwoPulseEnabled = false;


    Zoxnoxious5524() :
        vcoOneVoctClipTimer(0.f),
        vcoOnePwClipTimer(0.f),
        vcoOneLinearClipTimer(0.f),
        vcoTwoVoctClipTimer(0.f),
        vcoTwoPwClipTimer(0.f),
        vcfCutoffClipTimer(0.f),
        vcfResonanceClipTimer(0.f),
        vcoMixClipTimer(0.f),
        finalGainClipTimer(0.f),
        vcoOnePulseClipTimer(0.f),
        vcoOneTriangleClipTimer(0.f),
        vcoOneSawClipTimer(0.f),
        vcoOneModAmountClipTimer(0.f),
        vcoTwoModAmountClipTimer(0.f),
        vcoTwoWaveshapeTzfmClipTimer(0.f),
        vcoTwoTriVcfClipTimer(0.f),
        output1NameString(invalidCardOutputName),
        output2NameString(invalidCardOutputName) {

        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(VCO_ONE_VOCT_KNOB_PARAM, 0.f, 1.f, 0.5f, "Frequency", " V", 0.f, 8.f);
        configParam(VCO_ONE_PW_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pulse Width", "%", 0.f, 100.f);
        configParam(VCO_ONE_LINEAR_KNOB_PARAM, 0.f, 1.f, 0.5f, "Linear Mod", " V", 0.f, 10.f -5.f);
        configParam(VCO_TWO_VOCT_KNOB_PARAM, 0.f, 1.f, 0.5f, "Frequency", " V", 0.f, 8.f);
        configParam(VCO_TWO_PW_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pulse Width", "%", 0.f, 100.f);
        configParam(VCF_CUTOFF_KNOB_PARAM, 0.f, 1.f, 1.f, "Cutoff", " V", 0.f, 10.f);
        configParam(VCF_RESONANCE_KNOB_PARAM, 0.f, 1.f, 0.f, "Resonance", "%", 0.f, 100.f);
        configParam(VCO_MIX_KNOB_PARAM, 0.f, 1.f, 0.5f, "VCO1/VCO2 Mix", "%", 0.f, 200.f, -100.f);
        configParam(FINAL_GAIN_KNOB_PARAM, 0.f, 1.f, 0.f, "Final VCA", "%", 0.f, 100.f);
        configParam(VCO_ONE_PULSE_KNOB_PARAM, 0.f, 1.f, 0.f, "Pulse Gain", "%", 0.f, 100.f);
        configParam(VCO_ONE_TRIANGLE_KNOB_PARAM, 0.f, 1.f, 1.f, "Tri Gain", "%", 0.f, 100.f);
        configParam(VCO_ONE_SAW_KNOB_PARAM, 0.f, 1.f, 0.f, "Saw Gain", "%", 0.f, 100.f);

        configParam(VCO_ONE_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Mod Amount", "%", 0.f, 100.f);
        configParam(VCO_TWO_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 0.f, "Mod Amount", "%", 0.f, 100.f);
        configParam(VCO_TWO_WAVESHAPE_TZFM_KNOB_PARAM, 0.f, 1.f, 0.f, "Waveshape TZFM", "%", 0.f, 100.f);
        configParam(VCO_TWO_TRI_VCF_KNOB_PARAM, 0.f, 1.f, 0.f, "Tri to VCF", "%", 0.f, 100.f);

        configSwitch(VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO One to Exp FM VCO Two", {"Off", "On"});
        configSwitch(VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO One to Wave Select VCO Two", {"Off", "On"});

        configSwitch(VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two to TZ-FM VCO One", {"Off", "On"});
        configSwitch(VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Soft Sync VCO One from VCO Two", {"Off", "On"});
        configSwitch(VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Hard Sub Sync VCO One from VCO Two", {"Off", "On"});

        configSwitch(VCO_ONE_TO_PW_VCO_TWO_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO One to VCO Two Pulse Width", {"Off", "On"});
        configSwitch(VCO_ONE_TO_VCF_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO One to VCF Freq Cutoff", {"Off", "On"});
        configSwitch(VCO_TWO_TO_PW_VCO_ONE_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two to VCO One Pulse Width", {"Off", "On"});

        configSwitch(VCO_TWO_WAVE_PULSE_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two Pulse Enable", {"Off", "On"});
        configSwitch(VCO_TWO_WAVE_SAW_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two Sawtooth Enable", {"Off", "On"});
        configSwitch(VCO_TWO_WAVE_TRI_BUTTON_PARAM, 0.f, 1.f, 0.f, "VCO Two Triangle Enable", {"Off", "On"});

        configInput(VCO_ONE_VOCT_INPUT, "VCO One V/Oct");
        configInput(VCO_ONE_PW_INPUT, "VCO One Pulse Width");
        configInput(VCO_ONE_LINEAR_INPUT, "VCO One Linear Freq Mod");
        configInput(VCO_TWO_VOCT_INPUT, "VCO Two V/Oct");
        configInput(VCO_TWO_PW_INPUT, "VCO Two Pulse Width");
        configInput(VCF_CUTOFF_INPUT, "VCF Cutoff Frequency");
        configInput(VCF_RESONANCE_INPUT, "VCF Resonance");
        configInput(VCO_MIX_INPUT, "VCO One / VCO Two Mix");
        configInput(FINAL_GAIN_INPUT, "Final VCA Gain");
        configInput(VCO_ONE_PULSE_INPUT, "VCO One Pulse Level");
        configInput(VCO_ONE_TRIANGLE_INPUT, "VCO One Triangle Level");
        configInput(VCO_ONE_SAW_INPUT, "VCO One Sawtooth Level");
        configInput(VCO_ONE_MOD_AMOUNT_INPUT, "VCO One Mod Level");
        configInput(VCO_TWO_MOD_AMOUNT_INPUT, "VCO Two Mod Level");
        configInput(VCO_TWO_WAVESHAPE_TZFM_INPUT, "VCO Two Waveshape to VCO One TZFM");
        configInput(VCO_TWO_TRI_VCF_INPUT, "VCO Two Triangel to VCF Cutoff Frequency");

        lightDivider.setDivision(512);
    }

    void process(const ProcessArgs& args) override {
        processExpander(args);

        if (lightDivider.process()) {
            lights[VCO_TWO_WAVE_PULSE_BUTTON_LIGHT].setBrightness(
                params[VCO_TWO_WAVE_PULSE_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_TWO_WAVE_SAW_BUTTON_LIGHT].setBrightness(
                params[VCO_TWO_WAVE_SAW_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_TWO_WAVE_TRI_BUTTON_LIGHT].setBrightness(
                params[VCO_TWO_WAVE_TRI_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_LIGHT].setBrightness(
                params[VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_LIGHT].setBrightness(
                params[VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_LIGHT].setBrightness(
                params[VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_LIGHT].setBrightness(
                params[VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_ONE_TO_PW_VCO_TWO_BUTTON_LIGHT].setBrightness(
                params[VCO_ONE_TO_PW_VCO_TWO_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_ONE_TO_VCF_BUTTON_LIGHT].setBrightness(
                params[VCO_ONE_TO_VCF_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_TWO_TO_PW_VCO_ONE_BUTTON_LIGHT].setBrightness(
                params[VCO_TWO_TO_PW_VCO_ONE_BUTTON_PARAM].getValue() > 0.f
            );

            lights[VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_LIGHT].setBrightness(
                params[VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_PARAM].getValue() > 0.f
            );


            // clipping lights and timers
            const float lightTime = args.sampleTime * lightDivider.getDivision();
            const float brightnessDeltaTime = 1 / lightTime;

            vcoOneVoctClipTimer -= lightTime;
            lights[VCO_ONE_VOCT_CLIP_LIGHT].setBrightnessSmooth(vcoOneVoctClipTimer > 0.f, brightnessDeltaTime);

            vcoOnePwClipTimer -= lightTime;
            lights[VCO_ONE_PW_CLIP_LIGHT].setBrightnessSmooth(vcoOnePwClipTimer > 0.f, brightnessDeltaTime);

            vcoOneLinearClipTimer -= lightTime;
            lights[VCO_ONE_LINEAR_CLIP_LIGHT].setBrightnessSmooth(vcoOneLinearClipTimer > 0.f, brightnessDeltaTime);

            vcoTwoVoctClipTimer -= lightTime;
            lights[VCO_TWO_VOCT_CLIP_LIGHT].setBrightnessSmooth(vcoTwoVoctClipTimer > 0.f, brightnessDeltaTime);

            vcoTwoPwClipTimer -= lightTime;
            lights[VCO_TWO_PW_CLIP_LIGHT].setBrightnessSmooth(vcoTwoPwClipTimer > 0.f, brightnessDeltaTime);

            vcfCutoffClipTimer -= lightTime;
            lights[VCF_CUTOFF_CLIP_LIGHT].setBrightnessSmooth(vcfCutoffClipTimer > 0.f, brightnessDeltaTime);

            vcfResonanceClipTimer -= lightTime;
            lights[VCF_RESONANCE_CLIP_LIGHT].setBrightnessSmooth(vcfResonanceClipTimer > 0.f, brightnessDeltaTime);

            vcoMixClipTimer -= lightTime;
            lights[VCO_MIX_CLIP_LIGHT].setBrightnessSmooth(vcoMixClipTimer > 0.f, brightnessDeltaTime);

            finalGainClipTimer -= lightTime;
            lights[FINAL_GAIN_CLIP_LIGHT].setBrightnessSmooth(finalGainClipTimer > 0.f, brightnessDeltaTime);

            vcoOnePulseClipTimer -= lightTime;
            lights[VCO_ONE_PULSE_CLIP_LIGHT].setBrightnessSmooth(vcoOnePulseClipTimer > 0.f, brightnessDeltaTime);

            vcoOneTriangleClipTimer -= lightTime;
            lights[VCO_ONE_TRIANGLE_CLIP_LIGHT].setBrightnessSmooth(vcoOneTriangleClipTimer > 0.f, brightnessDeltaTime);

            vcoOneSawClipTimer -= lightTime;
            lights[VCO_ONE_SAW_CLIP_LIGHT].setBrightnessSmooth(vcoOneSawClipTimer > 0.f, brightnessDeltaTime);

            vcoOneModAmountClipTimer -= lightTime;
            lights[VCO_ONE_MOD_AMOUNT_CLIP_LIGHT].setBrightnessSmooth(vcoOneModAmountClipTimer > 0.f, brightnessDeltaTime);

            vcoTwoModAmountClipTimer -= lightTime;
            lights[VCO_TWO_MOD_AMOUNT_CLIP_LIGHT].setBrightnessSmooth(vcoTwoModAmountClipTimer > 0.f, brightnessDeltaTime);

            vcoTwoWaveshapeTzfmClipTimer -= lightTime;
            lights[VCO_TWO_WAVESHAPE_TZFM_CLIP_LIGHT].setBrightnessSmooth(vcoTwoWaveshapeTzfmClipTimer > 0.f, brightnessDeltaTime);

            vcoTwoTriVcfClipTimer -= lightTime;
            lights[VCO_TWO_TRI_VCF_CLIP_LIGHT].setBrightnessSmooth(vcoTwoTriVcfClipTimer > 0.f, brightnessDeltaTime);

            setLeftExpanderLight(LEFT_EXPANDER_LIGHT);
            setRightExpanderLight(RIGHT_EXPANDER_LIGHT);

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
            controlMsg->midiMessageSet = true;
            controlMsg->midiMessage = midiMessageQueue.front();
            midiMessageQueue.pop_front();
        }


        // Any buttons params pushed need to send midi events.  Send directly or queue.
        for (int i = 0; i < (int) (sizeof(buttonParamToMidiProgramList) / sizeof(struct buttonParamMidiProgram)); ++i) {
            int newValue = (int) (params[ buttonParamToMidiProgramList[i].button ].getValue() + 0.5f);
            sendOrQueueMidiMessage(controlMsg, newValue, i);
        }

        // convert the params to the MIDI prog change number: 0, 1, 2, 3
        int vcoTwoSaw = static_cast<int>(std::round(params[VCO_TWO_WAVE_SAW_BUTTON_PARAM].getValue())) * 2;
        int vcoTwoTri = static_cast<int>(std::round(params[VCO_TWO_WAVE_TRI_BUTTON_PARAM].getValue()));
        int vcoTwoTriSawCurrentState = vcoTwoSaw + vcoTwoTri + vcoTwoSawTriMidiProgramOffset;
        if (vcoTwoTriSawCurrentState != vcoTwoTriSawPrevState) {
            vcoTwoTriSawPrevState = vcoTwoTriSawCurrentState;

            // either send the message right now if slot open or queue it
            if (controlMsg->midiMessageSet == false) {
                // send direct
                controlMsg->midiMessage.setSize(2);
                controlMsg->midiMessage.setChannel(midiChannel);
                controlMsg->midiMessage.setStatus(midiProgramChangeStatus);
                controlMsg->midiMessage.setNote(vcoTwoTriSawPrevState);
                controlMsg->midiMessageSet = true;
            }
            else if (midiMessageQueue.size() < midiMessageQueueMaxSize) {
                midi::Message queuedMessage;
                queuedMessage.setSize(2);
                queuedMessage.setChannel(midiChannel);
                queuedMessage.setStatus(midiProgramChangeStatus);
                queuedMessage.setNote(vcoTwoTriSawPrevState);
                midiMessageQueue.push_back(queuedMessage);
            }
            else {
                INFO("Zoxnoxioius5524: dropping MIDI message, bus full and queue full");
            }

        }


        float v;
        const float clipTime = 0.25f;

        // Sequence of output: this needs to match what the Raspberry
        // Pi driver expects.  Put the values in that order.

        // 2130 Saw Level
        v = params[VCO_ONE_SAW_KNOB_PARAM].getValue() + inputs[VCO_ONE_SAW_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 15] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 15] != v) {
            vcoOneSawClipTimer = clipTime;
        }

        // 2130 Pulse Level
        v = params[VCO_ONE_PULSE_KNOB_PARAM].getValue() + inputs[VCO_ONE_PULSE_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 14] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 14] != v) {
            vcoOnePulseClipTimer = clipTime;
        }

        // 3394 Mod Amount VCA
        v = params[VCO_TWO_MOD_AMOUNT_KNOB_PARAM].getValue() + inputs[VCO_TWO_MOD_AMOUNT_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 13] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 13] != v) {
            vcoTwoModAmountClipTimer = clipTime;
        }

        // 3394 Waveshaped output to 2130 FM
        v = params[VCO_TWO_WAVESHAPE_TZFM_KNOB_PARAM].getValue() + inputs[VCO_TWO_WAVESHAPE_TZFM_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 12] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 12] != v) {
            vcoTwoWaveshapeTzfmClipTimer = clipTime;
        }

        // VCO One Pulse Width
        v = params[VCO_ONE_PW_KNOB_PARAM].getValue() + inputs[VCO_ONE_PW_INPUT].getVoltageSum() / 10.f;
        // TODO: Handle the PWM tuning on the Pi, allow for clamp(v, 0.f, 1.f)
        v = v * 0.9 + 0.05;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 11] = clamp(v, 0.05f, 0.95f); 
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 11] != v) {
            vcoOnePwClipTimer = clipTime;
        }

        // VCO One Volt/Octave
        v = params[VCO_ONE_VOCT_KNOB_PARAM].getValue() + inputs[VCO_ONE_VOCT_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 10] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 10] != v) {
            vcoOneVoctClipTimer = clipTime;
        }

        // VCO One Linear TZFM
        v = params[VCO_ONE_LINEAR_KNOB_PARAM].getValue() + inputs[VCO_ONE_LINEAR_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 9] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 9] != v) {
            vcoOneLinearClipTimer = clipTime;
        }

        // 2130 Triangle Level
        v = params[VCO_ONE_TRIANGLE_KNOB_PARAM].getValue() + inputs[VCO_ONE_TRIANGLE_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 8] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 8] != v) {
            vcoOneTriangleClipTimer = clipTime;
        }

        // 3394 VCF Cutoff
        v = params[VCF_CUTOFF_KNOB_PARAM].getValue() + inputs[VCF_CUTOFF_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 7] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 7] != v) {
            vcfCutoffClipTimer = clipTime;
        }

        // VCO Two Volt/Octave
        v = params[VCO_TWO_VOCT_KNOB_PARAM].getValue() + inputs[VCO_TWO_VOCT_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 6] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 6] != v) {
            vcoTwoVoctClipTimer = clipTime;
        }

        // 2130 Mod Amount VCA
        v = params[VCO_ONE_MOD_AMOUNT_KNOB_PARAM].getValue() + inputs[VCO_ONE_MOD_AMOUNT_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 5] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 5] != v) {
            vcoOneModAmountClipTimer = clipTime;
        }

        // 3394 VCF Resonance
        v = params[VCF_RESONANCE_KNOB_PARAM].getValue() + inputs[VCF_RESONANCE_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 4] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 4] != v) {
            vcfResonanceClipTimer = clipTime;
        }

        // 3394 Triangle to VCF
        v = params[VCO_TWO_TRI_VCF_KNOB_PARAM].getValue() + inputs[VCO_TWO_TRI_VCF_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 3] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 3] != v) {
            vcoTwoTriVcfClipTimer = clipTime;
        }

        // Final Gain VCA on 3394
        v = params[FINAL_GAIN_KNOB_PARAM].getValue() + inputs[FINAL_GAIN_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset + 2] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset + 2] != v) {
            finalGainClipTimer = clipTime;
        }

        // VCO Two PW
        // Note Pulse Width handles enable/disable
        vcoTwoPulseEnabled = static_cast<bool>(std::round(params[VCO_TWO_WAVE_PULSE_BUTTON_PARAM].getValue()));
        if (vcoTwoPulseEnabled) {
            v = params[VCO_TWO_PW_KNOB_PARAM].getValue() + inputs[VCO_TWO_PW_INPUT].getVoltageSum() / 10.f;
            // TODO: Handle the PWM tuning on the Pi, allow for clamp(v, 0.f, 1.f)
            v = v * 0.9 + 0.05;
            controlMsg->frame[outputDeviceId][cvChannelOffset + 1] = clamp(v, 0.05f, 0.95f);
            if (controlMsg->frame[outputDeviceId][cvChannelOffset + 1] != v) {
                vcoTwoPwClipTimer = clipTime;
            }
        }
        else {
            controlMsg->frame[outputDeviceId][cvChannelOffset + 1] = 0.f;
        }

        // VCO One / VCO Two Mix to filter on 3394
        v = params[VCO_MIX_KNOB_PARAM].getValue() + inputs[VCO_MIX_INPUT].getVoltageSum() / 10.f;
        controlMsg->frame[outputDeviceId][cvChannelOffset] = clamp(v, 0.f, 1.f);
        if (controlMsg->frame[outputDeviceId][cvChannelOffset] != v) {
            vcoMixClipTimer = clipTime;
        }

    }


    /** getCardHardwareId
     * return the hardware Id of the card
     */
    static const uint8_t hardwareId = 0x04;
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



    private:
    void sendOrQueueMidiMessage(ZoxnoxiousControlMsg *controlMsg, int newValue, int index) {
        if (buttonParamToMidiProgramList[index].previousValue != newValue) {
            buttonParamToMidiProgramList[index].previousValue = newValue;
            if (controlMsg->midiMessageSet == false) {
                // send direct
                controlMsg->midiMessage.setSize(2);
                controlMsg->midiMessage.setChannel(midiChannel);
                controlMsg->midiMessage.setStatus(midiProgramChangeStatus);
                controlMsg->midiMessage.setNote(buttonParamToMidiProgramList[index].midiProgram[newValue]);
                controlMsg->midiMessageSet = true;
                INFO("zoxnoxious5524: clock %" PRId64 " :  MIDI message direct midi channel %d", APP->engine->getFrame(), midiChannel);
            }
            else if (midiMessageQueue.size() < midiMessageQueueMaxSize) {
                midi::Message queuedMessage;
                queuedMessage.setSize(2);
                queuedMessage.setChannel(midiChannel);
                queuedMessage.setStatus(midiProgramChangeStatus);
                queuedMessage.setNote(buttonParamToMidiProgramList[index].midiProgram[newValue]);
                midiMessageQueue.push_back(queuedMessage);
            }
            else {
                INFO("Zoxnoxious5524: dropping MIDI message, bus full and queue full");
            }
        }
    }


};


struct Zoxnoxious5524Widget : ModuleWidget {
    Zoxnoxious5524Widget(Zoxnoxious5524* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Zoxnoxious5524.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.298, 25.908)), module, Zoxnoxious5524::VCO_ONE_VOCT_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.903, 25.908)), module, Zoxnoxious5524::VCO_ONE_PW_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(49.645, 25.908)), module, Zoxnoxious5524::VCO_ONE_LINEAR_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(73.245, 25.908)), module, Zoxnoxious5524::VCO_TWO_VOCT_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(90.987, 25.908)), module, Zoxnoxious5524::VCO_TWO_PW_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(127.94, 25.908)), module, Zoxnoxious5524::VCF_CUTOFF_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.544, 25.908)), module, Zoxnoxious5524::VCF_RESONANCE_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(127.94, 62.522)), module, Zoxnoxious5524::VCO_MIX_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.544, 62.522)), module, Zoxnoxious5524::FINAL_GAIN_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.298, 65.704)), module, Zoxnoxious5524::VCO_ONE_PULSE_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(49.645, 65.594)), module, Zoxnoxious5524::VCO_ONE_TRIANGLE_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.903, 65.867)), module, Zoxnoxious5524::VCO_ONE_SAW_KNOB_PARAM));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.705, 102.585)), module, Zoxnoxious5524::VCO_ONE_MOD_AMOUNT_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(73.225, 103.172)), module, Zoxnoxious5524::VCO_TWO_MOD_AMOUNT_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(127.94, 103.172)), module, Zoxnoxious5524::VCO_TWO_WAVESHAPE_TZFM_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(147.045, 103.397)), module, Zoxnoxious5524::VCO_TWO_TRI_VCF_KNOB_PARAM));


        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(73.245, 66.311)), module, Zoxnoxious5524::VCO_TWO_WAVE_PULSE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_WAVE_PULSE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(90.518, 66.311)), module, Zoxnoxious5524::VCO_TWO_WAVE_SAW_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_WAVE_SAW_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(107.792, 66.311)), module, Zoxnoxious5524::VCO_TWO_WAVE_TRI_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_WAVE_TRI_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(31.971, 100.58)), module, Zoxnoxious5524::VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_PARAM, Zoxnoxious5524::VCO_ONE_TO_EXP_FM_VCO_TWO_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(47.31, 100.58)), module, Zoxnoxious5524::VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_PARAM, Zoxnoxious5524::VCO_ONE_TO_WAVE_SELECT_VCO_TWO_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(90.846, 100.58)), module, Zoxnoxious5524::VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_TO_FREQ_VCO_ONE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(110.444, 100.58)), module, Zoxnoxious5524::VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_TO_SOFT_SYNC_VCO_ONE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(31.971, 115.486)), module, Zoxnoxious5524::VCO_ONE_TO_PW_VCO_TWO_BUTTON_PARAM, Zoxnoxious5524::VCO_ONE_TO_PW_VCO_TWO_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(47.31, 115.486)), module, Zoxnoxious5524::VCO_ONE_TO_VCF_BUTTON_PARAM, Zoxnoxious5524::VCO_ONE_TO_VCF_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(90.846, 115.486)), module, Zoxnoxious5524::VCO_TWO_TO_PW_VCO_ONE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_TO_PW_VCO_ONE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(110.444, 115.486)), module, Zoxnoxious5524::VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_PARAM, Zoxnoxious5524::VCO_TWO_TO_HARD_SYNC_VCO_ONE_BUTTON_LIGHT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.298, 39.271)), module, Zoxnoxious5524::VCO_ONE_VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.903, 39.271)), module, Zoxnoxious5524::VCO_ONE_PW_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(49.645, 39.271)), module, Zoxnoxious5524::VCO_ONE_LINEAR_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(73.245, 39.271)), module, Zoxnoxious5524::VCO_TWO_VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(90.987, 39.271)), module, Zoxnoxious5524::VCO_TWO_PW_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(127.94, 39.271)), module, Zoxnoxious5524::VCF_CUTOFF_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.544, 39.271)), module, Zoxnoxious5524::VCF_RESONANCE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(127.94, 75.884)), module, Zoxnoxious5524::VCO_MIX_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.544, 75.884)), module, Zoxnoxious5524::FINAL_GAIN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.298, 79.066)), module, Zoxnoxious5524::VCO_ONE_PULSE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(49.645, 78.956)), module, Zoxnoxious5524::VCO_ONE_TRIANGLE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.903, 79.229)), module, Zoxnoxious5524::VCO_ONE_SAW_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.705, 115.948)), module, Zoxnoxious5524::VCO_ONE_MOD_AMOUNT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(73.225, 116.534)), module, Zoxnoxious5524::VCO_TWO_MOD_AMOUNT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(127.94, 116.534)), module, Zoxnoxious5524::VCO_TWO_WAVESHAPE_TZFM_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(147.045, 116.759)), module, Zoxnoxious5524::VCO_TWO_TRI_VCF_INPUT));

        addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(2.020, 8.219)), module, Zoxnoxious5524::LEFT_EXPANDER_LIGHT));
        addChild(createLightCentered<TriangleRightLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(160.346, 8.219)), module, Zoxnoxious5524::RIGHT_EXPANDER_LIGHT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.261, 31.798)), module, Zoxnoxious5524::VCO_ONE_VOCT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(35.865, 31.798)), module, Zoxnoxious5524::VCO_ONE_PW_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.608, 31.798)), module, Zoxnoxious5524::VCO_ONE_LINEAR_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(77.208, 31.798)), module, Zoxnoxious5524::VCO_TWO_VOCT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(94.95, 31.798)), module, Zoxnoxious5524::VCO_TWO_PW_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(131.902, 31.798)), module, Zoxnoxious5524::VCF_CUTOFF_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(151.507, 31.798)), module, Zoxnoxious5524::VCF_RESONANCE_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(131.902, 68.411)), module, Zoxnoxious5524::VCO_MIX_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(151.507, 68.411)), module, Zoxnoxious5524::FINAL_GAIN_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.261, 71.704)), module, Zoxnoxious5524::VCO_ONE_PULSE_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.608, 71.593)), module, Zoxnoxious5524::VCO_ONE_TRIANGLE_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(35.866, 71.867)), module, Zoxnoxious5524::VCO_ONE_SAW_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.668, 108.585)), module, Zoxnoxious5524::VCO_ONE_MOD_AMOUNT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(77.188, 109.172)), module, Zoxnoxious5524::VCO_TWO_MOD_AMOUNT_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(131.902, 109.172)), module, Zoxnoxious5524::VCO_TWO_WAVESHAPE_TZFM_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(151.008, 109.286)), module, Zoxnoxious5524::VCO_TWO_TRI_VCF_CLIP_LIGHT));


        mix1OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(139.527, 50.578)));
        mix1OutputTextField->box.size = (mm2px(Vec(18.0, 3.636)));
        mix1OutputTextField->setText(module ? &module->output1NameString : NULL);
        addChild(mix1OutputTextField);

        mix2OutputTextField = createWidget<CardTextDisplay>(mm2px(Vec(40.269, 48.012)));
        mix2OutputTextField->box.size = (mm2px(Vec(18.0, 3.636)));
        mix2OutputTextField->setText(module ? &module->output2NameString : NULL);
        addChild(mix2OutputTextField);
    }

    CardTextDisplay *mix1OutputTextField;
    CardTextDisplay *mix2OutputTextField;
};


Model* modelZoxnoxious5524 = createModel<Zoxnoxious5524, Zoxnoxious5524Widget>("Zoxnoxious5524");
