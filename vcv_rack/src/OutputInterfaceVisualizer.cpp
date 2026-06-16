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
          WARN("maxGraphEdges exceeded");
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

    for (size_t i = 0; i < message->output1SourceCount && i < maxGraphEdges; ++i) {
        GraphEdgeRenderInfo& edge = graphSnapshot.edges[graphSnapshot.edgeCount++];
        edge.valid = true;
        edge.fromModuleId = message->output1Sources[i].moduleId;
        edge.fromPort = GraphPort::A;
        edge.toModuleId = getId(); // it's the output module, don't need the ID
        edge.targetKind = GraphTargetKind::Output1;
        edge.weight = 0.5f;
    }
    for (size_t i = 0; i < message->output2SourceCount && i < maxGraphEdges; ++i) {
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
    clockDivider.setDivision(512);
  }


  static const char* graphPortName(GraphPort port) {
    return port == GraphPort::A ? "A" : "B";
  }

#if 0
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
#endif


  void process(const ProcessArgs& args) override {
    if (clockDivider.process()) {
      if (leftExpander.module && leftExpander.module->model == modelOutputInterface) {
        ParticipantGraphMessage *message = static_cast<ParticipantGraphMessage*>(leftExpander.module->rightExpander.consumerMessage);
        if (message) {
          buildGraphRenderSnapshot(message);
        }
      }
    }
  }

};




struct GraphNodeRenderInfo {
  bool valid = false;
  int64_t moduleId = -1;
  int col = 0;
  int row = 0;
  Vec center;
};

struct GraphLayout {
  size_t nodeCount = 0;
  GraphNodeRenderInfo nodes[maxGraphNodes];

  Vec output1Center;
  Vec output2Center;
};


static bool containsNode(const GraphLayout& layout, int64_t moduleId) {
  for (size_t i = 0; i < layout.nodeCount; ++i) {
    if (layout.nodes[i].valid && layout.nodes[i].moduleId == moduleId)
      return true;
  }
  return false;
}

static void addNode(GraphLayout& layout, int64_t moduleId) {

  if (moduleId < 0) {
    // bizrule: only graph instantiated module
    return;
  }

  if (containsNode(layout, moduleId) || layout.nodeCount >= maxGraphNodes) {
    // if already added the just move along; bail if we're at a maximum
    return;
  }

  GraphNodeRenderInfo& node = layout.nodes[layout.nodeCount++];
  node.valid = true;
  node.moduleId = moduleId;
}

static bool nodeIsDestination(const GraphRenderSnapshot& snapshot, int64_t moduleId) {
  for (size_t i = 0; i < snapshot.edgeCount; ++i) {
    const GraphEdgeRenderInfo& e = snapshot.edges[i];
    if (e.valid && e.targetKind == GraphTargetKind::Participant && e.toModuleId == moduleId) {
      return true;
    }
  }
  return false;
}

// Currently unused:
static bool nodeFeedsDestination(const GraphRenderSnapshot& snapshot, int64_t moduleId) {
  for (size_t i = 0; i < snapshot.edgeCount; ++i) {
    const GraphEdgeRenderInfo& e = snapshot.edges[i];
    if (e.valid && e.fromModuleId == moduleId) {
      return true;
    }
  }
  return false;
}


static Vec gridCenter(const Rect& r, int col, int row) {
  const float marginX = 8.f;
  const float marginY = 8.f;

  const float usableW = r.size.x - 2.f * marginX;
  const float usableH = r.size.y - 2.f * marginY;

  const float colW = usableW / 4.f;
  const float rowH = usableH / 3.f;

  return Vec(r.pos.x + marginX + colW * (col + 0.5f),
             r.pos.y + marginY + rowH * (row + 0.5f) );
}


static void buildLayout(const GraphRenderSnapshot& snapshot, GraphLayout& layout, const Rect& box) {

  for (size_t i = 0; i < snapshot.edgeCount; ++i) {
    const GraphEdgeRenderInfo &edge = snapshot.edges[i];
    addNode(layout, edge.fromModuleId);

    if (edge.targetKind == GraphTargetKind::Participant) {
      addNode(layout, edge.toModuleId);
    }
  }

  for (size_t i = 0; i < layout.nodeCount; ++i) {
    GraphNodeRenderInfo& n = layout.nodes[i];

    if (nodeIsDestination(snapshot, n.moduleId)) {
      n.col = 1;
    }
    else {
      n.col = 0;
    }
  }

  int rowCounts[3] = {0, 0, 0};

  for (size_t i = 0; i < layout.nodeCount; ++i) {
    GraphNodeRenderInfo& n = layout.nodes[i];

    if (n.col < 0) n.col = 0;
    if (n.col > 2) n.col = 2;

    n.row = rowCounts[n.col]++;
  }

  for (size_t i = 0; i < layout.nodeCount; ++i) {
    GraphNodeRenderInfo& n = layout.nodes[i];
    n.center = gridCenter(box, n.col, n.row);
  }

  layout.output1Center = gridCenter(box, 3, 0);
  layout.output2Center = gridCenter(box, 3, 2);
}


static Rect nodeRectFromCenter(Vec center, Vec size) {
  return Rect(center.minus(size.div(2.f)), size);
}


struct SystemRoutingVisualizerDisplay : LedDisplay {
  GraphRenderSnapshot *snapshot;

  static constexpr float nodeW = 22.f;
  static constexpr float nodeH = 12.f;
  static constexpr float outputW = 20.f;
  static constexpr float outputH = 10.f;

  SystemRoutingVisualizerDisplay() {
  }


  NVGcolor bgColor() const {
    return nvgRGB(7, 12, 13);
  }

  NVGcolor nodeFill() const {
    return nvgRGB(25, 39, 40);
  }

  NVGcolor nodeStroke(float a = 1.f) const {
    return nvgRGBAf(0.48f, 0.65f, 0.60f, a);
  }


  NVGcolor displayTextColor(float a = 1.f) const {
    return nvgRGBAf(0.70f, 0.88f, 0.78f, a);
  }

  void drawText(NVGcontext* vg, Vec p, const char* text, float size,
                int align = NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE,
                float alpha = 1.f) {
    nvgFontSize(vg, size);
    nvgFontFaceId(vg, APP->window->uiFont->handle);
    nvgTextAlign(vg, align);
    nvgFillColor(vg, displayTextColor(alpha));
    nvgText(vg, p.x, p.y, text, NULL);
  }

  void drawNode(NVGcontext* vg, Vec center, Vec size, const char* label) {
    Rect r = nodeRectFromCenter(center, size);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 2.5f);
    nvgFillColor(vg, nodeFill());
    nvgFill(vg);

    nvgStrokeWidth(vg, 0.9f);
    nvgStrokeColor(vg, nodeStroke(0.95f));
    nvgStroke(vg);

    drawText(vg, r.getCenter(), label, 6.0f);
  }

  void drawNodes(NVGcontext* vg, const GraphLayout& layout) {
    const Vec nodeSize = Vec(nodeW, nodeH);

    for (size_t i = 0; i < layout.nodeCount; ++i) {
      const GraphNodeRenderInfo& node = layout.nodes[i];

      if (!node.valid)
        continue;

      drawNode(vg, node.center, nodeSize, "VCO");
    }
  }

  void drawOutputs(NVGcontext* vg, const GraphLayout& layout) {
    const Vec outputSize = Vec(outputW, outputH);

    drawNode(vg, layout.output1Center, outputSize, "OUT1");
    drawNode(vg, layout.output2Center, outputSize, "OUT2");
  }


  void drawLayer(const DrawArgs& args, int layer) override {
    if (layer != 1 || !snapshot) {
      return;
    }

    GraphLayout layout;
    Rect localBox = Rect(Vec(0.f, 0.f), box.size);
    buildLayout(*snapshot, layout, localBox);

    drawNodes(args.vg, layout);
    drawOutputs(args.vg, layout);
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
