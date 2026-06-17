#include "plugin.hpp"

#include "zcomponentlib.hpp"
#include "OutputInterface.hpp"

namespace zox {

static constexpr int8_t maxGraphNodes = maxVoiceCards + 1; // allow for output interface
static constexpr size_t maxGraphEdges = 24; // was 16

enum class RenderNodeKind : uint8_t {
  VoiceCard,
  Output,
  ExternalInput
};

enum class RenderTargetKind : uint8_t {
  VoiceCard,
  Output1,
  Output2
};

// slotNum not present here: the slotNum is the index in an array of these
struct GraphRenderNode {
  int64_t moduleId = -1; // for Rack-side presence only

  int col = 0;
  int row = 0;
  Vec center;

  uint8_t hardwareId = 0;
  RenderNodeKind kind = RenderNodeKind::VoiceCard;

  bool valid = false;
};

struct GraphRenderEdge {
  int8_t fromSlotNum = invalidSlot;
  uint8_t fromHardwareId = 0;
  int64_t fromModuleId = -1;
  GraphPort fromPort = GraphPort::A;

  RenderTargetKind targetKind = RenderTargetKind::VoiceCard;

  int8_t toSlotNum = invalidSlot;     // valid for participant target
  uint8_t toHardwareId = 0;
  int64_t toModuleId = -1;

  float weight = 1.f;                 // ignore visually for now
  bool valid = false;
};

struct GraphRenderSnapshot {
  GraphRenderNode nodes[maxGraphNodes];

  size_t edgeCount = 0;
  GraphRenderEdge edges[maxGraphEdges];
};


struct OutputInterfaceVisualizer final : Module {
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


  void addSource(const ParticipantGraphInfo& participantInfo,
                 const GraphSource source,
                 const RenderTargetKind targetKind) {
    // fill in the edges of the graph
    if (graphSnapshot.edgeCount >= maxGraphEdges) {
      WARN("maxGraphEdges exceeded");
      return;
    }

    // this filters to only instantiated/rendered edges by policy
    if (source.valid && source.slotNum != invalidSlot && source.moduleId >= 0) {
      GraphRenderEdge& edge = graphSnapshot.edges[graphSnapshot.edgeCount++];
      edge.fromSlotNum = source.slotNum;
      edge.fromHardwareId = source.hardwareId;
      edge.fromModuleId = source.moduleId;
      edge.fromPort = source.port;
      edge.targetKind = targetKind;
      edge.toSlotNum = participantInfo.slotNum;
      edge.toHardwareId = participantInfo.hardwareId;
      edge.toModuleId = participantInfo.moduleId;
      edge.valid = true;
      edge.weight = source.inputWeight * participantInfo.outputWeight;
    }
  }


  void addNode(int8_t slotNum, uint8_t hardwareId, int64_t moduleId, RenderNodeKind kind) {
    if (slotNum >= 0 && slotNum < maxGraphNodes) {
      if (moduleId >= 0) {
        graphSnapshot.nodes[slotNum].valid = true;
        graphSnapshot.nodes[slotNum].moduleId = moduleId;
        graphSnapshot.nodes[slotNum].hardwareId = hardwareId;
        graphSnapshot.nodes[slotNum].kind = kind;
      }
      else {
        graphSnapshot.nodes[slotNum].valid = false;
      }
    }
  }

  void buildGraphRenderSnapshot(const ParticipantGraphMessage& message) {
    graphSnapshot = GraphRenderSnapshot{};

    for (size_t i = 0; i < message.participantInfoCount; ++i) {
      const ParticipantGraphInfo& info = message.participantInfos[i];

      if (info.moduleId < 0) {
          continue;
      }

      addNode(info.slotNum, info.hardwareId, info.moduleId, RenderNodeKind::VoiceCard);
      addSource(info, info.source1, RenderTargetKind::VoiceCard);
      addSource(info, info.source2, RenderTargetKind::VoiceCard);
    }


    // always show the output nodes
    addNode(message.outputInterfaceInfo.slotNum,
            message.outputInterfaceInfo.hardwareId,
            message.outputInterfaceInfo.moduleId,
            RenderNodeKind::Output);

    // add Output1 sources
    for (size_t i = 0; i < message.output1SourceCount; ++i) {
      addSource(message.outputInterfaceInfo, message.output1Sources[i], RenderTargetKind::Output1);
    }

    // and Output2 sources
    for (size_t i = 0; i < message.output2SourceCount; ++i) {
      addSource(message.outputInterfaceInfo, message.output2Sources[i], RenderTargetKind::Output2);
    }

  }


  OutputInterfaceVisualizer() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    clockDivider.setDivision(512);
  }


  static const char* graphPortName(GraphPort port) {
    return port == GraphPort::A ? "A" : "B";
  }

  static const char* renderNodeKindName(RenderNodeKind kind) {
    switch (kind) {
    case RenderNodeKind::VoiceCard:     return "VoiceCard";
    case RenderNodeKind::Output:        return "Output";
    case RenderNodeKind::ExternalInput: return "ExternalInput";
    }
    return "?";
  }

  static const char* renderTargetKindName(RenderTargetKind kind) {
    switch (kind) {
    case RenderTargetKind::VoiceCard: return "VoiceCard";
    case RenderTargetKind::Output1:   return "Output1";
    case RenderTargetKind::Output2:   return "Output2";
    }
    return "?";
  }


  void dumpGraphRenderSnapshot() const {
    INFO("========== GraphRenderSnapshot ==========");
    INFO("Nodes:");
    for (size_t i = 0; i < maxGraphNodes; ++i) {
      const GraphRenderNode& node = graphSnapshot.nodes[i];

      if (!node.valid)
        continue;

      INFO("  slot=%zu moduleId=%lld hwId=%u kind=%s col=%d row=%d",
           i,
           (long long)node.moduleId,
           node.hardwareId,
           renderNodeKindName(node.kind),
           node.col,
           node.row);
    }

    INFO("Edges: count=%zu", graphSnapshot.edgeCount);

    for (size_t i = 0; i < graphSnapshot.edgeCount; ++i) {
      const GraphRenderEdge& edge = graphSnapshot.edges[i];

      if (!edge.valid)
        continue;

      INFO("  [%zu] slot%d:%s(hw=%u mod=%lld) -> %s slot%d(hw=%u mod=%lld) weight=%.2f",
           i,
           edge.fromSlotNum,
           graphPortName(edge.fromPort),
           edge.fromHardwareId,
           (long long)edge.fromModuleId,

           renderTargetKindName(edge.targetKind),

           edge.toSlotNum,
           edge.toHardwareId,
           (long long)edge.toModuleId,

           edge.weight);
    }
  }



  void process(const ProcessArgs& args) override {
    if (clockDivider.process()) {
      if (leftExpander.module && leftExpander.module->model == modelOutputInterface) {
        ParticipantGraphMessage *message = static_cast<ParticipantGraphMessage*>(leftExpander.module->rightExpander.consumerMessage);
        if (message) {
          buildGraphRenderSnapshot(*message);
          //dumpGraphRenderSnapshot();
        }
      }
    }
  }

};



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

#if 0
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

      drawNode(vg, node.center, nodeSize, getCardNameByHardwareId(node.hardwareId));
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
#endif

struct OutputInterfaceVisualizerWidget : ModuleWidget {

  OutputInterfaceVisualizerWidget(OutputInterfaceVisualizer* module) {
    setModule(module);

    setPanel(createPanel(asset::plugin(pluginInstance, "res/OutputInterfaceVisualizer.svg")));

    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    if (module) {
#if 0
      auto* display = createWidget<SystemRoutingVisualizerDisplay>(mm2px(Vec(5.5, 15.0)));
      display->box.size = mm2px(Vec(60.0, 60.0));
      display->snapshot = &module->graphSnapshot;
      addChild(display);
#endif
    }
  }
};


} // namespace zox

Model* modelOutputInterfaceVisualizer = createModel<zox::OutputInterfaceVisualizer, zox::OutputInterfaceVisualizerWidget>("OutputInterfaceVisualizer");
