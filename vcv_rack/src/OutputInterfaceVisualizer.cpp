#include "plugin.hpp"

#include "zcomponentlib.hpp"
#include "OutputInterface.hpp"

namespace zox {

struct OutputInterfaceVisualizer : Module {
  enum ParamId {
    PARAMS_LEN
  };
  enum InputId {
    INPUTS_LEN
  };
  enum OutputId {
    OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };

  dsp::ClockDivider clockDivider;

  OutputInterfaceVisualizer() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    clockDivider.setDivision(51200);
  }


  static const char* graphPortName(GraphPort port) {
    return port == GraphPort::A ? "A" : "B";
  }

  void dumpParticipantGraphs() const {
    ParticipantGraphMessage *message = static_cast<ParticipantGraphMessage*>(leftExpander.module->rightExpander.consumerMessage);

    INFO("Participant graph dump: %zu participants", message->participantInfoCount);

    for (size_t i = 0; i < message->participantInfoCount; ++i) {
      const ParticipantGraphInfo& info = message->participantInfos[i];
      INFO("  node moduleId=%lld hardwareId=%d",
           (long long) info.moduleId,
           info.hardwareId);

      const GraphSource sources[2] = {info.source1, info.source2};

      for (int i = 0; i < 2; ++i) {
        const GraphSource& src = sources[i];
        if (!src.valid) {
          INFO("    source%d: none", i + 1);
          continue;
        }

        INFO("    source%d: slot=%d moduleId=%lld port=%s",
             i + 1, src.slotNum, (long long) src.moduleId, graphPortName(src.port));
      }
    }

    for (size_t i = 0; i < message->output1SourceCount; ++i) {
      const GraphSource& g = message->output1Sources[i];
      INFO("  Out1: %lld", g.moduleId);
    }
    for (size_t i = 0; i < message->output2SourceCount; ++i) {
      const GraphSource& g = message->output2Sources[i];
      INFO("  Out2: %lld", g.moduleId);
    }
  }



  void process(const ProcessArgs& args) override {
    if (clockDivider.process()) {
      if (leftExpander.module && leftExpander.module->model == modelOutputInterface) {
        dumpParticipantGraphs();
      }
    }
  }

};






struct OutputInterfaceVisualizerWidget : ModuleWidget {

  OutputInterfaceVisualizerWidget(OutputInterfaceVisualizer* module) {
    setModule(module);

    setPanel(createPanel(asset::plugin(pluginInstance, "res/OutputInterfaceVisualizer.svg")));

    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    if (module) {
    }
  }
};


} // namespace zox

Model* modelOutputInterfaceVisualizer = createModel<zox::OutputInterfaceVisualizer, zox::OutputInterfaceVisualizerWidget>("OutputInterfaceVisualizer");
