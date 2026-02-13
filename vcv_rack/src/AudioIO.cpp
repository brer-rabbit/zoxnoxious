#include "plugin.hpp"

#include "AudioIO.hpp"
#include "common.hpp"
#include "zcomponentlib.hpp"


namespace zox {

std::atomic<AudioIO*> AudioIO::instance { nullptr };
// TODO: THIS WILL NEED TO BE SET TO 100~200 ONCE DONE HERE
static constexpr int midiPollRateHz = 1;
static constexpr int lightRateHz = 60;

enum cvChannel {
    OUT2_CHANNEL = 0,
    OUT1_CHANNEL
};


AudioIO::AudioIO() : cardAOutput1NameString(invalidCardOutputName),
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
                     cardFOutput2NameString(invalidCardOutputName),
                     out1LevelClipTimer(0.f),
                     out2LevelClipTimer(0.f) {

  for(int i = 0; i < maxAudioDevices; ++i) {
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

  lightDivider.setDivision(APP->engine->getSampleRate() / lightRateHz);
  discoveryRequestClockDivider.setDivision(APP->engine->getSampleRate());  // once per second
  midiPollClockDivider.setDivision(static_cast<int>(APP->engine->getSampleRate()) / midiPollRateHz);

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


AudioIO::~AudioIO() {
  for (auto it = audioPorts.begin(); it != audioPorts.end(); ++it) {
    (*it)->setDriverId(-1);
    delete *it;
  }
}


void AudioIO::onAdd(const AddEvent &e) {
  Module::onAdd(e);
  AudioIO *expected = nullptr;
  AudioIO::instance.compare_exchange_strong(expected, this, std::memory_order_release);
}


void AudioIO::onRemove(const RemoveEvent &e) {
  AudioIO *expected = this;
  AudioIO::instance.compare_exchange_strong(expected, nullptr, std::memory_order_release);
  Module::onRemove(e);
}

void AudioIO::onReset() {
  for (auto it = audioPorts.begin(); it != audioPorts.end(); ++it) {
    (*it)->setDriverId(-1);
  }
  midiOutput.reset();
}


void AudioIO::onSampleRateChange(const SampleRateChangeEvent& e) {
  for (auto it = audioPorts.begin(); it != audioPorts.end(); ++it) {
    (*it)->engineInputBuffer.clear();
    (*it)->engineOutputBuffer.clear();
  }

  midiPollClockDivider.setDivision(static_cast<int>(e.sampleRate) / midiPollRateHz);
  INFO("midi polling set to %" PRIu32, midiPollClockDivider.getDivision());
  // discoveryRequestClockDivider: no need to change, it's acted on once only
  lightDivider.setDivision(static_cast<int>(e.sampleRate) / lightRateHz);
}


void AudioIO::process(const ProcessArgs& args) {
  dsp::Frame<maxAudioChannels> sharedFrames[ audioPorts.size() ];
  midi::Message midiOutMessage;
  bool isMidiClockTick = midiPollClockDivider.process();
  const Broker::Snapshot snap = broker.snapshot();

  /*
  if (isMidiClockTick) {
    for (size_t i = 0; i < maxVoiceCards; ++i) {
      const auto& s = snap.slots[i];
      INFO("slot %ld: p=%p allocated=%d hw=%d",
           i, (void*)s.participant,
           s.props.isAllocated,
           s.props.hardwareId);
    }
  }
  */


  // process all participants
  for (size_t i = 0; i < maxVoiceCards; ++i) {
    const Slot *slot = &snap.slots[i];

    if (slot->participant != nullptr && slot->props.isAllocated) {
      slot->participant->pullSamples(args,
                                     sharedFrames[slot->props.outputDeviceId],
                                     slot->props.cvChannelOffset);

      if (isMidiClockTick) {
        INFO("frame %ld: midi process on slot %ld module id %ld",
             args.frame,
             i, slot->participant->getModuleId());
        slot->participant->pullMidi(args,
                                    midiPollClockDivider.getDivision(),
                                    slot->props.midiChannel,
                                    midiOutMessage);
      }
    }
  }



  // check for incoming midi
  midi::Message midiInMsg;
  while (midiInput.tryPop(&midiInMsg, args.frame)) {
    processMidiInMessage(midiInMsg);
  }


  if (discoveryReportReceived == false && discoveryRequestClockDivider.process()) {
    INFO("Sending MIDI message discovery request");
    midiOutput.sendMidiMessage(MIDI_DISCOVERY_REQUEST_SYSEX);
  }


  if (lightDivider.process()) {
    // purely UI related state changes
    const float lightTime = args.sampleTime * lightDivider.getDivision();
    const float brightnessDeltaTime = 1 / lightTime;

    out1LevelClipTimer -= lightTime;
    lights[OUT1_LEVEL_CLIP_LIGHT].setBrightnessSmooth(out1LevelClipTimer > 0.f, brightnessDeltaTime);

    out2LevelClipTimer -= lightTime;
    lights[OUT2_LEVEL_CLIP_LIGHT].setBrightnessSmooth(out2LevelClipTimer > 0.f, brightnessDeltaTime);


    // only do midi stuff if we have an assigned channel
    if (false) { 
      // MIX1 and MIX2 buttons to midi programs
      //
      // check for any MIX1 buttons changing state and send midi
      // messages for them.  Toggle light if so.  Do this by
      // indexing the enums -- just don't re-order the enums

      for (int i = 0; i < 12; ++i) {
        int buttonParam = CARD_A_MIX1_OUTPUT_BUTTON_PARAM + i;
        int lightParam = CARD_A_MIX1_OUTPUT_BUTTON_LIGHT + i;

        if (params[buttonParam].getValue() != buttonParamToMidiProgramList[i].previousValue) {
          buttonParamToMidiProgramList[i].previousValue = params[buttonParam].getValue();

          int buttonParamValue = (params[buttonParam].getValue() > 0.f);
          lights[lightParam].setBrightness(buttonParamValue);
          int midiProgram = buttonParamToMidiProgramList[i].midiProgram[buttonParamValue];
          sendMidiProgramChangeMessage(midiProgram);
        }
      }
    }
  }
}
    


  /** getCardHardwareId
   * return the hardware Id of this card.
   * This must match the numeric identifier the Pi has in the etc/zoxnoxiousd.cfg file.
   * If it doesn't match ZoxnoxiousControlMsg will not have a channel assignment for this card.
   */
static const uint8_t hardwareId = 0x07;
uint8_t AudioIO::getHardwareId() { // TODO: should this be an interface?
  return hardwareId;
}


static const uint8_t midiProgramChangeStatus = 0xC;
static const uint8_t midiManufacturerId = 0x7d;
static const uint8_t midiSysexDiscoveryReport = 0x01;

/** sendMidiProgramChangeMessage
 *
 * send a program change message
 */
int AudioIO::sendMidiProgramChangeMessage(int programNumber) {
  midi::Message midiProgramMessage;
  midiProgramMessage.setSize(2);
  midiProgramMessage.setChannel(1);
  midiProgramMessage.setStatus(midiProgramChangeStatus);
  midiProgramMessage.setNote(programNumber);
  midiOutput.sendMidiMessage(midiProgramMessage);
  INFO("sending midi message for program %d channel %d",
       programNumber, 1);
  return 0;
}



void AudioIO::processMidiInMessage(const midi::Message &msg) {
  // the only thing being processed as of now is the Discovery Report
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
 * Yes, the report does specify 8 cards.  One is the backplane (this module),
 * the hardware guarantees only six are populated (hardware does not have a 7th slot)
 * but it's there in the report anyway.  Only six are cold-swappable voice cards.
 */

static constexpr int discoveryReportMessageSize = 28;
static constexpr int numReportsCards = 8;

// to be mapped to ParticipantProperty
struct DiscoveredCard {
    uint8_t hardwareId;
    uint8_t cvChannelOffset;
    uint8_t outputDeviceId;
    bool valid;
};


void AudioIO::processDiscoveryReport(const midi::Message &msg) {
  constexpr int byteOffset = 3; // actual data starts at this offset after sysex header
  int msgSize = msg.getSize(); // cache

  if (discoveryReportReceived == true) {
    INFO("ignoring duplicate Discovery Report");
    return;
  }

  if (msgSize != discoveryReportMessageSize) {
    WARN("Discovery report contains %d bytes, expected %d", msgSize, discoveryReportMessageSize);
  }

  DiscoveredCard cards[numReportsCards] = {};

  // first pass: just parse
  for (int i = 0; i < numReportsCards; ++i) {
    int base = byteOffset + (i * 3);
    uint8_t id = msg.bytes[base];
    uint8_t cv = msg.bytes[base + 1];
    uint8_t dev = msg.bytes[base + 2];

    // 0x00 and 0xFF are not valid hardware Ids
    if (id != 0x00 && id != 0xFF) {
      cards[i] = { id, cv, dev, true };
    }
    else {
      cards[i] = { 0, 0, 0, false };
    }

  }

  // second pass: apply bizrules
  applyDiscoveryReport(cards);
  discoveryReportReceived = true;
}


// note the midi channel is assigned here.  By convention with the zoxnoxiousd daemon
// the first in the report has channel 0, then 1, etc.
// "Discovered card number maps to MIDI channel number.  Not actual slot number."
void AudioIO::applyDiscoveryReport(DiscoveredCard *cards) {
  int assignedMidiChannel = 0;
  ParticipantProperty deviceTree[maxVoiceCards] = {};
  size_t participant_count = 0;

  for (int i = 0; i < numReportsCards; ++i) {
    if (!cards[i].valid) {
      continue; // ignore it
    }

    if (cards[i].hardwareId == getHardwareId()) {
      // Backplane
      cvChannelOffset = cards[i].cvChannelOffset;
      outputDeviceId = cards[i].outputDeviceId;
      midiChannel = assignedMidiChannel++;
    }
    else if (i < maxVoiceCards) {
      // Voice card
      ParticipantProperty& slot = deviceTree[i];
      participant_count++;
      
      slot.moduleId = -1;
      slot.hardwareId = cards[i].hardwareId;
      slot.cvChannelOffset = cards[i].cvChannelOffset;
      slot.outputDeviceId = cards[i].outputDeviceId;
      slot.midiChannel = assignedMidiChannel++;
      slot.isAllocated = false;
      INFO("registered hardware id %d midi channel %d", slot.hardwareId, slot.midiChannel);
    }
    else {
      WARN("too many voice cards");
    }

  }

  broker.registerDevices(deviceTree, participant_count);
}


json_t* AudioIO::dataToJson() {
  json_t* rootJ = json_object();
  json_object_set_new(rootJ, "midiInput", midiInput.toJson());
  json_object_set_new(rootJ, "midiOutput", midiOutput.toJson());
  for (std::size_t deviceNum = 0; deviceNum < audioPorts.size(); ++deviceNum) {
    std::string thisAudioPortNum = audioPortNum + std::to_string(deviceNum);
    json_object_set_new(rootJ, thisAudioPortNum.c_str(), audioPorts[deviceNum]->toJson());
  }
  return rootJ;
}

void AudioIO::dataFromJson(json_t* rootJ) {
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



bool AudioIO::isPrimary() const {
    return AudioIO::instance.load(std::memory_order_relaxed) == this;
}



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

} // namespace zox

Model* modelAudioIO = createModel<zox::AudioIO, zox::AudioIOWidget>("AudioIO");

