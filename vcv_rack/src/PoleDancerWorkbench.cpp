#include <cmath>
#include <complex>
#include "plugin.hpp"
#include "modulehelpers.hpp"
#include "zcomponentlib.hpp"

namespace zox {

constexpr float MIN_DB = -36.f;
constexpr float MAX_DB = 24.f;
constexpr int POINTS = 256;

// utility for range of drawing the scope
struct ScopeRange {
  float minW = 0.01f;
  float maxW = 100.f;
};

struct PoleMixCoefficients {
  float weight[5] = {0.f, 0.f, 0.f, 0.f, 1.f};
  float fb[4] = {};
};


static constexpr float POLEMIX_VOLTAGE_ANALYZER = 0.8f; // 10V --> scale for 8X
static float poleMixParamToCoeff(float v) {
  return v * POLEMIX_VOLTAGE_ANALYZER;
}

static constexpr float RESONANCE_VOLTAGE_SCALE = 4.f; // 2.5V == coefficient 1.0
static float resonanceParamToCoeff(float v) {
  return v * RESONANCE_VOLTAGE_SCALE;
}


static constexpr float sign[5] = {+1, -1, +1, -1, +1};

class PoleMixResponseModel {
public:
  PoleMixCoefficients coeffs;

  std::complex<float> eval(float w) const {
    std::complex<float> s(0.f, w);

    // pTerms = { p^4, p^3, p^2, p^1, p^0 }
    std::complex<float> pTerms[5];
    pTerms[3] = s + 1.f; 
    pTerms[2] = pTerms[3] * pTerms[3];
    pTerms[1] = pTerms[2] * pTerms[3];
    pTerms[0] = pTerms[2] * pTerms[2];
    pTerms[4] = 1.f;

    std::complex<float> numerator = 0.f;
    for (int i = 0; i < 5; ++i) {
      numerator += sign[i] * coeffs.weight[i] * pTerms[i];
    }

    std::complex<float> denominator = pTerms[0];
    for (int i = 0; i < 4; ++i) {
      denominator += coeffs.fb[i] * pTerms[i + 1];
    }

    return std::abs(denominator) < 1e-9f
      ? std::complex<float>(0.f, 0.f)
      : numerator / denominator;
  }

};



// this allows template-izing for PoleDancer and PoleDancerPersonality.  This way, each
// can only connect to their type of expander.
struct WorkbenchForPoleDancer {
  static Model* leftModel() {
    return modelPoleDancer;
  }

  static bool hasControls() {
    return false;
  }
};

struct WorkbenchForPersonality {
  static Model* leftModel() {
    return modelPoleDancerPersonality;
  }

  static bool hasControls() {
    return true;
  }
};


template <typename Policy>
struct PoleDancerWorkbench : Module {
  enum ParamId {
    DRY_MIX_PARAM,
    POLE1_MIX_PARAM,
    POLE2_MIX_PARAM,
    POLE3_MIX_PARAM,
    POLE4_MIX_PARAM,
    RESONANCE_P1_PARAM,
    RESONANCE_P2_PARAM,
    RESONANCE_P3_PARAM,
    RESONANCE_P4_PARAM,
    SCOPE_SCALE_PARAM,
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
  PoleMixCoefficients poleMixCoefs;
  ScopeRange scopeRange;

  PersonalityMessage expanderMessages[2] = {};

  PoleDancerWorkbench() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(DRY_MIX_PARAM, 0.f, 10.f, 0.f, "Dry Mix", "%", 0.f, 10.f);
    configParam(POLE1_MIX_PARAM, 0.f, 10.f, 0.f, "Pole 1 Mix", "%", 0.f, 10.f);
    configParam(POLE2_MIX_PARAM, 0.f, 10.f, 0.f, "Pole 2 Mix", "%", 0.f, 10.f);
    configParam(POLE3_MIX_PARAM, 0.f, 10.f, 0.f, "Pole 3 Mix", "%", 0.f, 10.f);
    configParam(POLE4_MIX_PARAM, 0.f, 10.f, 10.f, "Pole 4 Mix", "%", 0.f, 10.f);
    configParam(RESONANCE_P1_PARAM, 0.f, 10.f, 10.f, "Pole 4 Mix", "%", 0.f, 10.f);
    configParam(RESONANCE_P2_PARAM, 0.f, 10.f, 10.f, "Pole 4 Mix", "%", 0.f, 10.f);
    configParam(RESONANCE_P3_PARAM, 0.f, 10.f, 10.f, "Pole 4 Mix", "%", 0.f, 10.f);
    configParam(RESONANCE_P4_PARAM, 0.f, 10.f, 10.f, "Pole 4 Mix", "%", 0.f, 10.f);
    configSwitch(SCOPE_SCALE_PARAM, 0.f, 2.f, 1.f, "Scope Scale", {"Wide", "Normal", "Narrow"});
    clockDivider.setDivision(512);
    leftExpander.producerMessage = &expanderMessages[0];
    leftExpander.consumerMessage = &expanderMessages[1];
  }


  void process(const ProcessArgs& args) override {

    if (clockDivider.process()) {

      if (leftPresent()) {
        // Read from Personality
        PersonalityMessage* fromPersonality = static_cast<PersonalityMessage*>(leftExpander.consumerMessage);
        if (fromPersonality->leftAuthoritative) {
          for (int i = 0; i < 5; i++) {
            params[DRY_MIX_PARAM + i].setValue(fromPersonality->values[i]);
          }
          for (int i = 0; i < 4; ++i) {
            params[RESONANCE_P1_PARAM + i].setValue(fromPersonality->resonance[i]);
          }
        }

      for (int i = 0; i < 5; ++i) {
        poleMixCoefs.weight[i] = poleMixParamToCoeff(params[DRY_MIX_PARAM + i].getValue());
      }
      for (int i = 0; i < 4; ++i) {
        poleMixCoefs.fb[i] = resonanceParamToCoeff(params[RESONANCE_P1_PARAM + i].getValue());
      }

        // Write to Left.  Resonance is not written.
        PersonalityMessage* toPersonality = static_cast<PersonalityMessage*>(leftExpander.module->rightExpander.producerMessage);
        toPersonality->leftAuthoritative = false;  // Right never claims authority
        for (int i = 0; i < 5; i++) {
          toPersonality->values[i] = params[DRY_MIX_PARAM + i].getValue();
        }
        leftExpander.module->rightExpander.messageFlipRequested = true;
      }

    }

    switch (static_cast<int>(std::round(params[SCOPE_SCALE_PARAM].getValue()))) {
    case 2:
      scopeRange.minW = 0.1;
      scopeRange.maxW = 10.f;
      break;
    case 1:
      scopeRange.minW = 0.01;
      scopeRange.maxW = 100.f;
      break;
    default:
      scopeRange.minW = 0.001;
      scopeRange.maxW = 1000.f;
      break;
    }

  }


  // policy method for template
  bool leftPresent() const {
    return leftExpander.module && leftExpander.module->model == Policy::leftModel();
  }
};



static bool different(float a, float b) {
    return std::fabs(a - b) > 0.001f;
}

static bool coeffsChanged(const PoleMixCoefficients& x, const PoleMixCoefficients& y) {
    return different(x.weight[0], y.weight[0])
      || different(x.weight[1], y.weight[1])
      || different(x.weight[2], y.weight[2])
      || different(x.weight[3], y.weight[3])
      || different(x.weight[4], y.weight[4])
      || different(x.fb[0], y.fb[0])
      || different(x.fb[1], y.fb[1])
      || different(x.fb[2], y.fb[2])
      || different(x.fb[3], y.fb[3]);
}


float wToX(float w, const Rect& r, const ScopeRange& range) {
  float t = std::log(w / range.minW) / std::log(range.maxW / range.minW);
  return r.pos.x + t * r.size.x;
}

float dbToY(float db, const Rect& r) {

  float yNorm = (db - MIN_DB) / (MAX_DB - MIN_DB);
  yNorm = clamp(yNorm, 0.f, 1.f);

  return r.pos.y + (1.f - yNorm) * r.size.y;
}

bool dbToVisibleY(float db, const Rect& r, float* y) {
  if (db < MIN_DB || db > MAX_DB) {
    return false;
  }
  float yNorm = (db - MIN_DB) / (MAX_DB - MIN_DB);
  *y = r.pos.y + (1.f - yNorm) * r.size.y;
  return true;
}


template <typename Policy>
struct PoleDancerWorkbenchDisplay : LedDisplay {
  using ModuleType = PoleDancerWorkbench<Policy>;
  ModuleType* module = nullptr;
  PoleMixCoefficients lastCoeffs;
  std::vector<Vec> magPoints;
  std::vector<Vec> phasePoints;
  ScopeRange lastRange;

  PoleDancerWorkbenchDisplay() {
    magPoints.reserve(POINTS);
    phasePoints.reserve(POINTS);
  }

  void rebuildMagnitudePoints(const PoleMixCoefficients& c, const Rect& r, const ScopeRange& range) {
    magPoints.clear();

    PoleMixResponseModel model;
    model.coeffs = c;

    //Rect r = box.zeroPos().shrink(Vec(4, 4));

    for (int i = 0; i < POINTS; ++i) {
      float t = float(i) / float(POINTS - 1);
      float w = range.minW * std::pow(range.maxW / range.minW, t);

      auto h = model.eval(w);
      float mag = std::max(std::abs(h), 1e-9f);
      float db = 20.f * std::log10(mag);

      float x = r.pos.x + t * r.size.x;

      float y;
      if (dbToVisibleY(db, r, &y)) {
        magPoints.push_back(Vec(x, y));
      }
      else {
        magPoints.push_back(Vec(NAN, NAN));
      }
    }
  }


  void rebuildPhasePoints(const PoleMixCoefficients& c, const Rect& r, const ScopeRange& range) {
    phasePoints.clear();
    PoleMixResponseModel model;
    model.coeffs = c;

    for (int i = 0; i < POINTS; ++i) {
      float t = float(i) / float(POINTS - 1);
      float w = range.minW * std::pow(range.maxW / range.minW, t);
      auto h = model.eval(w);
        
      float phase = std::arg(h); // Value in (-π, π]
        
      // Map -π to +π across the vertical height of phaseRect
      float yNorm = (phase - (-M_PI)) / (2.f * M_PI);
      float x = r.pos.x + t * r.size.x;
      float y = r.pos.y + (1.f - yNorm) * r.size.y;
        
      phasePoints.push_back(Vec(x, y));
    }
  }


  void drawCurve(NVGcontext* vg, const std::vector<Vec>& points, NVGcolor color, float wrapThreshold = INFINITY) {
    bool pathOpen = false;
    nvgBeginPath(vg);

    for (size_t i = 0; i < points.size(); ++i) {
      const Vec& p = points[i];
      if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
        pathOpen = false;
        continue;
      }

      // Check for phase wrapping jumps
      if (pathOpen && i > 0 && std::isfinite(wrapThreshold)) {
        float diffY = std::abs(p.y - points[i-1].y);
        if (diffY > wrapThreshold) { // Arbitrary threshold for a jump
          nvgStrokeWidth(vg, 1.5f);
          nvgStrokeColor(vg, color);
          nvgStroke(vg); // Finish current segment
                
          nvgBeginPath(vg);
          nvgMoveTo(vg, p.x, p.y);
          continue;
        }
      }

      if (!pathOpen) {
        nvgMoveTo(vg, p.x, p.y);
        pathOpen = true;
      } else {
        nvgLineTo(vg, p.x, p.y);
      }
    }
    nvgStrokeWidth(vg, 1.5f);
    nvgStrokeColor(vg, color);
    nvgStroke(vg);
  }

  void drawVerticalGridLine(NVGcontext* vg, const Rect& r, float w, NVGcolor color, float width, const ScopeRange& range) {

    if (w < range.minW || w > range.maxW)
      return;

    float x = wToX(w, r, range);

    nvgBeginPath(vg);
    nvgMoveTo(vg, x, r.pos.y);
    nvgLineTo(vg, x, r.pos.y + r.size.y);
    nvgStrokeWidth(vg, width);
    nvgStrokeColor(vg, color);
    nvgStroke(vg);
  }


  void drawGrid(NVGcontext* vg, const Rect& magRect, const Rect& phaseRect, const ScopeRange& range) {
    // 0 dB line only in magnitude plot
    float y0 = dbToY(0.f, magRect);

    nvgBeginPath(vg);
    nvgMoveTo(vg, magRect.pos.x, y0);
    nvgLineTo(vg, magRect.pos.x + magRect.size.x, y0);
    nvgStrokeWidth(vg, 1.0f);
    nvgStrokeColor(vg, nvgRGBA(180, 180, 180, 80));
    nvgStroke(vg);

    // frequency grid lines, broken across both plots
    drawVerticalGridLine(vg, magRect, 0.5f, nvgRGBA(180, 180, 180, 100), 1.0f, range);
    drawVerticalGridLine(vg, magRect, 1.0f, nvgRGBA(180, 180, 180, 150), 1.0f, range);
    drawVerticalGridLine(vg, magRect, 2.0f, nvgRGBA(180, 180, 180, 100), 1.0f, range);

    drawVerticalGridLine(vg, phaseRect, 0.5f, nvgRGBA(180, 180, 180, 100), 1.0f, range);
    drawVerticalGridLine(vg, phaseRect, 1.0f, nvgRGBA(180, 180, 180, 150), 1.0f, range);
    drawVerticalGridLine(vg, phaseRect, 2.0f, nvgRGBA(180, 180, 180, 100), 1.0f, range);
  }

  void drawLabel(NVGcontext* vg, const char* text, const Rect& r) {
    nvgFontSize(vg, 8.f);
    //nvgFillColor(vg, nvgRGBA(200, 200, 200, 120));
    nvgFillColor(vg, nvgRGBA(240, 240, 240, 180));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(vg, r.pos.x + 2.f, r.pos.y + 2.f, text, NULL);
  }


  void drawLayer(const DrawArgs& args, int layer) override {
    if (layer == 1) {
      PoleMixCoefficients c = module ? module->poleMixCoefs : PoleMixCoefficients{};
      ScopeRange range = module ? module->scopeRange : ScopeRange{};

      // Split the box: 60% for Magnitude, 40% for Phase
      float splitY = box.size.y * 0.6f;
      Rect magRect = Rect(Vec(0, 0), Vec(box.size.x, splitY)).shrink(Vec(4, 4));
      Rect phaseRect = Rect(Vec(0, splitY), Vec(box.size.x, box.size.y - splitY)).shrink(Vec(4, 4));

      if (coeffsChanged(c, lastCoeffs) ||
          different(range.minW, lastRange.minW)) { // assume minW will change if window changes
        lastCoeffs = c;
        lastRange = range;
        rebuildMagnitudePoints(c, magRect, range);
        rebuildPhasePoints(c, phaseRect, range);
      }

      drawLabel(args.vg, "MAG", magRect);
      drawLabel(args.vg, "PHASE", phaseRect);
      drawGrid(args.vg, magRect, phaseRect, range);
      drawCurve(args.vg, magPoints, nvgRGB(0xff, 0x9a, 0x35));
      drawCurve(args.vg, phasePoints, nvgRGB(80, 180, 255), phaseRect.size.y * 0.4f); // Blue Phase
    }
    LedDisplay::drawLayer(args, layer);
  }

};


template <typename Policy>
struct PoleDancerWorkbenchWidget : ModuleWidget {
  using ModuleType = PoleDancerWorkbench<Policy>;

  PoleDancerWorkbenchWidget(ModuleType* module) {
    setModule(module);
    if (Policy::hasControls()) {
        setPanel(createPanel(asset::plugin(pluginInstance, "res/PoleDancerPersonalityWorkbench.svg")));
    }
    else {
      setPanel(createPanel(asset::plugin(pluginInstance, "res/PoleDancerWorkbench.svg")));
    }

    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH / 2, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    auto* display = createWidget<PoleDancerWorkbenchDisplay<Policy> >(mm2px(Vec(5.5, 15.0)));
    display->box.size = mm2px(Vec(60.0, 60.0));
    display->module = module;
    addChild(display);

    if (Policy::hasControls()) {
      addParam(createParamCentered<Trimpot>(mm2px(Vec(13.323, 89.323)), module, ModuleType::DRY_MIX_PARAM));
      addParam(createParamCentered<Trimpot>(mm2px(Vec(24.448, 89.323)), module, ModuleType::POLE1_MIX_PARAM));
      addParam(createParamCentered<Trimpot>(mm2px(Vec(35.573, 89.323)), module, ModuleType::POLE2_MIX_PARAM));
      addParam(createParamCentered<Trimpot>(mm2px(Vec(46.698, 89.323)), module, ModuleType::POLE3_MIX_PARAM));
      addParam(createParamCentered<Trimpot>(mm2px(Vec(57.823, 89.323)), module, ModuleType::POLE4_MIX_PARAM));
      addParam(createParamCentered<CKSSThree>(mm2px(Vec(13.323, 108.526)), module, ModuleType::SCOPE_SCALE_PARAM));
    }
    else {
      addParam(createParamCentered<CKSSThree>(mm2px(Vec(13.323, 90.526)), module, ModuleType::SCOPE_SCALE_PARAM));
    }
  }

};

} // namespace zox

using PoleDancerWorkbenchForPoleDancer = zox::PoleDancerWorkbench<zox::WorkbenchForPoleDancer>;
using PoleDancerWorkbenchForPersonality = zox::PoleDancerWorkbench<zox::WorkbenchForPersonality>;
using PoleDancerWorkbenchForPoleDancerWidget = zox::PoleDancerWorkbenchWidget<zox::WorkbenchForPoleDancer>;
using PoleDancerWorkbenchForPersonalityWidget = zox::PoleDancerWorkbenchWidget<zox::WorkbenchForPersonality>;

Model* modelPoleDancerWorkbenchForPoleDancer =
  createModel<PoleDancerWorkbenchForPoleDancer,
              PoleDancerWorkbenchForPoleDancerWidget
              >("PoleDancerWorkbench-PoleDancer");

Model* modelPoleDancerWorkbenchForPersonality =
  createModel<PoleDancerWorkbenchForPersonality,
              PoleDancerWorkbenchForPersonalityWidget
              >("PoleDancerWorkbench-Personality");

