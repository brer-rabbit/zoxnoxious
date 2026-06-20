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

  uint8_t hardwareId = invalidCardId;
  RenderNodeKind kind = RenderNodeKind::VoiceCard;

  float output1Weight = 0.5f;
  float output2Weight = 0.5f;

  bool valid = false;
};

struct GraphRenderEdge {
  int8_t fromSlotNum = invalidSlot;
  uint8_t fromHardwareId = 0;
  int64_t fromModuleId = -1;
  GraphPort fromPort = GraphPort::OUT1;

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
      edge.weight = source.inputWeight;
    }
  }


  void addNode(const ParticipantGraphInfo& info, RenderNodeKind kind) {
    if (info.slotNum >= 0 && info.slotNum < maxGraphNodes) {
      if (info.moduleId >= 0) {
        graphSnapshot.nodes[info.slotNum].valid = true;
        graphSnapshot.nodes[info.slotNum].moduleId = info.moduleId;
        graphSnapshot.nodes[info.slotNum].hardwareId = info.hardwareId;
        graphSnapshot.nodes[info.slotNum].kind = kind;
        graphSnapshot.nodes[info.slotNum].output1Weight = info.output1Weight;
        graphSnapshot.nodes[info.slotNum].output2Weight = info.output2Weight;
      }
      else {
        graphSnapshot.nodes[info.slotNum].valid = false;
      }
    }
  }

  // iterate over all the edges:
  // find the "from" slot and incorporate the source weight via simple multiply
  void calculateEdgeWeights() {
    for (size_t i = 0; i < graphSnapshot.edgeCount; ++i) {

      if (graphSnapshot.edges[i].valid &&
          graphSnapshot.edges[i].fromSlotNum >= 0 &&
          graphSnapshot.edges[i].fromSlotNum < maxGraphNodes &&
          graphSnapshot.nodes[ graphSnapshot.edges[i].fromSlotNum ].moduleId != -1 &&
          graphSnapshot.nodes[ graphSnapshot.edges[i].fromSlotNum ].hardwareId != invalidCardId) {

        graphSnapshot.edges[i].weight *= (graphSnapshot.edges[i].fromPort == GraphPort::OUT1) ?
          graphSnapshot.nodes[ graphSnapshot.edges[i].fromSlotNum ].output1Weight :
          graphSnapshot.nodes[ graphSnapshot.edges[i].fromSlotNum ].output2Weight;
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

      addNode(info, RenderNodeKind::VoiceCard);
      addSource(info, info.source1, RenderTargetKind::VoiceCard);
      addSource(info, info.source2, RenderTargetKind::VoiceCard);
    }


    // always show the output nodes
    addNode(message.outputInterfaceInfo,
            RenderNodeKind::Output);

    // add Output1 sources
    for (size_t i = 0; i < message.output1SourceCount; ++i) {
      addSource(message.outputInterfaceInfo, message.output1Sources[i], RenderTargetKind::Output1);
    }

    // and Output2 sources
    for (size_t i = 0; i < message.output2SourceCount; ++i) {
      addSource(message.outputInterfaceInfo, message.output2Sources[i], RenderTargetKind::Output2);
    }

    calculateEdgeWeights();

  }


  OutputInterfaceVisualizer() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    clockDivider.setDivision(512);
  }


  static const char* graphPortName(GraphPort port) {
    return port == GraphPort::OUT1 ? "OUT1" : "OUT2";
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



// ---- visualizer

struct GraphLayout {
  Vec nodeCenters[maxGraphNodes];

  Vec output1Center;
  Vec output2Center;
};



static Rect nodeRectFromCenter(Vec center, Vec size) {
  return Rect(center.minus(size.div(2.f)), size);
}


static bool edgeTargetsOutput(const GraphRenderEdge& e) {
  return e.valid &&
    (e.targetKind == RenderTargetKind::Output1 ||
     e.targetKind == RenderTargetKind::Output2);
}

static bool nodeFeedsOutput(const GraphRenderSnapshot& s, int8_t slot) {
  for (size_t i = 0; i < s.edgeCount; ++i) {
    const GraphRenderEdge& e = s.edges[i];
    if (e.valid && e.fromSlotNum == slot && edgeTargetsOutput(e)) {
      return true;
    }
  }
  return false;
}

static bool nodeFeedsNodeThatFeedsOutput(const GraphRenderSnapshot& s, int8_t slot) {
  for (size_t i = 0; i < s.edgeCount; ++i) {
    const GraphRenderEdge& e = s.edges[i];
    if (!e.valid) {
      continue;
    }
    if (e.targetKind != RenderTargetKind::VoiceCard) {
      continue;
    }
    if (e.fromSlotNum != slot) {
      continue;
    }
    if (nodeFeedsOutput(s, e.toSlotNum)) {
      return true;
    }
  }
  return false;
}


// choose a preferred column for the slot.  Must only return 0, 1, or 2.
// Does not handle RenderNodeKind::Output.
static int chooseColumn(const GraphRenderSnapshot& s, int8_t slot) {
  /*
   * const GraphRenderNode& node = s.nodes[slot];
   * if (node.kind == RenderNodeKind::Output) {
   *  return 3;
   * }
  */
  if (nodeFeedsOutput(s, slot)) {
    return 2;
  }
  
  if (nodeFeedsNodeThatFeedsOutput(s, slot)) {
    return 1;
  }
  
  return 0;
}


static Vec voiceNodeCenter(const Rect& r, int col, int row, int rowCount) {
  const float leftMargin = 5.f;
  const float rightOutputGutter = 30.f;
  const float marginY = 28.f;

  const float usableW = r.size.x - leftMargin - rightOutputGutter;
  const float usableH = r.size.y - 2.f * marginY;

  const float colW = usableW / 3.f;

  float y = r.pos.y + r.size.y * 0.5f;
  if (rowCount > 1) {
    y = r.pos.y + marginY + (usableH / float(rowCount - 1)) * row;
  }

  return Vec(r.pos.x + leftMargin + colW * (col + 0.5f), y);
}


static Vec outputCenter(const Rect& r, int outputIndex) {
  const float rightMargin = 18.f;
  const float marginY = 48.f;

  float y = outputIndex == 0
    ? r.pos.y + marginY
    : r.pos.y + r.size.y - marginY;

  return Vec(r.pos.x + r.size.x - rightMargin, y);
}

static float portOffset(GraphPort port) {
  return port == GraphPort::OUT1 ? -4.f : 4.f;
}

static Vec rightAnchor(Vec center, Vec size) {
  return Vec(center.x + size.x * 0.5f, center.y);
}

static Vec rightAnchor(Vec center, Vec size, GraphPort port) {
  return Vec(center.x + size.x * 0.5f, center.y + portOffset(port));
}

static Vec leftAnchor(Vec center, Vec size) {
  return Vec(center.x - size.x * 0.5f, center.y);
}

static Vec topAnchor(Vec center, Vec size) {
  return Vec(center.x - 8.f, center.y - size.y * 0.5f);
}

static Vec topAnchor(Vec center, Vec size, GraphPort port) {
  return Vec(center.x + portOffset(port) + 6.f, center.y - size.y * 0.5f);
}

static Vec bottomAnchor(Vec center, Vec size) {
  return Vec(center.x - 8.f, center.y + size.y * 0.5f);
}

static Vec bottomAnchor(Vec center, Vec size, GraphPort port) {
  return Vec(center.x + portOffset(port) + 6.f, center.y + size.y * 0.5f);
}


enum class AnchorSide : uint8_t { Left, Right, Top, Bottom };
enum class EdgeRouteKind : uint8_t { Normal, Vertical, Feedback, SelfLoop };

struct EdgeRoute {
  EdgeRouteKind kind = EdgeRouteKind::Normal;
  AnchorSide fromSide = AnchorSide::Right;
  AnchorSide toSide = AnchorSide::Left;
};

static EdgeRoute chooseEdgeRoute(Vec fromCenter, Vec toCenter, bool selfLoop, float midY) {
  EdgeRoute route;

  const float dx = toCenter.x - fromCenter.x;
  const float dy = toCenter.y - fromCenter.y;

  if (selfLoop) {
    route.kind = EdgeRouteKind::SelfLoop;
    route.fromSide = AnchorSide::Bottom;
    route.toSide = AnchorSide::Left;
    return route;
  }

  if (dx >= 8.f) {
    route.kind = EdgeRouteKind::Normal;
    route.fromSide = AnchorSide::Right;
    route.toSide = AnchorSide::Left;
    return route;
  }

  if (std::fabs(dy) < 4.f) {
    route.kind = EdgeRouteKind::Feedback;
    // route feedback above or below?  Check where we are in the box
    if (fromCenter.y < midY) {
      route.fromSide = AnchorSide::Top;
      route.toSide = AnchorSide::Top;
    }
    else {
      route.fromSide = AnchorSide::Bottom;
      route.toSide = AnchorSide::Bottom;
    }
    return route;
  }

  route.kind = dx < 0.f ? EdgeRouteKind::Feedback : EdgeRouteKind::Vertical;
  route.fromSide = dy > 0.f ? AnchorSide::Bottom : AnchorSide::Top;
  route.toSide   = dy > 0.f ? AnchorSide::Top    : AnchorSide::Bottom;
  return route;
}



static float getAverageConnectionY(int slot, int colForSlot[], int rowForSlot[], const GraphRenderSnapshot& s) {
  float totalY = 0.f;
  int count = 0;
  for (size_t i = 0; i < s.edgeCount; ++i) {
    const GraphRenderEdge& e = s.edges[i];
    if (e.valid && e.fromSlotNum == slot && colForSlot[e.toSlotNum] == colForSlot[slot] + 1) {
      totalY += rowForSlot[e.toSlotNum];
      count++;
    }
  }
  return (count > 0) ? (totalY / count) : rowForSlot[slot];
}

static void optimizeLayout(int colForSlot[], int rowForSlot[], const GraphRenderSnapshot& s) {
  // Iterate from right to left (Column 2 down to 0)
  for (int col = 1; col >= 0; --col) {
    // Find all nodes in the current column
    std::vector<int> colNodes;
    for (int i = 0; i < maxGraphNodes; ++i) {
      if (colForSlot[i] == col) colNodes.push_back(i);
    }

    // Simple Bubble-Sort style optimization to minimize vertical distance
    // to connected nodes in the next column
    for (size_t i = 0; i < colNodes.size(); ++i) {
      for (size_t j = i + 1; j < colNodes.size(); ++j) {
        int nodeA = colNodes[i];
        int nodeB = colNodes[j];

        // Calculate "average connection Y" for both nodes in the next column
        float costA = getAverageConnectionY(nodeA, colForSlot, rowForSlot, s);
        float costB = getAverageConnectionY(nodeB, colForSlot, rowForSlot, s);

        // If nodeB is "higher up" but has a lower Y-connection target, swap them
        if (costB < costA) {
          std::swap(rowForSlot[nodeA], rowForSlot[nodeB]);
        }
      }
    }
  }
}


struct SystemRoutingVisualizerDisplay : LedDisplay {
  GraphRenderSnapshot *snapshot = nullptr;

  static constexpr float nodeW = 36.f;
  static constexpr float nodeH = 18.f;
  static constexpr float outputW = 20.f;
  static constexpr float outputH = 14.f;

  // create a dynamic vertical stack for each column.
  // colForSlot is a lookup for which column index was assigned to a node.
  // rowForSlot is a lookup for the row the slot is assigned.
  // colCounts tracks how many nodes have been assigned to each column.
  // Two passes:
  // (1) chooseColumn gets the node's preferred column
  void buildLayout(GraphLayout& layout, const Rect& box) {
    layout = GraphLayout{};

    int colForSlot[maxGraphNodes];
    int rowForSlot[maxGraphNodes];
    int colCounts[3] = {};

    for (int i = 0; i < maxGraphNodes; ++i) {
      colForSlot[i] = -1;
      rowForSlot[i] = -1;
    }

    for (int slot = 0; slot < maxGraphNodes; ++slot) {
      const GraphRenderNode& node = snapshot->nodes[slot];

      if (!node.valid || node.kind == RenderNodeKind::Output) {
        continue;
      }

      // by contract chooseColumn must return in the 0:2 range
      int col = chooseColumn(*snapshot, slot);
      if (colCounts[col] >= 3) {
        if (col == 2) {
          // six voice cards max, if col == 2 then col 1 must have space
          col = 1;
        }
        else {
          // col 0 or 1 and it's full.  The next column right must have at least one spot
          col++;
        }
      }

      colForSlot[slot] = col;
      rowForSlot[slot] = colCounts[col]++;
    }

    // make it look good
    optimizeLayout(colForSlot, rowForSlot, *snapshot);

    for (int slot = 0; slot < maxGraphNodes; ++slot) {
      if (colForSlot[slot] < 0) {
        continue;
      }

      // todo: play with this for initial node placement by quantity
      layout.nodeCenters[slot] = voiceNodeCenter(box,
                                                 colForSlot[slot],
                                                 rowForSlot[slot],
                                                 colCounts[colForSlot[slot]]);
    }

    layout.output1Center = outputCenter(box, 0);
    layout.output2Center = outputCenter(box, 1);
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

  void drawNode(NVGcontext* vg, Vec center, Vec size, int slot, const char* label) {
    Rect r = nodeRectFromCenter(center, size);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 2.5f);
    nvgFillColor(vg, nodeFill());
    nvgFill(vg);

    nvgStrokeWidth(vg, 0.9f);
    nvgStrokeColor(vg, nodeStroke(0.95f));
    nvgStroke(vg);

    if (slot >= 0) {
      char slotBuf[4];
      snprintf(slotBuf, sizeof(slotBuf), "%c:", slot + 'A');
      drawText(vg, Vec(r.pos.x + 3.5f, center.y - 3.5f), slotBuf, 6.2f, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
      drawText(vg, Vec(r.pos.x + 3.5f, center.y + 3.5f), label, 6.0f, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    }
    else {
      drawText(vg, r.getCenter(), label, 6.0f);
    }
  }

  void drawOutputs(NVGcontext* vg, const GraphLayout& layout) {
    const Vec outputSize = Vec(outputW, outputH);
    drawNode(vg, layout.output1Center, outputSize, -1, "OUT1");
    drawNode(vg, layout.output2Center, outputSize, -1, "OUT2");
  }

  void drawNodes(NVGcontext* vg, const GraphLayout& layout) {
    const Vec nodeSize = Vec(nodeW, nodeH);

    for (int slot = 0; slot < maxGraphNodes; ++slot) {
      const GraphRenderNode& node = snapshot->nodes[slot];

      if (!node.valid || node.kind == RenderNodeKind::Output) {
        continue;
      }
      drawNode(vg, layout.nodeCenters[slot], nodeSize, slot, getCardNameByHardwareId(node.hardwareId));
    }

    drawOutputs(vg, layout);
  }


  Vec anchorForSideAndPort(const Vec& nodeCenter, AnchorSide side, GraphPort port) {
    const Vec nodeSize = Vec(nodeW, nodeH);
    Vec anchorPoint;

    switch (side) {
    case AnchorSide::Left:
      anchorPoint = leftAnchor(nodeCenter, nodeSize);
      break;
    case AnchorSide::Right:
      anchorPoint = rightAnchor(nodeCenter, nodeSize, port);
      break;
    case AnchorSide::Top:
      anchorPoint = topAnchor(nodeCenter, nodeSize, port);
      break;
    case AnchorSide::Bottom:
      anchorPoint = bottomAnchor(nodeCenter, nodeSize, port);
      break;
    }
    return anchorPoint;
  }

  Vec anchorForSideAndTargetKind(const Vec& nodeCenter, AnchorSide side, RenderTargetKind targetKind) {
    const Vec nodeSize = Vec(nodeW, nodeH);
    const Vec outputSize = Vec(outputW, outputH);
    Vec anchorPoint;

    if (targetKind == RenderTargetKind::VoiceCard) {
      switch (side) {
      case AnchorSide::Left:
        anchorPoint = leftAnchor(nodeCenter, nodeSize);
        break;
      case AnchorSide::Right:
        anchorPoint = rightAnchor(nodeCenter, nodeSize);
        break;
      case AnchorSide::Top:
        anchorPoint = topAnchor(nodeCenter, nodeSize);
        break;
      case AnchorSide::Bottom:
        anchorPoint = bottomAnchor(nodeCenter, nodeSize);
        break;
      }
    }
    else {
      anchorPoint = leftAnchor(nodeCenter, outputSize);
    }

    return anchorPoint;
  }


  // contract:
  // toSlotNum and fromSlotNum are valid indices
  void drawEdges(NVGcontext* vg, const GraphLayout& layout) {

    for (size_t i = 0; i < snapshot->edgeCount; ++i) {
      const GraphRenderEdge& edge = snapshot->edges[i];

      if (!edge.valid) {
        continue;
      }

      // get the intent of the edge: which way is it going?
      Vec p1 = layout.nodeCenters[edge.fromSlotNum];
      Vec p2;
      switch (edge.targetKind) {
      case RenderTargetKind::Output1:
        p2 = layout.output1Center;
        break;
      case RenderTargetKind::Output2:
        p2 = layout.output2Center;
        break;
      case RenderTargetKind::VoiceCard:
        p2 = layout.nodeCenters[edge.toSlotNum];
        break;
      }

      EdgeRoute edgeRoute = chooseEdgeRoute(p1, p2,
                                            edge.fromSlotNum == edge.toSlotNum,
                                            box.size.y * 0.5f);

      p1 = anchorForSideAndPort(layout.nodeCenters[edge.fromSlotNum], edgeRoute.fromSide, edge.fromPort);
      switch (edge.targetKind) {
      case RenderTargetKind::VoiceCard:
        p2 = anchorForSideAndTargetKind(layout.nodeCenters[edge.toSlotNum], edgeRoute.toSide, edge.targetKind);
        break;
      case RenderTargetKind::Output1:
        p2 = anchorForSideAndTargetKind(layout.output1Center, edgeRoute.toSide, edge.targetKind);
        break;
      case RenderTargetKind::Output2:
        p2 = anchorForSideAndTargetKind(layout.output2Center, edgeRoute.toSide, edge.targetKind);
        break;
      }        

      if (edgeRoute.kind == EdgeRouteKind::Feedback) {
        float gutterY;
        float gutterOffset = 3.f + 2.5f * edge.fromSlotNum;

        if (edgeRoute.fromSide == edgeRoute.toSide) {
          gutterY = (edgeRoute.fromSide == AnchorSide::Top ?
                     (std::min(p1.y, p2.y) - gutterOffset) :
                     (std::max(p1.y, p2.y) + gutterOffset));
        }
        else {
          gutterY = (edgeRoute.fromSide == AnchorSide::Top ?
                     (std::min(p1.y, p2.y) + gutterOffset) :
                     (std::max(p1.y, p2.y) - gutterOffset));
        }

        nvgBeginPath(vg);
        nvgMoveTo(vg, p1.x, p1.y);
        nvgLineTo(vg, p1.x, gutterY);
        nvgLineTo(vg, p2.x, gutterY);
        nvgLineTo(vg, p2.x, p2.y);
        nvgStrokeColor(vg, displayTextColor(edge.weight));
        nvgStrokeWidth(vg, 1.2f);
        nvgStroke(vg);
      }
      else {
        nvgBeginPath(vg);
        nvgMoveTo(vg, p1.x, p1.y);
        nvgLineTo(vg, p2.x, p2.y);
        nvgStrokeColor(vg, displayTextColor(edge.weight));
        nvgStrokeWidth(vg, 1.2f);
        nvgStroke(vg);
      }
    }
  }


  void drawLayer(const DrawArgs& args, int layer) override {
    if (layer != 1 || !snapshot) {
      return;
    }

    GraphLayout layout;
    Rect localBox = Rect(Vec(0.f, 0.f), box.size);
    buildLayout(layout, localBox);

    drawNodes(args.vg, layout);
    drawEdges(args.vg, layout);
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
