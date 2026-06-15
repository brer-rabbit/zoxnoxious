#include "plugin.hpp"

#include "zcomponentlib.hpp"
#include "OutputInterface.hpp"

namespace zox {

static constexpr size_t maxGraphNodes = 7;
static constexpr size_t maxGraphEdges = 24; // was 16

enum class GraphTargetKind : uint8_t {
  Participant,
  Output1,
  Output2
};

struct GraphEdgeRenderInfo {
  bool valid = false;

  int64_t fromModuleId = -1;
  GraphPort fromPort = GraphPort::A;

  GraphTargetKind targetKind = GraphTargetKind::Participant;
  int64_t toModuleId = -1;

  float weight = 1.f;
};

struct GraphRenderSnapshot {
  size_t edgeCount = 0;
  GraphEdgeRenderInfo edges[maxGraphEdges];
};


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
  GraphRenderSnapshot graphSnapshot;


  void buildGraphRenderSnapshot(ParticipantGraphMessage *message) {
    graphSnapshot = GraphRenderSnapshot{};

    for (size_t i = 0; i < message->participantInfoCount; ++i) {
      const ParticipantGraphInfo& info = message->participantInfos[i];
      const GraphSource sources[2] = {info.source1, info.source2};

      for (int i = 0; i < 2; ++i) {
        const GraphSource& src = sources[i];

        if (!src.valid || src.moduleId < 0 || info.moduleId < 0) {
          continue;
        }
        if (graphSnapshot.edgeCount >= maxGraphEdges) {
          return;
        }

        GraphEdgeRenderInfo& edge = graphSnapshot.edges[graphSnapshot.edgeCount++];
        edge.valid = true;
        edge.fromModuleId = src.moduleId;
        edge.fromPort = src.port;
        edge.toModuleId = info.moduleId;
        edge.weight = src.inputWeight * info.outputWeight;
      }
    }

    for (size_t i = 0; i < message->output1SourceCount; ++i) {
        GraphEdgeRenderInfo& edge = graphSnapshot.edges[graphSnapshot.edgeCount++];
        edge.valid = true;
        edge.fromModuleId = message->output1Sources[i].moduleId;
        edge.fromPort = GraphPort::A;
        edge.toModuleId = getId(); // it's the output module, don't need the ID
        edge.targetKind = GraphTargetKind::Output1;
        edge.weight = 0.5f;
    }
    for (size_t i = 0; i < message->output2SourceCount; ++i) {
        GraphEdgeRenderInfo& edge = graphSnapshot.edges[graphSnapshot.edgeCount++];
        edge.valid = true;
        edge.fromModuleId = message->output2Sources[i].moduleId;
        edge.fromPort = GraphPort::B;
        edge.toModuleId = getId(); // it's the output module, don't need the ID
        edge.targetKind = GraphTargetKind::Output2;
        edge.weight = 0.5f;
    }

  }


  OutputInterfaceVisualizer() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    clockDivider.setDivision(51200);
  }


  static const char* graphPortName(GraphPort port) {
    return port == GraphPort::A ? "A" : "B";
  }

  void dumpParticipantGraphs(ParticipantGraphMessage *message) const {

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
        ParticipantGraphMessage *message = static_cast<ParticipantGraphMessage*>(leftExpander.module->rightExpander.consumerMessage);
        if (message) {
          buildGraphRenderSnapshot(message);
          dumpParticipantGraphs(message); // DEBUG ONLY
        }
      }
    }
  }

};




struct SystemRoutingVisualizerDisplay : LedDisplay {
  GraphRenderSnapshot *snapshot;

  SystemRoutingVisualizerDisplay() {
  }

  void drawLayer(const DrawArgs& args, int layer) override {
    if (layer != 1 || !snapshot) {
      return;
    }

    drawBackground(args.vg);
    drawGraph(args.vg, *snapshot);
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
      auto* display = createWidget<SystemRoutingVisualizerDisplay>(mm2px(Vec(5.5, 15.0)));
      display->box.size = mm2px(Vec(60.0, 60.0));
      display->snapshot = &module->graphSnapshot;
      addChild(display);

    }
  }
};


} // namespace zox

Model* modelOutputInterfaceVisualizer = createModel<zox::OutputInterfaceVisualizer, zox::OutputInterfaceVisualizerWidget>("OutputInterfaceVisualizer");
