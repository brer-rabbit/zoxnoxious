#include "plugin.hpp"

#include "AudioIO.hpp"
#include "common.hpp"
#include "zcomponentlib.hpp"

namespace zox {

std::atomic<AudioIO*> AudioIO::instance { nullptr };

// TODO: THIS WILL NEED TO BE SET TO 100~200 ONCE DONE HERE
static constexpr int midiPollRateHz = 10;

enum cvChannel {
    OUT2_CHANNEL = 0,
    OUT1_CHANNEL
};


AudioIO::AudioIO() : out1LevelClipTimer(0.f),
                     out2LevelClipTimer(0.f),
                     buttonStates(buttonMappings.size()),
                     buttonMidiController(buttonMappings),
                     routes{{
                         {OUT1_LEVEL_KNOB_PARAM, OUT1_LEVEL_INPUT, OUT1_CHANNEL, &out1LevelClipTimer, nullptr},
                         {OUT2_LEVEL_KNOB_PARAM, OUT2_LEVEL_INPUT, OUT2_CHANNEL, &out2LevelClipTimer, nullptr}
  }}
{

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

  orchestrationClockDivider.setDivision(APP->engine->getSampleRate());  // once per second
  midiPollClockDivider.setDivision(static_cast<int>(APP->engine->getSampleRate()) / midiPollRateHz);


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
  orchestrationClockDivider.setDivision(static_cast<int>(e.sampleRate));
}


void AudioIO::process(const ProcessArgs& args) {
  dsp::Frame<maxAudioChannels> sharedFrames[ audioPorts.size() ];
  bool isMidiClockTick = midiPollClockDivider.process();
  bool isOrchestrationClockTick = orchestrationClockDivider.process();

  const Broker::Snapshot snap = broker.snapshot();

  // check for incoming midi
  midi::Message midiInMsg;
  while (midiInput.tryPop(&midiInMsg, args.frame)) {
    processMidiInMessage(midiInMsg);
  }

  // DEBUG REMOVE THIS
  if (APP->engine->getFrame() == 80000) {
    midi::Message discoReport;
    discoReport.setSize(28);
    discoReport.bytes[0] = 0xF0;
    discoReport.bytes[1] = 0x7D;
    discoReport.bytes[2] = 0x01;
    discoReport.bytes[3] = 0x02;
    discoReport.bytes[4] = 0x00;
    discoReport.bytes[5] = 0x00;
    discoReport.bytes[6] = 0x02;
    discoReport.bytes[7] = 0x00;
    discoReport.bytes[8] = 0x00;
    discoReport.bytes[9] = 0x07;
    discoReport.bytes[10] = 0x00;
    discoReport.bytes[11] = 0x00;

    processDiscoveryReport(discoReport);
  }

  if (discoveryReportReceived == false) {
    if (isOrchestrationClockTick) {
      INFO("Sending MIDI message discovery request");
      midiOutput.sendMidiMessage(MIDI_DISCOVERY_REQUEST_SYSEX);
    }
    return;
  }

  // walk the modules to see if anything should be attached
  if (isOrchestrationClockTick) {
    scanParticipants();

  }

  // process all participants for samples
  for (size_t i = 0; i < maxVoiceCards; ++i) {
    const Slot *slot = &snap.slots[i];
    if (slot->participant != nullptr && slot->props.isAllocated) {
      slot->participant->pullSamples(args,
                                     sharedFrames[slot->props.outputDeviceId],
                                     slot->props.cvChannelOffset);
    }
  }


  processSelfSamples(sharedFrames[outputDeviceId]);
  sendFramesToDevices(sharedFrames, audioPorts.size());

  if (isMidiClockTick) {
    setStatusLight();

    // process all participants for MIDI
    midi::Message midiOutMessage;
    for (size_t i = 0; i < maxVoiceCards; ++i) {
      const Slot *slot = &snap.slots[i];
      if (slot->participant != nullptr && slot->props.isAllocated) {
        if (slot->participant->pullMidi(args,
                                        midiPollClockDivider.getDivision(),
                                        slot->props.midiChannel,
                                        midiOutMessage)) {
          INFO("frame %" PRId64 ": midi: slot %ld module id %" PRId64 " send message %02X",
               args.frame, i, slot->participant->getModuleId(), midiOutMessage.getNote());
          midiOutput.sendMidiMessage(midiOutMessage);
        }
      }
    }

    if (buttonMidiController.process(this, midiChannel, midiOutMessage)) {
      midiOutput.sendMidiMessage(midiOutMessage);
    }
    buttonMidiController.updateLights(this);

    const float lightTime = args.sampleTime * midiPollClockDivider.getDivision();
    const float brightnessDeltaTime = 1 / lightTime;

    out1LevelClipTimer -= lightTime;
    lights[OUT1_LEVEL_CLIP_LIGHT].setBrightnessSmooth(out1LevelClipTimer > 0.f, brightnessDeltaTime);
    out2LevelClipTimer -= lightTime;
    lights[OUT2_LEVEL_CLIP_LIGHT].setBrightnessSmooth(out2LevelClipTimer > 0.f, brightnessDeltaTime);
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


void AudioIO::setStatusLight() {
  if (discoveryReportReceived) {  // green
    lights[RIGHT_EXPANDER_LIGHT + 0].setBrightness(0.f);
    lights[RIGHT_EXPANDER_LIGHT + 1].setBrightness(1.f);
    lights[RIGHT_EXPANDER_LIGHT + 2].setBrightness(0.f);
  }
  /* yellow case
     else if (validRightExpander) {
     lights[RIGHT_EXPANDER_LIGHT + 0].setBrightness(1.f);
     lights[RIGHT_EXPANDER_LIGHT + 1].setBrightness(1.f);
     lights[RIGHT_EXPANDER_LIGHT + 2].setBrightness(0.f);
     }
  */
  else {  // red
    lights[RIGHT_EXPANDER_LIGHT + 0].setBrightness(1.f);
    lights[RIGHT_EXPANDER_LIGHT + 1].setBrightness(0.f);
    lights[RIGHT_EXPANDER_LIGHT + 2].setBrightness(0.f);
  }

}

static const uint8_t midiManufacturerId = 0x7d;
static const uint8_t midiSysexDiscoveryReport = 0x01;


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
  int8_t cvChannelOffset;
  int8_t outputDeviceId;
  int8_t slotNum;
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

  // first pass: just parse.  These are in slot order, so i == slot
  for (int8_t i = 0; i < numReportsCards; ++i) {
    int base = byteOffset + (i * 3);
    uint8_t id = msg.bytes[base];
    int8_t cv = msg.bytes[base + 1];
    int8_t dev = msg.bytes[base + 2];

    // 0x00 and 0xFF are not valid hardware Ids
    if (id != 0x00 && id != 0xFF) {
      cards[i] = { id, cv, dev, i, true };
    }
    else {
      cards[i] = { 0, 0, 0, i, false };
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
      slotNum = cards[i].slotNum;
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
      slot.slotNum = cards[i].slotNum;
      slot.isAllocated = false;
      INFO("registered hardware id %d midi channel %d", slot.hardwareId, slot.midiChannel);
    }
    else {
      WARN("too many voice cards");
    }

  }

  broker.registerDevices(deviceTree, participant_count);
}


// add our own samples to the DSP frame
void AudioIO::processSelfSamples(rack::dsp::Frame<maxAudioChannels> &sharedFrame) {
  float v;
  bool clipped;
  static constexpr float clipTime = 0.25f;
  // out1 level
  v = params[OUT1_LEVEL_KNOB_PARAM].getValue() + inputs[OUT1_LEVEL_INPUT].getVoltageSum() / 10.f;
  clipped = (v < 0.f) || (v > 1.f);
  sharedFrame.samples[cvChannelOffset + OUT1_CHANNEL] = clamp(v, 0.f, 1.f);
  if (clipped) {
    out1LevelClipTimer = clipTime;
  }

  // out2 level
  v = params[OUT2_LEVEL_KNOB_PARAM].getValue() + inputs[OUT2_LEVEL_INPUT].getVoltageSum() / 10.f;
  clipped = (v < 0.f) || (v > 1.f);
  sharedFrame.samples[cvChannelOffset + OUT2_CHANNEL] = clamp(v, 0.f, 1.f);
  if (clipped) {
    out2LevelClipTimer = clipTime;
  }

  // TODO: validate the below and remove the above
  /*
  processCvRoutes(routes.data(),
                  routes.size(),
                  clipTime,
                  cvChannelOffset,
                  sharedFrame.samples,
                  params.data(),
                  inputs.data());
  */

}


// send all audio frames to the audio ports
void AudioIO::sendFramesToDevices(rack::dsp::Frame<maxAudioChannels> *sharedFrame, int numFrames) {
  for (int deviceNum = 0; deviceNum < numFrames; ++deviceNum) {
    if (!audioPorts[deviceNum]->engineInputBuffer.full()) {
      audioPorts[deviceNum]->engineInputBuffer.push(sharedFrame[deviceNum]);
    }
  }
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


void AudioIO::scanParticipants() {

  for (int64_t moduleId : APP->engine->getModuleIds()) {
    Module *m = APP->engine->getModule(moduleId);
    auto* p = dynamic_cast<ParticipantAdapter*>(m);
    if (!p) {
      continue;
    }

    auto& lifecycle = p->getLifecycle();

    if (lifecycle.wantAttach()) {
      INFO("completing attach for module %" PRId64, moduleId);
      if (lifecycle.completeAttach(&broker, p->getParticipant())) {
        p->onAttach();
      }
    }
  }
}




struct AudioIOWidget : ModuleWidget {
  AudioIOWidget(AudioIO* module) :
    outputA1Text(nullptr), outputA2Text(nullptr),
    outputB1Text(nullptr), outputB2Text(nullptr),
    outputC1Text(nullptr), outputC2Text(nullptr),
    outputD1Text(nullptr), outputD2Text(nullptr),
    outputE1Text(nullptr), outputE2Text(nullptr),
    outputF1Text(nullptr), outputF2Text(nullptr) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/AudioIO.svg")));

    if (module != nullptr) {
      Broker& b = module->getBroker();
      const std::shared_ptr<HardwareNameService> hns = b.getHardwareNameService();
      if (hns) {
        outputA1Text = hns->getNamePtr(0);
        outputA2Text = hns->getNamePtr(1);
        outputB1Text = hns->getNamePtr(2);
        outputB2Text = hns->getNamePtr(3);
        outputC1Text = hns->getNamePtr(4);
        outputC2Text = hns->getNamePtr(5);
        outputD1Text = hns->getNamePtr(6);
        outputD2Text = hns->getNamePtr(7);
        outputE1Text = hns->getNamePtr(8);
        outputE2Text = hns->getNamePtr(9);
        outputF1Text = hns->getNamePtr(10);
        outputF2Text = hns->getNamePtr(11);
      }
    }

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


    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(11.275, 104.378)), module, AudioIO::OUT1_LEVEL_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(11.275, 117.427)), module, AudioIO::OUT2_LEVEL_KNOB_PARAM));


    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(26.122, 104.378)), module, AudioIO::OUT1_LEVEL_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(26.122, 117.427)), module, AudioIO::OUT2_LEVEL_INPUT));


    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(19.361, 101.184)), module, AudioIO::OUT1_LEVEL_CLIP_LIGHT));
    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(19.361, 114.233)), module, AudioIO::OUT2_LEVEL_CLIP_LIGHT));

    // mm2px(Vec(22.0, 3.636))
    cardAOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 16.344)));
    cardAOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardAOutput1TextField->setText(outputA1Text);
    addChild(cardAOutput1TextField);

    // mm2px(Vec(22.0, 3.636))
    cardAOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 21.035)));
    cardAOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardAOutput2TextField->setText(outputA2Text);
    addChild(cardAOutput2TextField);

    // mm2px(Vec(22.0, 3.636))
    cardBOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 29.406)));
    cardBOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardBOutput1TextField->setText(outputB1Text);
    addChild(cardBOutput1TextField);

    // mm2px(Vec(22.0, 3.636))
    cardBOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 34.592)));
    cardBOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardBOutput2TextField->setText(outputB2Text);
    addChild(cardBOutput2TextField);

    // mm2px(Vec(22.0, 3.636))
    cardCOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 42.467)));
    cardCOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardCOutput1TextField->setText(outputC1Text);
    addChild(cardCOutput1TextField);

    // mm2px(Vec(22.0, 3.636))
    cardCOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 47.654)));
    cardCOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardCOutput2TextField->setText(outputC2Text);
    addChild(cardCOutput2TextField);


    // mm2px(Vec(22.0, 3.636))
    cardDOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 55.529)));
    cardDOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardDOutput1TextField->setText(outputD1Text);
    addChild(cardDOutput1TextField);

    // mm2px(Vec(22.0, 3.636))
    cardDOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 60.715)));
    cardDOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardDOutput2TextField->setText(outputD2Text);
    addChild(cardDOutput2TextField);

    // mm2px(Vec(22.0, 3.636))
    cardEOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 68.59)));
    cardEOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardEOutput1TextField->setText(outputE1Text);
    addChild(cardEOutput1TextField);

    // mm2px(Vec(22.0, 3.636))
    cardEOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 73.777)));
    cardEOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardEOutput2TextField->setText(outputE2Text);
    addChild(cardEOutput2TextField);

    // mm2px(Vec(22.0, 3.636))
    cardFOutput1TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 81.652)));
    cardFOutput1TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardFOutput1TextField->setText(outputF1Text);
    addChild(cardFOutput1TextField);

    // mm2px(Vec(22.0, 3.636))
    cardFOutput2TextField = createWidget<CardTextDisplay>(mm2px(Vec(6.321, 86.838)));
    cardFOutput2TextField->box.size = (mm2px(Vec(22.0, 3.636)));
    cardFOutput2TextField->setText(outputF2Text);
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

  CardTextDisplay *cardAOutput1TextField;
  CardTextDisplay *cardAOutput2TextField;
  std::string *outputA1Text;
  std::string *outputA2Text;

  CardTextDisplay *cardBOutput1TextField;
  CardTextDisplay *cardBOutput2TextField;
  std::string *outputB1Text;
  std::string *outputB2Text;

  CardTextDisplay *cardCOutput1TextField;
  CardTextDisplay *cardCOutput2TextField;
  std::string *outputC1Text;
  std::string *outputC2Text;

  CardTextDisplay *cardDOutput1TextField;
  CardTextDisplay *cardDOutput2TextField;
  std::string *outputD1Text;
  std::string *outputD2Text;

  CardTextDisplay *cardEOutput1TextField;
  CardTextDisplay *cardEOutput2TextField;
  std::string *outputE1Text;
  std::string *outputE2Text;

  CardTextDisplay *cardFOutput1TextField;
  CardTextDisplay *cardFOutput2TextField;
  std::string *outputF1Text;
  std::string *outputF2Text;
};


const std::string AudioIO::audioPortNum = "audioPort";

// see Zoxnoxious MIDI spec for details

const midi::Message AudioIO::MIDI_DISCOVERY_REQUEST_SYSEX = []{
  midi::Message m;
  m.bytes = { 0xF0, 0x7D, 0x02, 0xF7 };
  return m;
}();

const midi::Message AudioIO::MIDI_SHUTDOWN_SYSEX = []{
  midi::Message m;
  m.bytes = { 0xF0, 0x7D, 0x03, 0xF7 };
  return m;
}();

const midi::Message AudioIO::MIDI_RESTART_SYSEX = []{
  midi::Message m;
  m.bytes = { 0xF0, 0x7D, 0x04, 0xF7 };
  return m;
}();

const midi::Message AudioIO::MIDI_TUNE_REQUEST = []{
  midi::Message m;
  m.bytes = { 0xF6 };
  return m;
}();


const std::vector<ButtonMapping<AudioIO> > AudioIO::buttonMappings = {
    { CARD_A_MIX1_OUTPUT_BUTTON_PARAM, CARD_A_MIX1_OUTPUT_BUTTON_LIGHT, { 0, 1} },
    { CARD_B_MIX1_OUTPUT_BUTTON_PARAM, CARD_B_MIX1_OUTPUT_BUTTON_LIGHT, { 2, 3} },
    { CARD_C_MIX1_OUTPUT_BUTTON_PARAM, CARD_C_MIX1_OUTPUT_BUTTON_LIGHT, { 4, 5} },
    { CARD_D_MIX1_OUTPUT_BUTTON_PARAM, CARD_D_MIX1_OUTPUT_BUTTON_LIGHT, { 6, 7} },
    { CARD_E_MIX1_OUTPUT_BUTTON_PARAM, CARD_E_MIX1_OUTPUT_BUTTON_LIGHT, { 8, 9} },
    { CARD_F_MIX1_OUTPUT_BUTTON_PARAM, CARD_F_MIX1_OUTPUT_BUTTON_LIGHT, { 10, 11} },
    { CARD_A_MIX2_OUTPUT_BUTTON_PARAM, CARD_A_MIX2_OUTPUT_BUTTON_LIGHT, { 12, 13} },
    { CARD_B_MIX2_OUTPUT_BUTTON_PARAM, CARD_B_MIX2_OUTPUT_BUTTON_LIGHT, { 14, 15} },
    { CARD_C_MIX2_OUTPUT_BUTTON_PARAM, CARD_C_MIX2_OUTPUT_BUTTON_LIGHT, { 16, 17} },
    { CARD_D_MIX2_OUTPUT_BUTTON_PARAM, CARD_D_MIX2_OUTPUT_BUTTON_LIGHT, { 18, 19} },
    { CARD_E_MIX2_OUTPUT_BUTTON_PARAM, CARD_E_MIX2_OUTPUT_BUTTON_LIGHT, { 20, 21} },
    { CARD_F_MIX2_OUTPUT_BUTTON_PARAM, CARD_F_MIX2_OUTPUT_BUTTON_LIGHT, { 22, 23} }
};


} // namespace zox

Model* modelAudioIO = createModel<zox::AudioIO, zox::AudioIOWidget>("AudioIO");

