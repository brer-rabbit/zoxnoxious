#include "plugin.hpp"

#include "AudioMidi.hpp"
#include "zcomponentlib.hpp"
#include "ZoxnoxiousExpander.hpp"

struct PatchingMatrix : ZoxnoxiousModule {
    enum ParamId {
        MIX_LEFT_SELECT_PARAM,
        LEFT_LEVEL_KNOB_PARAM,
        RIGHT_LEVEL_KNOB_PARAM,
        CARD_A_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_A_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_B_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_B_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_C_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_C_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_D_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_D_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_E_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_E_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_F_MIX1_OUTPUT_BUTTON_PARAM,
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
        CARD_A_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_A_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_B_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_B_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_C_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_C_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_D_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_D_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_E_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_E_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_F_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_F_MIX2_OUTPUT_BUTTON_LIGHT,
        ENUMS(LEFT_EXPANDER_LIGHT, 3),
        ENUMS(RIGHT_EXPANDER_LIGHT, 3),
        LIGHTS_LEN
    };

    ZoxnoxiousAudioPort audioPort;
    ZoxnoxiousMidiOutput midiOutput;
    std::deque<midi::Message> midiMessageQueue;

    dsp::ClockDivider lightDivider;
    float leftLevelClipTimer;
    float rightLevelClipTimer;


    std::string cardAOutput1NameString;
    std::string cardAOutput2NameString;
    std::string cardBOutput1NameString;
    std::string cardBOutput2NameString;
    std::string cardCOutput1NameString;
    std::string cardCOutput2NameString;
    std::string cardDOutput1NameString;
    std::string cardDOutput2NameString;
    std::string cardEOutput1NameString;
    std::string cardEOutput2NameString;
    std::string cardFOutput1NameString;
    std::string cardFOutput2NameString;


    PatchingMatrix() : audioPort(this),
                       cardAOutput1NameString(invalidCardOutputName),
                       cardAOutput2NameString(invalidCardOutputName),
                       cardBOutput1NameString(invalidCardOutputName),
                       cardBOutput2NameString(invalidCardOutputName),
                       cardCOutput1NameString(invalidCardOutputName),
                       cardCOutput2NameString(invalidCardOutputName),
                       cardDOutput1NameString(invalidCardOutputName),
                       cardDOutput2NameString(invalidCardOutputName),
                       cardEOutput1NameString(invalidCardOutputName),
                       cardEOutput2NameString(invalidCardOutputName),
                       cardFOutput1NameString(invalidCardOutputName),
                       cardFOutput2NameString(invalidCardOutputName) {
        setExpanderPrimary();

        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configSwitch(MIX_LEFT_SELECT_PARAM, 0.f, 1.f, 0.f, "Left Output", { "Out1", "Out2" });
        configSwitch(MIX_RIGHT_SELECT_PARAM, 0.f, 1.f, 0.f, "Right Output", { "Out1", "Out2" });
        configParam(LEFT_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Left Level", " V", 0.f, 10.f);
        configParam(RIGHT_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Right Level", " V", 0.f, 10.f);

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

        configLight(LEFT_EXPANDER_LIGHT, "Connection Status");
        configLight(RIGHT_EXPANDER_LIGHT, "Connection Status");

        onReset();

        lightDivider.setDivision(512);
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
        lights[CARD_A_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_A_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_A_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_B_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_B_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_B_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_C_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_C_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_C_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_D_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_D_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_D_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_E_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_E_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_E_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_F_MIX1_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX1_OUTPUT_BUTTON_PARAM].getValue() > 0.f);
        lights[CARD_F_MIX2_OUTPUT_BUTTON_LIGHT].setBrightness(params[CARD_F_MIX2_OUTPUT_BUTTON_PARAM].getValue() > 0.f);


        if (audioPort.deviceNumOutputs > 0) {
            dsp::Frame<maxChannels> inputFrame = {};
            float v;
            const float clipTime = 0.25f;

            // copy expander control voltages to Frame
            // this stuff ought to go in the parent class...
            if (leftExpander.module != NULL && validLeftExpander) {
                ZoxnoxiousControlMsg *leftExpanderConsumerMessage;
                leftExpanderConsumerMessage = static_cast<ZoxnoxiousControlMsg*>(leftExpander.module->rightExpander.consumerMessage);
                if (leftExpanderConsumerMessage) {

                    //for (int i = 0; i < audioPort.deviceNumOutputs; ++i) {  // TODO: USE THIS LINE NOT THE NEXT
                    for (int i = 0; i < maxChannels; ++i) { // DELETE THIS LINE
                        inputFrame.samples[i] = leftExpanderConsumerMessage->frame[i];
                    }
                }
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

            setLeftExpanderLight(LEFT_EXPANDER_LIGHT);
            setRightExpanderLight(RIGHT_EXPANDER_LIGHT);
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
        zCommand_a.channelAssignments[0] = { 0x02, 3, 2, false };
        zCommand_a.channelAssignments[1] = { 0x02, 9, 3, false };
        //zCommand_a.channelAssignments[2] = { 0x00, -1, -1, false };
        zCommand_a.channelAssignments[2] = { 0x02, 15, 4, false };
        zCommand_a.channelAssignments[3] = { 0x00, -1, -1, false };
        zCommand_a.channelAssignments[4] = { 0x00, -1, -1, false };
        zCommand_a.channelAssignments[5] = { 0x00, -1, -1, false };
        zCommand_a.channelAssignments[6] = { 0x00, -1, -1, false };
        zCommand_a.channelAssignments[7] = { getHardwareId(), 0, 1, false };

        // take ownership of our card
        processZoxnoxiousCommand(&zCommand_a);

        zCommand_b = zCommand_a;
        INFO("PatchingMatrix: set command msg to authoritative");
    }


    void onChannelAssignmentEstablished(ZoxnoxiousCommandMsg *zCommand) override {
        ZoxnoxiousModule::onChannelAssignmentEstablished(zCommand);

        cardAOutput1NameString = getCardOutputName(zCommand_a.channelAssignments[0].cardId, 1, 0);
        cardAOutput2NameString = getCardOutputName(zCommand_a.channelAssignments[0].cardId, 2, 0);

        cardBOutput1NameString = getCardOutputName(zCommand_a.channelAssignments[1].cardId, 1, 1);
        cardBOutput2NameString = getCardOutputName(zCommand_a.channelAssignments[1].cardId, 2, 1);

        cardCOutput1NameString = getCardOutputName(zCommand_a.channelAssignments[2].cardId, 1, 2);
        cardCOutput2NameString = getCardOutputName(zCommand_a.channelAssignments[2].cardId, 2, 2);

        cardDOutput1NameString = getCardOutputName(zCommand_a.channelAssignments[3].cardId, 1, 3);
        cardDOutput2NameString = getCardOutputName(zCommand_a.channelAssignments[3].cardId, 2, 3);

        cardEOutput1NameString = getCardOutputName(zCommand_a.channelAssignments[4].cardId, 1, 4);
        cardEOutput2NameString = getCardOutputName(zCommand_a.channelAssignments[4].cardId, 2, 4);

        cardFOutput1NameString = getCardOutputName(zCommand_a.channelAssignments[5].cardId, 1, 5);
        cardFOutput2NameString = getCardOutputName(zCommand_a.channelAssignments[5].cardId, 2, 6);

    }

    void onChannelAssignmentLost() override {
        ZoxnoxiousModule::onChannelAssignmentLost();
    }


};


struct PatchingMatrixWidget : ModuleWidget {
    PatchingMatrixWidget(PatchingMatrix* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/AudioOut.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(40.177, 38.587)), module, PatchingMatrix::CARD_A_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(51.729, 43.278)), module, PatchingMatrix::CARD_A_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_A_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(40.177, 51.648)), module, PatchingMatrix::CARD_B_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(51.729, 56.835)), module, PatchingMatrix::CARD_B_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_B_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(40.177, 64.71)), module, PatchingMatrix::CARD_C_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(51.729, 69.897)), module, PatchingMatrix::CARD_C_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_C_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(40.177, 77.771)), module, PatchingMatrix::CARD_D_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(51.729, 82.958)), module, PatchingMatrix::CARD_D_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_D_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(40.177, 90.833)), module, PatchingMatrix::CARD_E_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(51.729, 96.02)), module, PatchingMatrix::CARD_E_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_E_MIX2_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(40.177, 103.894)), module, PatchingMatrix::CARD_F_MIX1_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX1_OUTPUT_BUTTON_LIGHT));
        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(51.729, 109.081)), module, PatchingMatrix::CARD_F_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_OUTPUT_BUTTON_LIGHT));

//        ParamWidget *cardFMix2Output = createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(125.017, 109.081)), module, PatchingMatrix::CARD_F_MIX2_OUTPUT_BUTTON_PARAM, PatchingMatrix::CARD_F_MIX2_OUTPUT_BUTTON_LIGHT);
//        addChild(cardFMix2Output);
//        cardFMix2Output->setVisible(false);
        

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(74.312, 47.501)), module, PatchingMatrix::LEFT_LEVEL_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(74.312, 89.669)), module, PatchingMatrix::RIGHT_LEVEL_KNOB_PARAM));


        addParam(createParamCentered<CKSS>(mm2px(Vec(66.373, 17.478)), module, PatchingMatrix::MIX_LEFT_SELECT_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(66.373, 117.692)), module, PatchingMatrix::MIX_RIGHT_SELECT_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(74.312, 57.166)), module, PatchingMatrix::LEFT_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(74.312, 99.334)), module, PatchingMatrix::RIGHT_LEVEL_INPUT));


        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(74.312, 38.417)), module, PatchingMatrix::LEFT_LEVEL_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(74.312, 80.585)), module, PatchingMatrix::RIGHT_LEVEL_CLIP_LIGHT));


        // mm2px(Vec(27.171, 3.636))
        cardAOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 36.769)));
        cardAOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardAOutput1TextField->setText(module ? &module->cardAOutput1NameString : NULL);
        addChild(cardAOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardAOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 41.46)));
        cardAOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardAOutput2TextField->setText(module ? &module->cardAOutput2NameString : NULL);
        addChild(cardAOutput2TextField);

        // mm2px(Vec(27.171, 3.636))
        cardBOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 49.83)));
        cardBOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardBOutput1TextField->setText(module ? &module->cardBOutput1NameString : NULL);
        addChild(cardBOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardBOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 55.017)));
        cardBOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardBOutput2TextField->setText(module ? &module->cardBOutput2NameString : NULL);
        addChild(cardBOutput2TextField);

        // mm2px(Vec(27.171, 3.636))
        cardCOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 62.892)));
        cardCOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardCOutput1TextField->setText(module ? &module->cardCOutput1NameString : NULL);
        addChild(cardCOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardCOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 68.078)));
        cardCOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardCOutput2TextField->setText(module ? &module->cardCOutput2NameString : NULL);
        addChild(cardCOutput2TextField);


        // mm2px(Vec(27.171, 3.636))
        cardDOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 75.953)));
        cardDOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardDOutput1TextField->setText(module ? &module->cardDOutput1NameString : NULL);
        addChild(cardDOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardDOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 81.14)));
        cardDOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardDOutput2TextField->setText(module ? &module->cardDOutput2NameString : NULL);
        addChild(cardDOutput2TextField);

        // mm2px(Vec(27.171, 3.636))
        cardEOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 89.015)));
        cardEOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardEOutput1TextField->setText(module ? &module->cardEOutput1NameString : NULL);
        addChild(cardEOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardEOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 94.201)));
        cardEOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardEOutput2TextField->setText(module ? &module->cardEOutput2NameString : NULL);
        addChild(cardEOutput2TextField);

        // mm2px(Vec(27.171, 3.636))
        cardFOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 102.076)));
        cardFOutput1TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardFOutput1TextField->setText(module ? &module->cardFOutput1NameString : NULL);
        addChild(cardFOutput1TextField);

        // mm2px(Vec(27.171, 3.636))
        cardFOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 107.263)));
        cardFOutput2TextField->box.size = (mm2px(Vec(27.171, 3.636)));
        cardFOutput2TextField->setText(module ? &module->cardFOutput2NameString : NULL);
        addChild(cardFOutput2TextField);

        addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(2.02, 8.219)), module, PatchingMatrix::LEFT_EXPANDER_LIGHT));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(88.4, 8.219)), module, PatchingMatrix::RIGHT_EXPANDER_LIGHT));
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
