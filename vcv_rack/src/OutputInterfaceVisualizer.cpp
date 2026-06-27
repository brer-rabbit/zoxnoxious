#include <array>
#include <algorithm>

#include "plugin.hpp"

#include "zcomponentlib.hpp"
#include "OutputInterface.hpp"

namespace zox {

// maxGraphNodes: number of voice cards + output node.
// maxGraphEdges: each voice card can have two inputs, then two final outputs with six each
static constexpr int8_t maxGraphNodes = maxVoiceCards;
static constexpr size_t maxGraphEdges = maxVoiceCards * 2 + 2 * 6;

static constexpr int graphRenderRateHz = 60;


enum class RenderNodeKind : uint8_t {
  VoiceCard,
  ExternalInput
};

enum class RenderTargetKind : uint8_t {
  VoiceCard,
  Output1,
  Output2
};

//======================================================
// Graph model
//======================================================

// slotNum not present here: the slotNum is the index in an array of these
struct GraphRenderNode {
  int64_t moduleId = -1; // for Rack-side presence only
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

  float weight = 1.f;
  bool valid = false;
};

struct GraphRenderSnapshot {
  GraphRenderNode nodes[maxGraphNodes];

  size_t edgeCount = 0;
  GraphRenderEdge edges[maxGraphEdges];
};


//======================================================
// Module Definition
//======================================================


struct OutputInterfaceVisualizer final : Module {
  enum ParamId {
    DISPLAY_PRIMARY_EDGE_PARAM,
    DISPLAY_SECONDARY_EDGE_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    INPUTS_LEN
  };
  enum OutputId {
    OUTPUTS_LEN
  };
  enum LightId {
    DISPLAY_PRIMARY_EDGE_LIGHT,
    DISPLAY_SECONDARY_EDGE_LIGHT,
    LIGHTS_LEN
  };

  dsp::ClockDivider clockDivider;

  GraphRenderSnapshot snapshotA;
  GraphRenderSnapshot snapshotB;
  std::atomic<GraphRenderSnapshot*> publishedSnapshot;
  GraphRenderSnapshot* writeSnapshot;

  bool displayPrimaryEdges = true;
  bool displaySecondaryEdges = true;

  void addSource(const ParticipantGraphInfo& participantInfo,
                 const GraphSource source,
                 const RenderTargetKind targetKind) {
    // fill in the edges of the graph
    if (writeSnapshot->edgeCount >= maxGraphEdges) {
      WARN("maxGraphEdges exceeded");
      return;
    }

    // this filters to only instantiated/rendered edges by policy
    if (source.valid && source.moduleId >= 0 &&
        source.slotNum >= 0 && source.slotNum < maxGraphNodes) {
      GraphRenderEdge& edge = writeSnapshot->edges[writeSnapshot->edgeCount++];
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
        writeSnapshot->nodes[info.slotNum].valid = true;
        writeSnapshot->nodes[info.slotNum].moduleId = info.moduleId;
        writeSnapshot->nodes[info.slotNum].hardwareId = info.hardwareId;
        writeSnapshot->nodes[info.slotNum].kind = kind;
        writeSnapshot->nodes[info.slotNum].output1Weight = info.output1Weight;
        writeSnapshot->nodes[info.slotNum].output2Weight = info.output2Weight;
      }
      else {
        writeSnapshot->nodes[info.slotNum] = GraphRenderNode{};
      }
    }
  }

  // iterate over all the edges:
  // find the "from" slot and incorporate the source weight via simple multiply
  void calculateEdgeWeights() {
    for (size_t i = 0; i < writeSnapshot->edgeCount; ++i) {
      const GraphRenderEdge& edge = writeSnapshot->edges[i];

      if (edge.valid) {
        const GraphRenderNode& fromNode = writeSnapshot->nodes[edge.fromSlotNum];

        if (fromNode.moduleId != -1 &&
            fromNode.hardwareId != invalidCardId) {
          writeSnapshot->edges[i].weight *= (edge.fromPort == GraphPort::OUT1 ?
                                            fromNode.output1Weight : fromNode.output2Weight);
        }
      }
    }
  }


  void buildGraphRenderSnapshot(const ParticipantGraphMessage& message,
                                GraphRenderSnapshot& graphSnapshot) {
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


  static constexpr int minClockDivision = 60;

  OutputInterfaceVisualizer() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configButton(DISPLAY_PRIMARY_EDGE_PARAM, "Show Primary Connections");
    configButton(DISPLAY_SECONDARY_EDGE_PARAM, "Show Secondary Connections");

    int calcClockDivision = std::max(minClockDivision,
                                     (static_cast<int>(APP->engine->getSampleRate()) / graphRenderRateHz));
    // division should really be frame rate-- this need not be frequent at all
    clockDivider.setDivision(calcClockDivision);

    snapshotA = {};
    snapshotB = {};
    publishedSnapshot.store(&snapshotA, std::memory_order_release);
    writeSnapshot = &snapshotB;

  }


  void onSampleRateChange(const SampleRateChangeEvent& e) override {
    int calcClockDivision = std::max(minClockDivision,
                                     (static_cast<int>(APP->engine->getSampleRate()) / graphRenderRateHz));
    clockDivider.setDivision(calcClockDivision);
  }


  static const char* graphPortName(GraphPort port) {
    return port == GraphPort::OUT1 ? "OUT1" : "OUT2";
  }

  static const char* renderNodeKindName(RenderNodeKind kind) {
    switch (kind) {
    case RenderNodeKind::VoiceCard:     return "VoiceCard";
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


# if 0
  // debug
  void dumpGraphRenderSnapshot() const {
    INFO("========== GraphRenderSnapshot ==========");
    INFO("Nodes:");
    for (size_t i = 0; i < maxGraphNodes; ++i) {
      const GraphRenderNode& node = writeSnapshot->nodes[i];

      if (!node.valid)
        continue;

      INFO("  slot=%zu moduleId=%lld hwId=%u kind=%s",
           i,
           (long long)node.moduleId,
           node.hardwareId,
           renderNodeKindName(node.kind));
    }

    INFO("Edges: count=%zu", writeSnapshot->edgeCount);

    for (size_t i = 0; i < writeSnapshot->edgeCount; ++i) {
      const GraphRenderEdge& edge = writeSnapshot->edges[i];

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
#endif


  void process(const ProcessArgs& args) override {
    if (clockDivider.process()) {
      if (leftExpander.module && leftExpander.module->model == modelOutputInterface) {
        ParticipantGraphMessage *message = static_cast<ParticipantGraphMessage*>(leftExpander.module->rightExpander.consumerMessage);

        displayPrimaryEdges = params[DISPLAY_PRIMARY_EDGE_PARAM].getValue();
        lights[DISPLAY_PRIMARY_EDGE_LIGHT].setBrightness(displayPrimaryEdges);

        displaySecondaryEdges = params[DISPLAY_SECONDARY_EDGE_PARAM].getValue();
        lights[DISPLAY_SECONDARY_EDGE_LIGHT].setBrightness(displaySecondaryEdges);

        if (message) {

          if (writeSnapshot == &snapshotA) {
            buildGraphRenderSnapshot(*message, snapshotA);
            publishedSnapshot.store(&snapshotA, std::memory_order_release);
            writeSnapshot = &snapshotB;
          }
          else {
            buildGraphRenderSnapshot(*message, snapshotB);
            publishedSnapshot.store(&snapshotB, std::memory_order_release);
            writeSnapshot = &snapshotA;
          }

          //dumpGraphRenderSnapshot();
        }
      }
    }
  }

};



//======================================================
// Layout
//======================================================

struct GraphLayout {
  Vec nodeCenters[maxGraphNodes];

  Vec output1Center;
  Vec output2Center;
};



static Rect nodeRectFromCenter(Vec center, Vec size) {
  return Rect(center.minus(size.div(2.f)), size);
}


//======================================================
// Graph Analysis
//======================================================


static int edgeTargetsOutput(const GraphRenderEdge& e) {
  int score = 0;
  if (e.valid) {
    if (e.targetKind == RenderTargetKind::Output1) {
      score += 1;
    }
    if (e.targetKind == RenderTargetKind::Output2) {
      score += 1;
    }
  }
  return score;
}

static int nodeFeedsOutput(const GraphRenderSnapshot& s, int8_t slot) {
  int totalStrength = 0;
  for (size_t i = 0; i < s.edgeCount; ++i) {
    const GraphRenderEdge& e = s.edges[i];
    if (e.valid && e.fromSlotNum == slot) {
      totalStrength += edgeTargetsOutput(e);
    }
  }
  return totalStrength;
}


struct GraphAnalysis {
  int distance = maxGraphNodes;   // lower is closer to output; maxGraphNodes value is "infinity"
  int outputStrength = 0;         // 0, 1, or 2
  int forwardEdgeCount = 0;       // edges to next column / downstream nodes
};

// a "forward edge" is from an node that feeds another internal node where the second
// node is closer to the output.  Most typically this would be a node that feeds the output.
// in a VCO --> VCF --> Output, analyzing the VCO, it has one forward edge.
static void computeForwardEdgeCounts(const GraphRenderSnapshot& s,
                                     GraphAnalysis analysis[maxGraphNodes]) {
  for (size_t i = 0; i < s.edgeCount; ++i) {
    const GraphRenderEdge& e = s.edges[i];

    if (!e.valid || e.targetKind != RenderTargetKind::VoiceCard) {
      continue;
    }

    const int srcDist = analysis[e.fromSlotNum].distance;
    const int dstDist = analysis[e.toSlotNum].distance;

    // A useful downstream edge moves closer to an output.
    if (dstDist < srcDist) {
      analysis[e.fromSlotNum].forwardEdgeCount++;
    }
  }
}


// Compute the minimum hop-distance from each slot to any output node.
// Distance 1 = directly feeds an output.
// Distance 2 = feeds a node that feeds an output.
// Distance (maxGraphNodes) = no path found (orphan/disconnected).
// Returns an array indexed by slot number.
static void computeGraphAnalysis(const GraphRenderSnapshot& s,
                                 GraphAnalysis distOut[maxGraphNodes]) {

  // pre-cond: distout[].distance initialized to maxGraphNodes (inifinity)
  // BFS frontier: start with nodes that directly feed an output
  std::vector<int> frontier;

  for (int slot = 0; slot < maxGraphNodes; ++slot) {
    if (!s.nodes[slot].valid) {
      continue;
    }

    distOut[slot].outputStrength = nodeFeedsOutput(s, slot);
    if (distOut[slot].outputStrength > 0) {
      distOut[slot].distance = 1;
      frontier.push_back(slot);
    }
  }

  // BFS: propagate distance backward through VoiceCard->VoiceCard edges
  while (!frontier.empty()) {
    std::vector<int> next;
    for (int target : frontier) {
      // find any node that feeds 'target' via a VoiceCard edge
      for (size_t i = 0; i < s.edgeCount; ++i) {
        const GraphRenderEdge& e = s.edges[i];
        if (!e.valid) continue;
        if (e.targetKind != RenderTargetKind::VoiceCard) continue;
        if (e.toSlotNum != target) continue;
        int src = e.fromSlotNum;
        if (src < 0 || src >= maxGraphNodes) continue;
        int candidate = distOut[target].distance + 1;
        if (candidate < distOut[src].distance) {
          distOut[src].distance = candidate;
          next.push_back(src);
        }
      }
    }
    frontier = std::move(next);
  }

  computeForwardEdgeCounts(s, distOut);
}


// columnPriority is used when a column overflows to establish a score
// of which nodes stay in the column and which get booted.
static int columnPriority(const GraphAnalysis& a) {
  int score = 0;
  // Strongest reason to stay near the output.
  score += a.outputStrength * 100;
  // Closer to output is better.
  score += (maxGraphNodes - a.distance) * 10;
  // Node bridges into output-feeding nodes.
  score += a.forwardEdgeCount * 5;

  return score;
}


using SlotArray = std::array<int, maxGraphNodes>;
static constexpr int kNumColumns = 3;

static void resolveSingleColumnOverflow(SlotArray& colForSlot,
                                        const GraphAnalysis analysis[maxGraphNodes],
                                        int overflowCol) {
  if (overflowCol < 0) {
    return;
  }

  std::array<int, maxGraphNodes> overflowSlots;
  int overflowCount = 0;

  for (int slot = 0; slot < maxGraphNodes; ++slot) {
    if (colForSlot[slot] == overflowCol) {
      overflowSlots[overflowCount++] = slot;
    }
  }

  if (overflowCount <= 3) {
    return;
  }

  std::stable_sort(overflowSlots.begin(),
                   overflowSlots.begin() + overflowCount,
                   [&](int a, int b) {
                     return columnPriority(analysis[a]) > columnPriority(analysis[b]);
                   });

  int colCounts[kNumColumns] = {};
  for (int slot = 0; slot < maxGraphNodes; ++slot) {
    int col = colForSlot[slot];
    if (col >= 0 && col < kNumColumns) {
      colCounts[col]++;
    }
  }

  // where to dump the overflowed nodes.
  // If column 2 overflows move left
  // If column 0 overflows move right
  // If column 1 overflows, either side will have space
  for (int i = 3; i < overflowCount; ++i) {
    int slot = overflowSlots[i];

    int targetCol = -1;
    if (overflowCol > 0 && colCounts[overflowCol - 1] < 3) {
      targetCol = overflowCol - 1;
    }
    else if (overflowCol < kNumColumns - 1 && colCounts[overflowCol + 1] < 3) {
      targetCol = overflowCol + 1;
    }

    if (targetCol >= 0) {
      colForSlot[slot] = targetCol;
      colCounts[overflowCol]--;
      colCounts[targetCol]++;
    }
  }
}



// Map hop-distance to a 0-2 column index.
// distance 1 --> col 2 (closest to output)
// distance 2 --> col 1
// distance 3+ (or unreachable) --> col 0
static int distanceToColumn(int dist) {
  if (dist == 1) return 2;
  if (dist == 2) return 1;
  return 0;
}


// place the node on per the col/row.  Assume a 3x3 matrix.
static Vec voiceNodeCenter(const Rect& r, int col, int row) {
  const float leftMargin = 5.f;
  const float rightOutputGutter = 30.f;
  const float marginY = 28.f;

  const float usableW = r.size.x - leftMargin - rightOutputGutter;
  const float usableH = r.size.y - 2.f * marginY;
  const float colW = usableW / 3.f;

  const float y = r.pos.y + marginY + (usableH / 2.f) * row;

  return Vec(r.pos.x + leftMargin + colW * (col + 0.5f), y);
}


static Vec outputCenter(const Rect& r, int outputIndex) {
  const float rightMargin = 14.f;
  const float marginY = 48.f;

  float y = outputIndex == 0
    ? r.pos.y + marginY
    : r.pos.y + r.size.y - marginY;

  return Vec(r.pos.x + r.size.x - rightMargin, y);
}


static constexpr float kPortAnchorOffset = 4.f;
static float portOffset(GraphPort port) {
  return port == GraphPort::OUT1 ? -kPortAnchorOffset : kPortAnchorOffset;
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
  return Vec(center.x + portOffset(port), center.y - size.y * 0.5f);
}

static Vec bottomAnchor(Vec center, Vec size) {
  return Vec(center.x - 8.f, center.y + size.y * 0.5f);
}

static Vec bottomAnchor(Vec center, Vec size, GraphPort port) {
  return Vec(center.x + portOffset(port), center.y + size.y * 0.5f);
}


enum class AnchorSide : uint8_t { Left, Right, Top, Bottom };
enum class EdgeRouteKind : uint8_t { Forward, Feedback, SelfLoop };

struct EdgeRoute {
  EdgeRouteKind kind = EdgeRouteKind::Forward;
  AnchorSide fromSide = AnchorSide::Right;
  AnchorSide toSide = AnchorSide::Left;
};


static EdgeRoute chooseEdgeRoute(Vec fromCenter, Vec toCenter, bool selfLoop) {
  EdgeRoute route;

  route.toSide = AnchorSide::Left;

  if (selfLoop) {
    route.kind = EdgeRouteKind::SelfLoop;
    route.fromSide = AnchorSide::Bottom;
    return route;
  }

  route.fromSide = AnchorSide::Right;
  const float dx = toCenter.x - fromCenter.x;

  if (dx > 0.f) {
    route.kind = EdgeRouteKind::Forward;
  }
  else {
    route.kind = EdgeRouteKind::Feedback;
  }

  return route;
}

//----------------
// Row placement
//----------------

static constexpr float kOutput1Row = 0.5f;
static constexpr float kOutput2Row = 1.5f;

static float barycenterForSlot(int slot,
                               const SlotArray& colForSlot,
                               const SlotArray& rowForSlot,
                               const GraphRenderSnapshot& s) {
  const int myCol = colForSlot[slot];

  float total = 0.f;
  int count = 0;

  for (size_t i = 0; i < s.edgeCount; ++i) {
    const GraphRenderEdge& e = s.edges[i];

    if (!e.valid || e.fromSlotNum != slot) {
      continue;
    }

    if (e.targetKind == RenderTargetKind::Output1) {
      total += kOutput1Row;
      count++;
    }
    else if (e.targetKind == RenderTargetKind::Output2) {
      total += kOutput2Row;
      count++;
    }
    else if (e.targetKind == RenderTargetKind::VoiceCard &&
             e.toSlotNum >= 0 &&
             e.toSlotNum < maxGraphNodes &&
             colForSlot[e.toSlotNum] == myCol + 1 &&
             rowForSlot[e.toSlotNum] >= 0) {
      total += rowForSlot[e.toSlotNum];
      count++;
    }
  }

  return count > 0 ? total / count : 1.f;
}


static void assignRowsByBarycenter(const SlotArray& colForSlot,
                                   SlotArray& rowForSlot,
                                   const GraphRenderSnapshot& s) {
  for (int col = 2; col >= 0; --col) {
    std::array<int, 3> slots;
    int count = 0;

    for (int slot = 0; slot < maxGraphNodes; ++slot) {
      if (colForSlot[slot] == col && count < 3) {
        slots[count++] = slot;
      }
    }

    if (count == 0) {
      continue;
    }

    float score[3] = {};
    for (int i = 0; i < count; ++i) {
      score[i] = barycenterForSlot(slots[i], colForSlot, rowForSlot, s);
    }

    // run through all permutations.  For a 3x3 with 6 nodes this isn't much.
    // only first two elements are used for the count=2 use case
    static const int rowPerms[6][3] = {
      {0, 1, 2},
      {0, 2, 1},
      {1, 0, 2},
      {1, 2, 0},
      {2, 0, 1},
      {2, 1, 0}
    };

    float bestCost = 1e9f;
    int bestRows[3] = {1, 1, 1};

    const int permCount = (count == 1) ? 3 : (count == 2 ? 6 : 6);

    for (int p = 0; p < permCount; ++p) {
      float cost = 0.f;

      for (int i = 0; i < count; ++i) {
        int candidateRow;

        if (count == 1) {
          candidateRow = p; // rows 0,1,2
        }
        else {
          candidateRow = rowPerms[p][i];
        }

        // calculate distance of how far it is pulled from ideal position
        cost += std::fabs(candidateRow - score[i]);
      }

      if (cost < bestCost) {
        bestCost = cost;
        for (int i = 0; i < count; ++i) {
          bestRows[i] = (count == 1) ? p : rowPerms[p][i];
        }
      }
    }

    for (int i = 0; i < count; ++i) {
      rowForSlot[slots[i]] = bestRows[i];
    }
  }
}




struct SystemRoutingVisualizerDisplay : LedDisplay {
  const GraphRenderSnapshot *snapshot = nullptr;
  const OutputInterfaceVisualizer *module = nullptr;

  static constexpr float nodeW = 36.f;
  static constexpr float nodeH = 18.f;
  static constexpr float outputW = 20.f;
  static constexpr float outputH = 14.f;

  // create a dynamic vertical stack for each column.
  // colForSlot is a lookup for which column index was assigned to a node.
  // rowForSlot is a lookup for the row the slot is assigned.
  // colCounts tracks how many nodes have been assigned to each column.
  // Two passes:
  // (1) BFS distance from output drives each node's preferred column
  void buildLayout(GraphLayout& layout, const Rect& box) {
    layout = GraphLayout{};

    SlotArray colForSlot;
    SlotArray rowForSlot;
    int colCounts[kNumColumns] = {};

    colForSlot.fill(-1);
    rowForSlot.fill(-1);

    // Compute BFS distances from outputs for all slots
    GraphAnalysis distToOutput[maxGraphNodes];
    computeGraphAnalysis(*snapshot, distToOutput);

    // given 6 nodes in a 3x3, it's only possible for one column to overflow.  Track
    // that here.  A value of -1 indicates no overflow.
    int overflowCol = -1;

    for (int slot = 0; slot < maxGraphNodes; ++slot) {
      const GraphRenderNode& node = snapshot->nodes[slot];

      if (!node.valid) {
        continue;
      }

      // distance-to-output drives column: dist1-->col2, dist2-->col1, else-->col0
      int col = distanceToColumn(distToOutput[slot].distance);
      colForSlot[slot] = col;
      colCounts[col]++;
      if (colCounts[col] > 3) {
        overflowCol = col;
      }

      // row assigned here for now
    }

    resolveSingleColumnOverflow(colForSlot, distToOutput, overflowCol);
    // rebuild colCounts after resolving overflow
    std::fill(std::begin(colCounts), std::end(colCounts), 0);
    for (int slot = 0; slot < maxGraphNodes; ++slot) {
      int col = colForSlot[slot];
      if (col >= 0 && col < kNumColumns) {
        colCounts[col]++;
      }
    }


    //sortRowsByNextColumnTargets(colForSlot, rowForSlot, *snapshot);
    assignRowsByBarycenter(colForSlot, rowForSlot, *snapshot);

    for (int slot = 0; slot < maxGraphNodes; ++slot) {
      if (colForSlot[slot] < 0) {
        continue;
      }

      // todo: play with this for initial node placement by quantity
      layout.nodeCenters[slot] = voiceNodeCenter(box,
                                                 colForSlot[slot],
                                                 rowForSlot[slot]);
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

  NVGcolor displayRouteColor(float a = 1.f) const {
    return nvgRGBAf(0.83f, 0.92f, 0.99f, a);
  }


  float edgeAlpha(float weight) {
    return clamp(0.30f + 0.70f * weight, 0.3f, 1.f);
  }

  float edgeWidth(float weight) {
    return clamp(0.8f + 0.8f * weight, 0.8f, 1.6f);
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

  // Draw a filled arrowhead at 'tip' pointing in the direction from 'tail' toward 'tip'.
  // halfBase and height control the triangle size.
  void drawArrowhead(NVGcontext* vg, Vec tail, Vec tip,
                     float halfBase = 3.5f, float height = 6.f,
                     NVGcolor color = nvgRGB(0,0,0)) {
    float dx = tip.x - tail.x;
    float dy = tip.y - tail.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;

    // unit vector along edge direction
    float ux = dx / len;
    float uy = dy / len;
    // perpendicular
    float px = -uy;
    float py =  ux;

    // base centre: step back 'height' from tip
    Vec base = Vec(tip.x - ux * height, tip.y - uy * height);

    Vec left  = Vec(base.x + px * halfBase, base.y + py * halfBase);
    Vec right = Vec(base.x - px * halfBase, base.y - py * halfBase);

    nvgBeginPath(vg);
    nvgMoveTo(vg, tip.x, tip.y);
    nvgLineTo(vg, left.x, left.y);
    nvgLineTo(vg, right.x, right.y);
    nvgClosePath(vg);
    nvgFillColor(vg, color);
    nvgFill(vg);
  }


  static constexpr float kNodeLabelOffsetY = 3.5f;
  static constexpr float kNodeLabelMarginX = 3.5f;
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
      drawText(vg, Vec(r.pos.x + kNodeLabelMarginX, center.y - kNodeLabelOffsetY), slotBuf,
               6.0f, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
      drawText(vg, Vec(r.pos.x + kNodeLabelMarginX, center.y + kNodeLabelOffsetY), label,
               6.0f, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
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

      if (!node.valid) {
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


  void drawForwardEdge(NVGcontext* vg, Vec from, Vec to,
                       const GraphRenderEdge& edge, NVGcolor edgeColor) {
    nvgBeginPath(vg);
    Vec midPoint = from;
    midPoint.x += (edge.fromPort == GraphPort::OUT1 ? 2.f : 4.f) + 2.f * edge.fromSlotNum;
    midPoint.y = to.y;
    nvgMoveTo(vg, from.x, from.y);
    nvgLineTo(vg, midPoint.x, from.y);
    nvgLineTo(vg, midPoint.x, to.y);
    nvgLineTo(vg, to.x, to.y);
    nvgStrokeColor(vg, edgeColor);
    nvgStrokeWidth(vg, edgeWidth(edge.weight));
    nvgStroke(vg);
    drawArrowhead(vg, midPoint, to, 2.f + 2.f * edge.weight, 5.f, edgeColor);
  }


  void drawFeedbackEdge(NVGcontext* vg, Vec from, Vec to,
                       const GraphRenderEdge& edge, NVGcolor edgeColor) {
    Vec leftGutter;
    Vec rightGutter;
    // stagger feedback gutters by slot number so parallel feedback paths
    // don't overlap.  Gutter starts a 3px growing by 2.5 per slot.
    float verticalGutterOffset = nodeH + 2.5f * edge.fromSlotNum;

    if (to.y < from.y) {
      // gutter goes above
      // forward amount determined by fromPort
      rightGutter.x = from.x + (edge.fromPort == GraphPort::OUT1 ? 2.f : 4.f) + 1.f * edge.fromSlotNum;
      rightGutter.y = from.y - verticalGutterOffset;
      leftGutter.x = to.x - (4.f + edge.fromSlotNum);
      leftGutter.y = to.y;
    }
    else {
      // gutter goes below
      // forward amount determined by fromPort
      rightGutter.x = from.x + (edge.fromPort == GraphPort::OUT1 ? 2.f : 4.f) + 1.f * edge.fromSlotNum;
      rightGutter.y = from.y + verticalGutterOffset;
      leftGutter.x = to.x - (4.f + edge.fromSlotNum);
      leftGutter.y = to.y;
    }

    nvgBeginPath(vg);
    nvgMoveTo(vg, from.x, from.y);
    nvgLineTo(vg, rightGutter.x, from.y);
    nvgLineTo(vg, rightGutter.x, rightGutter.y);
    nvgLineTo(vg, leftGutter.x, rightGutter.y);
    nvgLineTo(vg, leftGutter.x, to.y);
    nvgLineTo(vg, to.x, to.y);
    nvgStrokeColor(vg, edgeColor);
    nvgStrokeWidth(vg, edgeWidth(edge.weight));
    nvgStroke(vg);

    drawArrowhead(vg, leftGutter, to, 2.f + 2.f * edge.weight, 5.f, edgeColor);
  }


  void drawSelfLoopEdge(NVGcontext* vg, Vec from, Vec to,
                        const GraphRenderEdge& edge, NVGcolor edgeColor) {
    nvgBeginPath(vg);
    Vec farPoint;
    farPoint.x = to.x - 6.f;
    farPoint.y = from.y + 4.f;
    nvgMoveTo(vg, from.x, from.y);
    nvgLineTo(vg, from.x, farPoint.y);
    nvgLineTo(vg, farPoint.x, farPoint.y);
    nvgLineTo(vg, farPoint.x, to.y);
    nvgLineTo(vg, to.x, to.y);
    nvgStrokeColor(vg, edgeColor);
    nvgStrokeWidth(vg, edgeWidth(edge.weight));
    nvgStroke(vg);
    Vec arrowTail = to;
    arrowTail.x -= 6.f;
    drawArrowhead(vg, arrowTail, to, 2.f + 2.f * edge.weight, 5.f, edgeColor);
  }


  // contract:
  // toSlotNum and fromSlotNum are valid indices
  // module is a valid pointer
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
                                            edge.fromSlotNum == edge.toSlotNum);

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

      NVGcolor edgeColor = displayRouteColor(edgeAlpha(edge.weight));

      if (edgeRoute.kind == EdgeRouteKind::Forward && module->displayPrimaryEdges) {
        drawForwardEdge(vg, p1, p2, edge, edgeColor);
      }
      else if (edgeRoute.kind == EdgeRouteKind::Feedback && module->displaySecondaryEdges) {
        drawFeedbackEdge(vg, p1, p2, edge, edgeColor);
      }
      else if (edgeRoute.kind == EdgeRouteKind::SelfLoop && module->displaySecondaryEdges) {
        drawSelfLoopEdge(vg, p1, p2, edge, edgeColor);
      }
    }
  }


  void drawLayer(const DrawArgs& args, int layer) override {
    if (!module) {
      return;
    }

    snapshot = module->publishedSnapshot.load(std::memory_order_acquire);

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
      display->module = module;
      addChild(display);
    }

    addParam(createLightParamCentered<ZPushButtonMediumStatefulLightLatch<SmallSimpleLight<ZoxAmberLight>>>(mm2px(Vec(12.5, 88.272)), module, OutputInterfaceVisualizer::DISPLAY_PRIMARY_EDGE_PARAM, OutputInterfaceVisualizer::DISPLAY_PRIMARY_EDGE_LIGHT));
    addParam(createLightParamCentered<ZPushButtonMediumStatefulLightLatch<SmallSimpleLight<ZoxAmberLight>>>(mm2px(Vec(12.5, 98.272)), module, OutputInterfaceVisualizer::DISPLAY_SECONDARY_EDGE_PARAM, OutputInterfaceVisualizer::DISPLAY_SECONDARY_EDGE_LIGHT));
  }
};


} // namespace zox

Model* modelOutputInterfaceVisualizer = createModel<zox::OutputInterfaceVisualizer, zox::OutputInterfaceVisualizerWidget>("OutputInterfaceVisualizer");
