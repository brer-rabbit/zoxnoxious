#include "plugin.hpp"

#include "AudioMidi.hpp"
#include "zcomponentlib.hpp"
#include "ZoxnoxiousExpander.hpp"


enum cvChannel {
    OUT2_CHANNEL = 0,
    OUT1_CHANNEL
};

struct AudioIO : ZoxnoxiousModule {
    enum ParamId {
        OUT1_LEVEL_KNOB_PARAM,
        OUT2_LEVEL_KNOB_PARAM,
        CARD_A_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_B_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_C_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_D_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_E_MIX1_OUTPUT_BUTTON_PARAM,
        CARD_F_MIX1_OUTPUT_BUTTON_PARAM,
        // the MIX2 enums need to be ordered:
        CARD_A_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_B_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_C_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_D_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_E_MIX2_OUTPUT_BUTTON_PARAM,
        CARD_F_MIX2_OUTPUT_BUTTON_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        OUT1_LEVEL_INPUT,
        OUT2_LEVEL_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUTS_LEN
    };
    enum LightId {
        OUT1_LEVEL_CLIP_LIGHT,
        OUT2_LEVEL_CLIP_LIGHT,
        CARD_A_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_B_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_C_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_D_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_E_MIX1_OUTPUT_BUTTON_LIGHT,
        CARD_F_MIX1_OUTPUT_BUTTON_LIGHT,
        // the MIX2 enums need to be ordered:
        CARD_A_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_B_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_C_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_D_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_E_MIX2_OUTPUT_BUTTON_LIGHT,
        CARD_F_MIX2_OUTPUT_BUTTON_LIGHT,
        ENUMS(LEFT_EXPANDER_LIGHT, 3),
        ENUMS(RIGHT_EXPANDER_LIGHT, 3),
        LIGHTS_LEN
    };

    std::vector<ZoxnoxiousAudioPort*> audioPorts;
    ZoxnoxiousMidiOutput midiOutput;
    midi::InputQueue midiInput;


    dsp::ClockDivider lightDivider;
    float out1LevelClipTimer;
    float out2LevelClipTimer;

    bool mix2ButtonsPreviousState[6];

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


    struct buttonParamMidiProgram {
        enum ParamId button;
        int previousValue;
        uint8_t midiProgram[2];
    } buttonParamToMidiProgramList[12] =
      {
          // the ordering here is important- taking a dependency on
          // ordering when producing midi messages
          { CARD_A_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 0, 1 } },
          { CARD_B_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 2, 3 } },
          { CARD_C_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 4, 5 } },
          { CARD_D_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 6, 7 } },
          { CARD_E_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 8, 9 } },
          { CARD_F_MIX1_OUTPUT_BUTTON_PARAM, INT_MIN, { 10, 11 } },
          { CARD_A_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 12, 13 } },
          { CARD_B_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 14, 15 } },
          { CARD_C_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 16, 17 } },
          { CARD_D_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 18, 19 } },
          { CARD_E_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 20, 21 } },
          { CARD_F_MIX2_OUTPUT_BUTTON_PARAM, INT_MIN, { 22, 23 } }
      };

    // Discovery related variables
    midi::Message MIDI_DISCOVERY_REQUEST_SYSEX; // this should be const static
    midi::Message MIDI_SHUTDOWN_SYSEX;
    midi::Message MIDI_RESTART_SYSEX;
    midi::Message MIDI_TUNE_REQUEST;

    bool receivedPluginList = false;
    dsp::ClockDivider discoveryRequestClockDivider;


    AudioIO() : cardAOutput1NameString(invalidCardOutputName),
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
        AudioIO::initCommandMsgState();

        for(int i = 0; i < maxDevices; ++i) {
            audioPorts.push_back(new ZoxnoxiousAudioPort(this));
        }


        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(OUT1_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Out1 Level", " V", 0.f, 10.f);
        configParam(OUT2_LEVEL_KNOB_PARAM, 0.f, 1.f, 0.5f, "Out2 Level", " V", 0.f, 10.f);

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

        configInput(OUT1_LEVEL_INPUT, "Out1 VCA Level");
        configInput(OUT2_LEVEL_INPUT, "Out2 VCA Level");

        configLight(LEFT_EXPANDER_LIGHT, "Connection Status");
        configLight(RIGHT_EXPANDER_LIGHT, "Connection Status");

        onReset();

        lightDivider.setDivision(512);
        discoveryRequestClockDivider.setDivision(APP->engine->getSampleRate());  // once per second
        // see Zoxnoxious MIDI spec for details
        MIDI_DISCOVERY_REQUEST_SYSEX.setSize(4);
        MIDI_DISCOVERY_REQUEST_SYSEX.bytes[0] = 0xF0;
        MIDI_DISCOVERY_REQUEST_SYSEX.bytes[1] = 0x7D;
        MIDI_DISCOVERY_REQUEST_SYSEX.bytes[2] = 0x02;
        MIDI_DISCOVERY_REQUEST_SYSEX.bytes[3] = 0xF7;

        MIDI_SHUTDOWN_SYSEX.setSize(4);
        MIDI_SHUTDOWN_SYSEX.bytes[0] = 0xF0;
        MIDI_SHUTDOWN_SYSEX.bytes[1] = 0x7D;
        MIDI_SHUTDOWN_SYSEX.bytes[2] = 0x03;
        MIDI_SHUTDOWN_SYSEX.bytes[3] = 0xF7;

        MIDI_RESTART_SYSEX.setSize(4);
        MIDI_RESTART_SYSEX.bytes[0] = 0xF0;
        MIDI_RESTART_SYSEX.bytes[1] = 0x7D;
        MIDI_RESTART_SYSEX.bytes[2] = 0x04;
        MIDI_RESTART_SYSEX.bytes[3] = 0xF7;

        MIDI_TUNE_REQUEST.setSize(1);
        MIDI_TUNE_REQUEST.bytes[0] = 0xF6;
    }


    ~AudioIO() {
        for (auto it = audioPorts.begin(); it != audioPorts.end(); ++it) {
            (*it)->setDriverId(-1);
            delete *it;
        }
    }

    void onReset() override {
        for (auto it = audioPorts.begin(); it != audioPorts.end(); ++it) {
            (*it)->setDriverId(-1);
        }
        midiOutput.reset();
    }


    void onSampleRateChange(const SampleRateChangeEvent& e) override {
        for (auto it = audioPorts.begin(); it != audioPorts.end(); ++it) {
            (*it)->engineInputBuffer.clear();
            (*it)->engineOutputBuffer.clear();
        }
    }


    void process(const ProcessArgs& args) override {
        processExpander(args);

        midi::Message msg;
        while (midiInput.tryPop(&msg, args.frame)) {
            processMidiMessage(msg);
        }

        if (receivedPluginList == false && discoveryRequestClockDivider.process()) {
            INFO("Sending MIDI message discovery request");
            midiOutput.sendMidiMessage(MIDI_DISCOVERY_REQUEST_SYSEX);
        }


        if (lightDivider.process()) {
            // slower moving stuff here
            // LEDs: clipping and expander connections
            const float lightTime = args.sampleTime * lightDivider.getDivision();
            const float brightnessDeltaTime = 1 / lightTime;

            out1LevelClipTimer -= lightTime;
            lights[OUT1_LEVEL_CLIP_LIGHT].setBrightnessSmooth(out1LevelClipTimer > 0.f, brightnessDeltaTime);

            out2LevelClipTimer -= lightTime;
            lights[OUT2_LEVEL_CLIP_LIGHT].setBrightnessSmooth(out2LevelClipTimer > 0.f, brightnessDeltaTime);

            setLeftExpanderLight(LEFT_EXPANDER_LIGHT);
            setRightExpanderLight(RIGHT_EXPANDER_LIGHT);

            // only do midi stuff if we have an assigned channel
            if (hasChannelAssignment) { 
                // MIX1 buttons to midi programs
                //
                // check for any MIX1 buttons changing state and send midi
                // messages for them.  Toggle light if so.  Do this by
                // indexing the enums -- just don't re-order the enums

                for (int i = 0; i < 6; ++i) {
                    int buttonParam = CARD_A_MIX1_OUTPUT_BUTTON_PARAM + i;
                    int lightParam = CARD_A_MIX1_OUTPUT_BUTTON_LIGHT + i;

                    if (params[buttonParam].getValue() !=
                        buttonParamToMidiProgramList[i].previousValue) {

                        buttonParamToMidiProgramList[i].previousValue =
                            params[buttonParam].getValue();

                        int buttonParamValue = (params[buttonParam].getValue() > 0.f);
                        lights[lightParam].setBrightness(buttonParamValue);
                        int midiProgram = buttonParamToMidiProgramList[i].midiProgram[buttonParamValue];
                        sendMidiProgramChangeMessage(midiProgram);
                    }
                }

                // MIX2 buttons to midi programs
                // this is exactly like MIX1, just different indices
                for (int i = 0; i < 6; ++i) {
                    int buttonParam = CARD_A_MIX2_OUTPUT_BUTTON_PARAM + i;
                    int lightParam = CARD_A_MIX2_OUTPUT_BUTTON_LIGHT + i;

                    if (params[buttonParam].getValue() !=
                        buttonParamToMidiProgramList[i].previousValue) {

                        buttonParamToMidiProgramList[i].previousValue =
                            params[buttonParam].getValue();

                        int buttonParamValue = (params[buttonParam].getValue() > 0.f);
                        lights[lightParam].setBrightness(buttonParamValue);
                        int midiProgram = buttonParamToMidiProgramList[i].midiProgram[buttonParamValue];
                        sendMidiProgramChangeMessage(midiProgram);
                    }
                }

            }
        }
    }



    

    /** processZoxnoxiousControl
     *
     * intake a control message which contains CV data and possibly a
     * MIDI message.  MIDI message sending is done here since this is
     * where we're processing the control messages.
     */
    void processZoxnoxiousControl(ZoxnoxiousControlMsg *controlMsg) override {
        // this ought to be the case -- consider making this an assertion
        if (!hasChannelAssignment) {
            if (APP->engine->getFrame() % 60000 == 0) {
                INFO("zoxnoxious: module id %" PRId64 " no channel assignment", getId());
            }
            return;
        }

        processControlChannels(controlMsg);

        // Send a MIDI message if present in the control message
        if (controlMsg->midiMessageSet) {
            INFO("AudioOut: sending midi message from control msg");
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
        // do not re-init if we already know the plugin list.
        // This implies we do not expect the list to change once it is set.
        if (receivedPluginList) {
            return;
        }

        zCommand_a.authoritativeSource = true;
        // channelAssignment data:
        // hardware cardId, channelOffset (from zero), midiChannel, assignmentOwned
        zCommand_a.channelAssignments[0] = { 0x00, invalidOutputDeviceId, invalidCvChannelOffset, invalidMidiChannel, false };
        zCommand_a.channelAssignments[1] = { 0x00, invalidOutputDeviceId, invalidCvChannelOffset, invalidMidiChannel, false };
        zCommand_a.channelAssignments[2] = { 0x00, invalidOutputDeviceId, invalidCvChannelOffset, invalidMidiChannel, false };
        zCommand_a.channelAssignments[3] = { 0x00, invalidOutputDeviceId, invalidCvChannelOffset, invalidMidiChannel, false };
        zCommand_a.channelAssignments[4] = { 0x00, invalidOutputDeviceId, invalidCvChannelOffset, invalidMidiChannel, false };
        zCommand_a.channelAssignments[5] = { 0x00, invalidOutputDeviceId, invalidCvChannelOffset, invalidMidiChannel, false };
        zCommand_a.channelAssignments[6] = { 0x00, invalidOutputDeviceId, invalidCvChannelOffset, invalidMidiChannel, false };
        zCommand_a.channelAssignments[7] = { 0x00, invalidOutputDeviceId, invalidCvChannelOffset, invalidMidiChannel, false };

        // to hardcode assignments:
        //zCommand_a.channelAssignments[0] = { 0x02, 0, 0, false };
        //zCommand_a.channelAssignments[5] = { 0x02, 6, 1, false };
        //zCommand_a.channelAssignments[7] = { getHardwareId(), 12, 2, false };

        // take ownership of our card
        processZoxnoxiousCommand(&zCommand_a);

        zCommand_b = zCommand_a;
        INFO("AudioIO: set command msg to authoritative; channel assignment: %d", hasChannelAssignment);
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
        cardFOutput2NameString = getCardOutputName(zCommand_a.channelAssignments[5].cardId, 2, 5);

    }

    void onChannelAssignmentLost() override {
        ZoxnoxiousModule::onChannelAssignmentLost();
    }


private:

    /** processControlChannels
     *
     * pull the CV channels out of the control message and send
     * them to the audio out
     */
    void processControlChannels(ZoxnoxiousControlMsg *controlMsg) {

        for (std::size_t deviceNum = 0; deviceNum < audioPorts.size(); ++deviceNum) {

            if (audioPorts[deviceNum]->deviceNumOutputs > 0) {
                float v;
                const float clipTime = 0.25f;

                // add in local control voltages if deviceNum is our device
                if (deviceNum == static_cast<std::size_t>(outputDeviceId)) {
                    // out1 level
                    v = params[OUT1_LEVEL_KNOB_PARAM].getValue() + inputs[OUT1_LEVEL_INPUT].getVoltageSum() / 10.f;
                    controlMsg->frame[deviceNum].samples[cvChannelOffset + OUT1_CHANNEL] = clamp(v, 0.f, 1.f);
                    if (controlMsg->frame[deviceNum].samples[cvChannelOffset + OUT1_CHANNEL] != v) {
                        out1LevelClipTimer = clipTime;
                    }

                    // out2 level
                    v = params[OUT2_LEVEL_KNOB_PARAM].getValue() + inputs[OUT2_LEVEL_INPUT].getVoltageSum() / 10.f;
                     controlMsg->frame[deviceNum].samples[cvChannelOffset + OUT2_CHANNEL] = clamp(v, 0.f, 1.f);
                    if (controlMsg->frame[deviceNum].samples[cvChannelOffset + OUT2_CHANNEL] != v) {
                        out2LevelClipTimer = clipTime;
                    }
                }
                
                if (!audioPorts[deviceNum]->engineInputBuffer.full()) {
                    audioPorts[deviceNum]->engineInputBuffer.push(controlMsg->frame[deviceNum]);
                }
            }
        } // for deviceNum
    }


    /** sendMidiProgramChangeMessage
     *
     * send a program change message
     */
    int sendMidiProgramChangeMessage(int programNumber) {
        midi::Message midiProgramMessage;
        midiProgramMessage.setSize(2);
        midiProgramMessage.setChannel(midiChannel);
        midiProgramMessage.setStatus(midiProgramChangeStatus);
        midiProgramMessage.setNote(programNumber);
        midiOutput.sendMidiMessage(midiProgramMessage);
        INFO("sending midi message for program %d channel %d",
             programNumber, midiChannel);
        return 0;
    }


    const uint8_t midiManufacturerId = 0x7d;
    const uint8_t midiSysexDiscoveryReport = 0x01;

    void processMidiMessage(const midi::Message &msg) {
        // sysex test
        INFO("processing MIDI message");
        if (msg.getStatus() == 0xf && msg.getSize() > 3 && msg.bytes[1] == midiManufacturerId) {
            if (msg.bytes[2] == midiSysexDiscoveryReport) {
                processDiscoveryReport(msg);
            }
            else {
                INFO("sysex: unknown");
            }
        }
    }


    /** processDiscoveryReport
     *
     * read the report on which cards are present in the system.  The
     * MIDI sysex format for this is 28 bytes:
     * 0xF0
     * 0x7D
     * 0x01 -- discovery report
     * 0x?? -- cardA id
     * 0x?? -- cardA channel offset
     * 0x?? -- cardA device id
     * 0x?? -- cardB id
     * 0x?? -- cardB channel offset
     * 0x?? -- cardB device id
     * 0x?? -- cardC id
     * 0x?? -- cardC channel offset
     * 0x?? -- cardC device id
     * 0x?? -- cardD id
     * 0x?? -- cardD channel offset
     * 0x?? -- cardD device id
     * 0x?? -- cardE id
     * 0x?? -- cardE channel offset
     * 0x?? -- cardE device id
     * 0x?? -- cardF id
     * 0x?? -- cardF channel offset
     * 0x?? -- cardF device id
     * 0x?? -- cardG id
     * 0x?? -- cardG channel offset
     * 0x?? -- cardG device id
     * 0x?? -- cardH id
     * 0x?? -- cardH channel offset
     * 0x?? -- cardH device id
     * 0xF7
     * if the card Id isn't 0x00 or 0xFF then process it.
     * This report specifies exactly what cards are present in the
     * system and how the host (VCV Rack) is to communicate with each card.
     */
    static const int discoveryReportMessageSize = 28;
    void processDiscoveryReport(const midi::Message &msg) {
        const int bytesOffset = 3; // actual data starts at this offset
        int assignedMidiChannel = 0;
        // which message to update?
        ZoxnoxiousCommandMsg *leftExpanderProducerMessage =
            leftExpander.producerMessage == &zCommand_a ? &zCommand_a : &zCommand_b;

        if (msg.getSize() != discoveryReportMessageSize) {
            WARN("Discovery report contains %d bytes, expected %d", msg.getSize(), discoveryReportMessageSize);
        }

        for (int i = 0; i < maxCards; ++i) {
            if (msg.bytes[i * 3 + bytesOffset] != 0 && msg.bytes[i * 3 + bytesOffset] != 0xFF) {
                leftExpanderProducerMessage->channelAssignments[i].cardId = msg.bytes[i * 3 + bytesOffset];
                leftExpanderProducerMessage->channelAssignments[i].cvChannelOffset = msg.bytes[i * 3 + bytesOffset + 1];
                leftExpanderProducerMessage->channelAssignments[i].outputDeviceId = msg.bytes[i * 3 + bytesOffset + 2];
                leftExpanderProducerMessage->channelAssignments[i].midiChannel = assignedMidiChannel++;
                leftExpanderProducerMessage->channelAssignments[i].assignmentOwned = false;
                INFO("Discovery Report: card 0x%X device %d offset %d midi %d",
                     leftExpanderProducerMessage->channelAssignments[i].cardId,
                     leftExpanderProducerMessage->channelAssignments[i].outputDeviceId,
                     leftExpanderProducerMessage->channelAssignments[i].cvChannelOffset,
                     leftExpanderProducerMessage->channelAssignments[i].midiChannel);
            }
            else {
                leftExpanderProducerMessage->channelAssignments[i].cardId = invalidCardId;
                leftExpanderProducerMessage->channelAssignments[i].outputDeviceId = invalidOutputDeviceId;
                leftExpanderProducerMessage->channelAssignments[i].cvChannelOffset = invalidCvChannelOffset;
                leftExpanderProducerMessage->channelAssignments[i].midiChannel = invalidMidiChannel;
                leftExpanderProducerMessage->channelAssignments[i].assignmentOwned = false;
                INFO("Discovery Report: No card %d", i);
            }
        }

        // note to self that with a report received, we need not keep requesting it.
        // Process the results and pass it along to any expanded modules (flip request).
        // Since the report is basically static (no hotplugging modules) we shouldn't need
        // to produce a new one or flip the left message again.
        receivedPluginList = true;
        processZoxnoxiousCommand(leftExpanderProducerMessage);
        leftExpander.messageFlipRequested = true;
    }



    static const std::string audioPortNum;

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "midiInput", midiInput.toJson());
        json_object_set_new(rootJ, "midiOutput", midiOutput.toJson());
        for (std::size_t deviceNum = 0; deviceNum < audioPorts.size(); ++deviceNum) {
            std::string thisAudioPortNum = audioPortNum + std::to_string(deviceNum);
            json_object_set_new(rootJ, thisAudioPortNum.c_str(), audioPorts[deviceNum]->toJson());
        }
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* midiInputJ = json_object_get(rootJ, "midiInput");
        if (midiInputJ) {
            midiInput.fromJson(midiInputJ);
        }

        json_t* midiOutputJ = json_object_get(rootJ, "midiOutput");
        if (midiOutputJ) {
            midiOutput.fromJson(midiOutputJ);
        }

        for (std::size_t deviceNum = 0; deviceNum < audioPorts.size(); ++deviceNum) {
            std::string thisAudioPortNum = audioPortNum + std::to_string(deviceNum);
            json_t* audioPortJ = json_object_get(rootJ, thisAudioPortNum.c_str());
            if (audioPortJ) {
                audioPorts[deviceNum]->fromJson(audioPortJ);
            }
        }
    }

};


struct AudioIOWidget : ModuleWidget {
    AudioIOWidget(AudioIO* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/AudioIO.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        //addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.794, 18.162)), module, AudioIO::CARD_A_MIX1_OUTPUT_BUTTON_PARAM, AudioIO::CARD_A_MIX1_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.347, 22.854)), module, AudioIO::CARD_A_MIX2_OUTPUT_BUTTON_PARAM, AudioIO::CARD_A_MIX2_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.794, 31.224)), module, AudioIO::CARD_B_MIX1_OUTPUT_BUTTON_PARAM, AudioIO::CARD_B_MIX1_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.347, 36.41)), module, AudioIO::CARD_B_MIX2_OUTPUT_BUTTON_PARAM, AudioIO::CARD_B_MIX2_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.794, 44.285)), module, AudioIO::CARD_C_MIX1_OUTPUT_BUTTON_PARAM, AudioIO::CARD_C_MIX1_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.347, 49.472)), module, AudioIO::CARD_C_MIX2_OUTPUT_BUTTON_PARAM, AudioIO::CARD_C_MIX2_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.794, 57.347)), module, AudioIO::CARD_D_MIX1_OUTPUT_BUTTON_PARAM, AudioIO::CARD_D_MIX1_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.347, 62.533)), module, AudioIO::CARD_D_MIX2_OUTPUT_BUTTON_PARAM, AudioIO::CARD_D_MIX2_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.794, 70.408)), module, AudioIO::CARD_E_MIX1_OUTPUT_BUTTON_PARAM, AudioIO::CARD_E_MIX1_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.347, 75.595)), module, AudioIO::CARD_E_MIX2_OUTPUT_BUTTON_PARAM, AudioIO::CARD_E_MIX2_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(36.794, 83.47)), module, AudioIO::CARD_F_MIX1_OUTPUT_BUTTON_PARAM, AudioIO::CARD_F_MIX1_OUTPUT_BUTTON_LIGHT));

        addParam(createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(48.347, 88.656)), module, AudioIO::CARD_F_MIX2_OUTPUT_BUTTON_PARAM, AudioIO::CARD_F_MIX2_OUTPUT_BUTTON_LIGHT));

//        ParamWidget *cardFMix2Output = createLightParamCentered<ZLightLatch<SmallSimpleLight<RedLight>>>(mm2px(Vec(44.440456, 107.0808)), module, AudioIO::CARD_F_MIX2_OUTPUT_BUTTON_PARAM, AudioIO::CARD_F_MIX2_OUTPUT_BUTTON_LIGHT);
//        addChild(cardFMix2Output);
//        cardFMix2Output->setVisible(false);
        

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(11.275, 104.378)), module, AudioIO::OUT1_LEVEL_KNOB_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(11.275, 117.427)), module, AudioIO::OUT2_LEVEL_KNOB_PARAM));


        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(26.122, 104.378)), module, AudioIO::OUT1_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(26.122, 117.427)), module, AudioIO::OUT2_LEVEL_INPUT));


        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(19.361, 101.184)), module, AudioIO::OUT1_LEVEL_CLIP_LIGHT));
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(19.361, 114.233)), module, AudioIO::OUT2_LEVEL_CLIP_LIGHT));


        // mm2px(Vec(22.0, 3.636))
        cardAOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 16.344)));
        cardAOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardAOutput1TextField->setText(module ? &module->cardAOutput1NameString : NULL);
        addChild(cardAOutput1TextField);

        // mm2px(Vec(22.0, 3.636))
        cardAOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 21.035)));
        cardAOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardAOutput2TextField->setText(module ? &module->cardAOutput2NameString : NULL);
        addChild(cardAOutput2TextField);

        // mm2px(Vec(22.0, 3.636))
        cardBOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 29.406)));
        cardBOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardBOutput1TextField->setText(module ? &module->cardBOutput1NameString : NULL);
        addChild(cardBOutput1TextField);

        // mm2px(Vec(22.0, 3.636))
        cardBOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 34.592)));
        cardBOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardBOutput2TextField->setText(module ? &module->cardBOutput2NameString : NULL);
        addChild(cardBOutput2TextField);

        // mm2px(Vec(22.0, 3.636))
        cardCOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 42.467)));
        cardCOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardCOutput1TextField->setText(module ? &module->cardCOutput1NameString : NULL);
        addChild(cardCOutput1TextField);

        // mm2px(Vec(22.0, 3.636))
        cardCOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 47.654)));
        cardCOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardCOutput2TextField->setText(module ? &module->cardCOutput2NameString : NULL);
        addChild(cardCOutput2TextField);


        // mm2px(Vec(22.0, 3.636))
        cardDOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 55.529)));
        cardDOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardDOutput1TextField->setText(module ? &module->cardDOutput1NameString : NULL);
        addChild(cardDOutput1TextField);

        // mm2px(Vec(22.0, 3.636))
        cardDOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 60.715)));
        cardDOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardDOutput2TextField->setText(module ? &module->cardDOutput2NameString : NULL);
        addChild(cardDOutput2TextField);

        // mm2px(Vec(22.0, 3.636))
        cardEOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 68.59)));
        cardEOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardEOutput1TextField->setText(module ? &module->cardEOutput1NameString : NULL);
        addChild(cardEOutput1TextField);

        // mm2px(Vec(22.0, 3.636))
        cardEOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 73.777)));
        cardEOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardEOutput2TextField->setText(module ? &module->cardEOutput2NameString : NULL);
        addChild(cardEOutput2TextField);

        // mm2px(Vec(22.0, 3.636))
        cardFOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 81.652)));
        cardFOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardFOutput1TextField->setText(module ? &module->cardFOutput1NameString : NULL);
        addChild(cardFOutput1TextField);

        // mm2px(Vec(22.0, 3.636))
        cardFOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 86.838)));
        cardFOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
        cardFOutput2TextField->setText(module ? &module->cardFOutput2NameString : NULL);
        addChild(cardFOutput2TextField);

        addChild(createLightCentered<TriangleLeftLight<SmallLight<RedGreenBlueLight>>>(mm2px(Vec(2.0200968, 8.21875)), module, AudioIO::LEFT_EXPANDER_LIGHT));
        addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(59.246, 8.219)), module, AudioIO::RIGHT_EXPANDER_LIGHT));
    }


    void appendContextMenu(Menu *menu) override {
        AudioIO *module = dynamic_cast<AudioIO*>(this->module);

        menu->addChild(new MenuSeparator);
        menu->addChild(createSubmenuItem("MIDI Out Device", "",
                                         [=](Menu* menu) {
                                             appendMidiMenu(menu, &module->midiOutput);
                                         }));
        menu->addChild(new MenuSeparator);
        menu->addChild(createSubmenuItem("MIDI In Device", "",
                                         [=](Menu* menu) {
                                             appendMidiMenu(menu, &module->midiInput);
                                         }));
        menu->addChild(new MenuSeparator);
        menu->addChild(createSubmenuItem("Audio Device 0", "",
                                         [=](Menu* menu) {
                                             appendAudioMenu(menu, module->audioPorts[0]);
                                         }));
        if (module->audioPorts.size() > 0) {
            menu->addChild(createSubmenuItem("Audio Device 1", "",
                                             [=](Menu* menu) {
                                                 appendAudioMenu(menu, module->audioPorts[1]);
                                             }));
        }

        menu->addChild(new MenuSeparator);

        menu->addChild(createMenuItem("Autotune", "", [=]() {
              module->midiOutput.sendMidiMessage(module->MIDI_TUNE_REQUEST);
            }));

        menu->addChild(new MenuSeparator);

        menu->addChild(createMenuItem("Restart", "", [=]() {
              module->midiOutput.sendMidiMessage(module->MIDI_RESTART_SYSEX);
            }));
        menu->addChild(createMenuItem("Shutdown", "", [=]() {
              module->midiOutput.sendMidiMessage(module->MIDI_SHUTDOWN_SYSEX);
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


const std::string AudioIO::audioPortNum = "audioPort";
Model* modelAudioIO = createModel<AudioIO, AudioIOWidget>("AudioIO");
