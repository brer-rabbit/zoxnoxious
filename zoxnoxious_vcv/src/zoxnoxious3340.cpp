#include "plugin.hpp"


static const int num_audio_inputs = 6;
static const int num_audio_outputs = 6;

// taken from Fundamental Audio.cpp

struct AudioPort : audio::Port {
    Module *module;

    dsp::DoubleRingBuffer<dsp::Frame<num_audio_inputs>, 32768> engineInputBuffer;
    dsp::DoubleRingBuffer<dsp::Frame<num_audio_outputs>, 32768> engineOutputBuffer;
    dsp::SampleRateConverter<num_audio_inputs> inputSrc;
    dsp::SampleRateConverter<num_audio_inputs> outputSrc;
    float deviceSampleRate = 0.f;
    int requestedEngineFrames = 0;


    AudioPort(Module* module) {
        this->module = module;
        maxOutputs = num_audio_outputs;
        maxInputs = num_audio_inputs;
        inputSrc.setQuality(6);
        outputSrc.setQuality(6);
    }

    void setMaster(bool master = true) {
        if (master) {
            APP->engine->setMasterModule(module);
        }
        else {
            // Unset master only if module is currently master
            if (isMaster())
                APP->engine->setMasterModule(NULL);
        }
    }

    bool isMaster() {
        return APP->engine->getMasterModule() == module;
    }



    void processInput(const float* input, int inputStride, int frames) override {
        int deviceNumInputs = std::min(getNumInputs(), num_audio_outputs);
        int deviceNumOutputs = std::min(getNumOutputs(), num_audio_inputs);
        deviceSampleRate = getSampleRate();

        if (!APP->engine->getMasterModule()) {
            setMaster();
        }
        bool isMasterCached = isMaster();

        // Set sample rate of engine if engine sample rate is "auto".
        if (isMasterCached) {
            APP->engine->setSuggestedSampleRate(deviceSampleRate);
        }

        float engineSampleRate = APP->engine->getSampleRate();
        float sampleRateRatio = engineSampleRate / deviceSampleRate;

        int maxEngineFrames = (int) std::ceil(frames * sampleRateRatio * 2.0) - 1;

        if (!isMasterCached && (int) engineOutputBuffer.size() > maxEngineFrames) {
            engineOutputBuffer.clear();
        }


        if (deviceNumOutputs > 0) {
            if (isMasterCached) {
                engineOutputBuffer.clear();
            }
            // Set up sample rate converter
            outputSrc.setRates(deviceSampleRate, engineSampleRate);
            outputSrc.setChannels(deviceNumInputs);
            // Convert audio input -> engine output
            dsp::Frame<num_audio_outputs> audioInputBuffer[frames];
            std::memset(audioInputBuffer, 0, sizeof(audioInputBuffer));
            for (int i = 0; i < frames; i++) {
                for (int j = 0; j < deviceNumInputs; j++) {
                    float v = input[i * inputStride + j];
                    audioInputBuffer[i].samples[j] = v;
                }
            }

            int audioInputFrames = frames;
            int outputFrames = engineOutputBuffer.capacity();
            outputSrc.process(input, inputStride, &audioInputFrames, (float*) engineOutputBuffer.endData(), num_audio_outputs, &outputFrames);
            engineOutputBuffer.endIncr(outputFrames);
            // Request exactly as many frames as we have in the engine output buffer.
            requestedEngineFrames = engineOutputBuffer.size();
        }
        else {
            requestedEngineFrames = std::max((int) std::ceil(frames * sampleRateRatio) - (int) engineInputBuffer.size(), 0);
        }

    }


    void processBuffer(const float* input, int inputStride, float* output, int outputStride, int frames) override {
        // Step engine
        if (isMaster() && requestedEngineFrames > 0) {
            // DEBUG("%p: %d block, stepping %d", this, frames, requestedEngineFrames);
            APP->engine->stepBlock(requestedEngineFrames);
        }
    }


    void processOutput(float* output, int outputStride, int frames) override {
        int deviceNumOutputs = std::min(getNumOutputs(), num_audio_inputs);
        float engineSampleRate = APP->engine->getSampleRate();
        float sampleRateRatio = engineSampleRate / deviceSampleRate;


        if (deviceNumOutputs > 0) {
            inputSrc.setRates(engineSampleRate, deviceSampleRate);
            inputSrc.setChannels(num_audio_outputs);
            // Convert engine input -> audio output
            dsp::Frame<num_audio_outputs> audioOutputBuffer[frames];
            int inputFrames = engineInputBuffer.size();
            int outputFrames = frames;
            //inputSrc.process((const float*) engineInputBuffer.startData(), num_audio_inputs, &inputFrames, output, outputStride, &outputFrames);
            inputSrc.process(engineInputBuffer.startData(), &inputFrames, audioOutputBuffer, &outputFrames);
            engineInputBuffer.startIncr(inputFrames);
            // Clamp output samples
            for (int i = 0; i < outputFrames; i++) {
                for (int j = 0; j < num_audio_outputs; j++) {
                    float v = output[i * outputStride + j];
                    v = clamp(v, -1.f, 1.f);
                    output[i * outputStride + j] = v;
                }
            }
            // Fill the rest of the audio output buffer with zeros
            for (int i = outputFrames; i < frames; i++) {
                for (int j = 0; j < num_audio_outputs; j++) {
                    output[i * outputStride + j] = 0.f;
                }
            }
        }

        // If the engine input buffer is too full, clear it to keep latency low.
        int maxEngineFrames = (int) std::ceil(frames * sampleRateRatio * 2.0) - 1;
        if ((int) engineInputBuffer.size() > maxEngineFrames) {
            engineInputBuffer.clear();
        }
    }




    void onStartStream() override {
        engineInputBuffer.clear();
        engineOutputBuffer.clear();
    }

    void onStopStream() override {
        deviceSampleRate = 0.f;
        engineInputBuffer.clear();
        engineOutputBuffer.clear();
        setMaster(false);
    }

};



struct Zoxnoxious3340 : Module {
    enum ParamId {
        FREQ_KNOB_PARAM,
        SYNC_ENABLE_BUTTON_PARAM,
        PULSE_WIDTH_KNOB_PARAM,
        MIX1_TRIANGLE_KNOB_PARAM,
        EXT_MOD_AMOUNT_KNOB_PARAM,
        LINEAR_KNOB_PARAM,
        MIX1_PULSE_BUTTON_PARAM,
        MIX1_SAW_LEVEL_SELECTOR_PARAM,
        EXT_MOD_SELECT_SWITCH_PARAM,
        MIX1_COMPARATOR_BUTTON_PARAM,
        MIX2_PULSE_BUTTON_PARAM,
        EXT_MOD_PWM_BUTTON_PARAM,
        EXP_FM_BUTTON_PARAM,
        LINEAR_FM_BUTTON_PARAM,
        MIX2_SAW_BUTTON_PARAM,
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
        SYNC_ENABLE_LIGHT,
        LIGHTS_LEN
    };

    AudioPort port;

    Zoxnoxious3340() : port(this) {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(FREQ_KNOB_PARAM, 0.f, 1.f, 0.5f, "Frequency", " V", 0.f, 10.f);
        configParam(PULSE_WIDTH_KNOB_PARAM, 0.f, 1.f, 0.5f, "Pulse Width", "%", 0.f, 100.f);
        configParam(LINEAR_KNOB_PARAM, -1.f, 1.f, 0.f, "Linear Mod", " V", 0.f, 5.f);

        configSwitch(MIX1_PULSE_BUTTON_PARAM, 0.f, 1.f, 1.f, "Pulse", {"Off", "On"});
        configParam(MIX1_TRIANGLE_KNOB_PARAM, 0.f, 1.f, 0.f, "Mix1 Triangle Level", "%", 0.f, 100.f);
        configSwitch(MIX1_SAW_LEVEL_SELECTOR_PARAM, 0.f, 2.f, 2.f, "Level", {"Off", "Low", "High"});
        configSwitch(MIX1_COMPARATOR_BUTTON_PARAM, 0.f, 1.f, 0.f, "Mix1 Comparator", {"Off", "On"});

        configSwitch(MIX2_PULSE_BUTTON_PARAM, 0.f, 1.f, 1.f, "Mix2 Pulse", {"Off", "On"});
        configSwitch(MIX2_SAW_BUTTON_PARAM, 0.f, 1.f, 1.f, "Mix2 Saw", {"Off", "On"});

        configSwitch(EXT_MOD_SELECT_SWITCH_PARAM, 0.f, 1.f, 1.f, "Ext Signal", {"1", "2"});
        configParam(EXT_MOD_AMOUNT_KNOB_PARAM, 0.f, 1.f, 1.f, "External Mod Level", "%", 0.f, 100.f);
        configSwitch(EXT_MOD_PWM_BUTTON_PARAM, 0.f, 1.f, 1.f, "Ext Mod to PWM", {"Off", "On"});
        configSwitch(EXP_FM_BUTTON_PARAM, 0.f, 1.f, 1.f, "Ext Mod to Exp FM", {"Off", "On"});
        configSwitch(LINEAR_FM_BUTTON_PARAM, 0.f, 1.f, 1.f, "Ext Mod to Linear FM", {"Off", "On"});

        configSwitch(SYNC_ENABLE_BUTTON_PARAM, 0.f, 1.f, 1.f, "Sync", {"Off", "On"});

        configInput(SYNC_PHASE_INPUT, "Sync Phase");
        configInput(FREQ_INPUT, "Pitch CV");
        configInput(PULSE_WIDTH_INPUT, "Pulse Width Modulation");
        configInput(LINEAR_INPUT, "Linear FM");
        configInput(MIX1_TRIANGLE_VCA_INPUT, "Mix1 Triangle Level");
        configInput(EXT_MOD_AMOUNT_INPUT, "External Modulation Amount");
    }


    ~Zoxnoxious3340() {
        port.setDriverId(-1);
    }



    void process(const ProcessArgs& args) override {
        bool sync = params[SYNC_ENABLE_BUTTON_PARAM].getValue() > 0.f;
        lights[SYNC_ENABLE_LIGHT].setBrightness(sync);

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


        // Push inputs to buffer
        dsp::Frame<num_audio_inputs> inputFrame = {};
        switch (num_audio_inputs) {
        default:
        case 6:
            // linear
            inputFrame.samples[5] =
                params[LINEAR_INPUT].getValue() + inputs[LINEAR_KNOB_PARAM].getVoltageSum() / 10.f;
            // fall through
        case 5:
            // external mod amount
            inputFrame.samples[4] =
                params[EXT_MOD_AMOUNT_INPUT].getValue() + inputs[EXT_MOD_AMOUNT_KNOB_PARAM].getVoltageSum() / 10.f;
            // fall through
        case 4:
            // mix1 triangle
            inputFrame.samples[3] =
                params[MIX1_TRIANGLE_VCA_INPUT].getValue() + inputs[MIX1_TRIANGLE_KNOB_PARAM].getVoltageSum() / 10.f;
            // fall through
        case 3:
            // pulse width
            inputFrame.samples[2] =
                params[PULSE_WIDTH_INPUT].getValue() + inputs[PULSE_WIDTH_KNOB_PARAM].getVoltageSum() / 10.f;
            // fall through
        case 2:
            // sync phase: default to 0.5f if not connected
            inputFrame.samples[1] = inputs[SYNC_PHASE_INPUT].isConnected() ?
                inputs[SYNC_PHASE_INPUT].isConnected() : 0.5f;
            // fall through
        case 1:
            // frequency
            inputFrame.samples[0] =
                params[FREQ_INPUT].getValue() + inputs[FREQ_KNOB_PARAM].getVoltageSum() / 10.f;
            // fall through
        case 0:
            break;
        }


        if (!port.engineInputBuffer.full()) {
            port.engineInputBuffer.push(inputFrame);
        }

    }


};



struct Audio2Display : LedDisplay {
    AudioDeviceMenuChoice* deviceChoice;

    void setAudioPort(audio::Port* port) {
        math::Vec pos;

        deviceChoice = createWidget<AudioDeviceMenuChoice>(math::Vec());
        deviceChoice->box.size.x = box.size.x;
        deviceChoice->port = port;
        addChild(deviceChoice);
        pos = deviceChoice->box.getBottomLeft();

        /*
          deviceSeparator = createWidget<LedDisplaySeparator>(pos);
        deviceSeparator->box.size.x = box.size.x;
        addChild(deviceSeparator);
        */
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

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.732, 26.944)), module, Zoxnoxious3340::FREQ_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.481, 26.944)), module, Zoxnoxious3340::PULSE_WIDTH_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(41.381, 26.944)), module, Zoxnoxious3340::LINEAR_KNOB_PARAM));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(59.699, 27.744)), module, Zoxnoxious3340::MIX1_PULSE_BUTTON_PARAM, Zoxnoxious3340::MIX1_PULSE_BUTTON_LIGHT));


        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(59.777, 43.567)), module, Zoxnoxious3340::MIX1_TRIANGLE_KNOB_PARAM));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(39.949, 62.463)), module, Zoxnoxious3340::SYNC_ENABLE_BUTTON_PARAM, Zoxnoxious3340::SYNC_ENABLE_LIGHT));


        addParam(createParamCentered<CKSSThreeHorizontal>(mm2px(Vec(58.742, 64.968)), module, Zoxnoxious3340::MIX1_SAW_LEVEL_SELECTOR_PARAM));

        addParam(createParamCentered<CKSS>(mm2px(Vec(7.9, 68.8)), module, Zoxnoxious3340::EXT_MOD_SELECT_SWITCH_PARAM));

        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(59.699, 83.967)), module, Zoxnoxious3340::MIX1_COMPARATOR_BUTTON_PARAM, Zoxnoxious3340::MIX1_COMPARATOR_BUTTON_LIGHT));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(9.257, 96.855)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_KNOB_PARAM));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(59.533, 104.804)), module, Zoxnoxious3340::MIX2_PULSE_BUTTON_PARAM, Zoxnoxious3340::MIX2_PULSE_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(9.871, 116.388)), module, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_PARAM, Zoxnoxious3340::EXT_MOD_PWM_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(25.969, 116.388)), module, Zoxnoxious3340::EXP_FM_BUTTON_PARAM, Zoxnoxious3340::EXP_FM_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(41.381, 116.388)), module, Zoxnoxious3340::LINEAR_FM_BUTTON_PARAM, Zoxnoxious3340::LINEAR_FM_BUTTON_LIGHT));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(59.533, 117.049)), module, Zoxnoxious3340::MIX2_SAW_BUTTON_PARAM, Zoxnoxious3340::MIX2_SAW_BUTTON_LIGHT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.732, 37.548)), module, Zoxnoxious3340::FREQ_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.481, 37.548)), module, Zoxnoxious3340::PULSE_WIDTH_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(41.381, 38.138)), module, Zoxnoxious3340::LINEAR_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(59.943, 53.383)), module, Zoxnoxious3340::MIX1_TRIANGLE_VCA_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(39.949, 77.141)), module, Zoxnoxious3340::SYNC_PHASE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.15, 96.855)), module, Zoxnoxious3340::EXT_MOD_AMOUNT_INPUT));

        Audio2Display* display = createWidget<Audio2Display>(mm2px(Vec(50.0, 10.0)));
        display->box.size = mm2px(Vec(25.0, 10.0));
        display->setAudioPort(module ? &module->port : NULL);
        addChild(display);
        //AudioDisplay* display = createWidget<AudioDisplay>(mm2px(Vec(50.0, 10.0)));
        //display->box.size = mm2px(Vec(25.0, 25.00));
        //display->setAudioPort(module ? &module->port : NULL);
        //addChild(display);

    }
};


Model* modelZoxnoxious3340 = createModel<Zoxnoxious3340, Zoxnoxious3340Widget>("Zoxnoxious3340");
