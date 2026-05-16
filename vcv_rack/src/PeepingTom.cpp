#include <cmath>
#include <complex>
#include "plugin.hpp"
#include "zcomponentlib.hpp"

constexpr float MIN_DB = -36.f;
constexpr float MAX_DB = 24.f;
constexpr int POINTS = 256;
constexpr float MIN_W = 0.01f;  // towards limiting: 0.05f
constexpr float MAX_W = 100.f;  // towards limiting: 30.f


struct PoleMixCoefficients {
    float a = 0.f; // input/dry
    float b = 0.f; // pole1
    float c = 0.f; // pole2
    float d = 0.f; // pole3
    float e = 1.f; // pole4
    float feedback = 0.f;
};


static constexpr float POLEMIX_VOLTAGE_SCALE = 1.25f; // 1.25V == coefficient 1.0
static constexpr float RESONANCE_VOLTAGE_SCALE = 2.5f; // 2.5V == coefficient 1.0
static float poleMixInputToCoeff(float v) {
    return v / POLEMIX_VOLTAGE_SCALE;
}
static float resonanceInputToCoeff(float v) {
    return v / RESONANCE_VOLTAGE_SCALE;
}



class PoleMixResponseModel {
public:
  PoleMixCoefficients coeffs;

  std::complex<float> eval(float w) const {
    std::complex<float> s(0.0f, w);
    auto p = s + 1.0f;
    auto p2 = p * p;
    auto p3 = p2 * p;
    auto p4 = p3 * p;

    auto numerator = coeffs.a * p4 -
      coeffs.b * p3 +
      coeffs.c * p2 -
      coeffs.d * p +
      coeffs.e;

    auto denominator = p4 + coeffs.feedback;

    // if denominator gets too small return zero zero
    return (std::abs(denominator) < 1e-9f) ?
      std::complex<float>(0.f, 0.f) : numerator / denominator;
  }

};



struct PeepingTom : Module {
  enum ParamId {
    PARAMS_LEN
  };
  enum InputId {
    POLE_MIX_INPUT,
    RESONANCE_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    POLE_MIX_OUTPUT,
    RESONANCE_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };


  dsp::ClockDivider clockDivider;
  PoleMixCoefficients poleMixCoefs;


  PeepingTom() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    clockDivider.setDivision(512);
  }


  void process(const ProcessArgs& args) override {
    if (outputs[POLE_MIX_OUTPUT].isConnected()) {
        int n = inputs[POLE_MIX_INPUT].getChannels();
        outputs[POLE_MIX_OUTPUT].setChannels(n);

        for (int ch = 0; ch < n; ++ch) {
            outputs[POLE_MIX_OUTPUT].setVoltage(
                inputs[POLE_MIX_INPUT].getVoltage(ch), ch
            );
        }
    }

    if (outputs[RESONANCE_OUTPUT].isConnected()) {
      outputs[RESONANCE_OUTPUT].setVoltage(inputs[RESONANCE_INPUT].getVoltage());
    }

    if (clockDivider.process()) {
      readPoleMixCoefficients();
    }
  }



  void readPoleMixCoefficients() {
    PoleMixCoefficients nextPoleMix;

    if (inputs[POLE_MIX_INPUT].isConnected()) {
      int n = inputs[POLE_MIX_INPUT].getChannels();

      nextPoleMix.a = (n > 0) ? poleMixInputToCoeff(inputs[POLE_MIX_INPUT].getVoltage(0)) : 0.f;
      nextPoleMix.b = (n > 1) ? poleMixInputToCoeff(inputs[POLE_MIX_INPUT].getVoltage(1)) : 0.f;
      nextPoleMix.c = (n > 2) ? poleMixInputToCoeff(inputs[POLE_MIX_INPUT].getVoltage(2)) : 0.f;
      nextPoleMix.d = (n > 3) ? poleMixInputToCoeff(inputs[POLE_MIX_INPUT].getVoltage(3)) : 0.f;
      nextPoleMix.e = (n > 4) ? poleMixInputToCoeff(inputs[POLE_MIX_INPUT].getVoltage(4)) : 1.f;
    }

    nextPoleMix.feedback = inputs[RESONANCE_INPUT].isConnected() ?
      resonanceInputToCoeff(inputs[RESONANCE_INPUT].getVoltage()) :
      0.f;

    poleMixCoefs = nextPoleMix;
  }

};



static bool different(float a, float b) {
    return std::fabs(a - b) > 0.001f;
}

static bool coeffsChanged(const PoleMixCoefficients& x, const PoleMixCoefficients& y) {
    return different(x.a, y.a)
        || different(x.b, y.b)
        || different(x.c, y.c)
        || different(x.d, y.d)
        || different(x.e, y.e)
        || different(x.feedback, y.feedback);
}


float wToX(float w, const Rect& r) {
  float t = std::log(w / MIN_W) / std::log(MAX_W / MIN_W);
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


struct PeepingTomDisplay : LedDisplay {
  PeepingTom* module = nullptr;
  PoleMixCoefficients lastCoeffs;
  std::vector<Vec> magPoints;
  std::vector<Vec> phasePoints;


  PeepingTomDisplay() {
    magPoints.reserve(POINTS);
    phasePoints.reserve(POINTS);
  }

  void rebuildMagnitudePoints(const PoleMixCoefficients& c, const Rect& r) {
    magPoints.clear();

    PoleMixResponseModel model;
    model.coeffs = c;

    //Rect r = box.zeroPos().shrink(Vec(4, 4));

    for (int i = 0; i < POINTS; ++i) {
      float t = float(i) / float(POINTS - 1);
      float w = MIN_W * std::pow(MAX_W / MIN_W, t);

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


  void rebuildPhasePoints(const PoleMixCoefficients& c, const Rect& r) {
    phasePoints.clear();
    PoleMixResponseModel model;
    model.coeffs = c;

    for (int i = 0; i < POINTS; ++i) {
      float t = float(i) / float(POINTS - 1);
      float w = MIN_W * std::pow(MAX_W / MIN_W, t);
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

  void drawVerticalGridLine(NVGcontext* vg, const Rect& r, float w, NVGcolor color, float width) {

    if (w < MIN_W || w > MAX_W)
      return;

    float x = wToX(w, r);

    nvgBeginPath(vg);
    nvgMoveTo(vg, x, r.pos.y);
    nvgLineTo(vg, x, r.pos.y + r.size.y);
    nvgStrokeWidth(vg, width);
    nvgStrokeColor(vg, color);
    nvgStroke(vg);
  }


  void drawGrid(NVGcontext* vg, const Rect& magRect, const Rect& phaseRect) {
    // 0 dB line only in magnitude plot
    float y0 = dbToY(0.f, magRect);

    nvgBeginPath(vg);
    nvgMoveTo(vg, magRect.pos.x, y0);
    nvgLineTo(vg, magRect.pos.x + magRect.size.x, y0);
    nvgStrokeWidth(vg, 1.0f);
    nvgStrokeColor(vg, nvgRGBA(180, 180, 180, 80));
    nvgStroke(vg);

    // frequency grid lines, broken across both plots
    drawVerticalGridLine(vg, magRect, 0.5f, nvgRGBA(180, 180, 180, 30), 1.0f);
    drawVerticalGridLine(vg, magRect, 1.0f, nvgRGBA(180, 180, 180, 90), 1.0f);
    drawVerticalGridLine(vg, magRect, 2.0f, nvgRGBA(180, 180, 180, 30), 1.0f);

    drawVerticalGridLine(vg, phaseRect, 0.5f, nvgRGBA(180, 180, 180, 30), 1.0f);
    drawVerticalGridLine(vg, phaseRect, 1.0f, nvgRGBA(180, 180, 180, 90), 1.0f);
    drawVerticalGridLine(vg, phaseRect, 2.0f, nvgRGBA(180, 180, 180, 30), 1.0f);
  }

  void drawLabel(NVGcontext* vg, const char* text, const Rect& r) {
    nvgFontSize(vg, 8.f);
    nvgFillColor(vg, nvgRGBA(220, 220, 220, 120));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgText(vg, r.pos.x + 2.f, r.pos.y + 2.f, text, NULL);
  }


  void drawLayer(const DrawArgs& args, int layer) override {
    if (layer == 1) {
      PoleMixCoefficients c = module ? module->poleMixCoefs : PoleMixCoefficients{};

      // Split the box: 60% for Magnitude, 40% for Phase
      float splitY = box.size.y * 0.6f;
      Rect magRect = Rect(Vec(0, 0), Vec(box.size.x, splitY)).shrink(Vec(4, 4));
      Rect phaseRect = Rect(Vec(0, splitY), Vec(box.size.x, box.size.y - splitY)).shrink(Vec(4, 4));

      if (coeffsChanged(c, lastCoeffs)) {
        lastCoeffs = c;
        rebuildMagnitudePoints(c, magRect);
        rebuildPhasePoints(c, phaseRect);
      }

      drawLabel(args.vg, "MAG", magRect);
      drawLabel(args.vg, "PHASE", phaseRect);
      drawGrid(args.vg, magRect, phaseRect);
      drawCurve(args.vg, magPoints, nvgRGB(255, 220, 80)); // Yellow Mag
      drawCurve(args.vg, phasePoints, nvgRGB(80, 180, 255), phaseRect.size.y * 0.4f); // Blue Phase
    }
    LedDisplay::drawLayer(args, layer);
  }

};



struct PeepingTomWidget : ModuleWidget {
  PeepingTomWidget(PeepingTom* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/PeepingTom.svg")));

    PeepingTomDisplay* display = createWidget<PeepingTomDisplay>(mm2px(Vec(5.0, 15.0)));
    display->box.size = mm2px(Vec(60.0, 45.0));
    display->module = module;
    addChild(display);

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.213, 121.907)), module, PeepingTom::POLE_MIX_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.213, 121.907)), module, PeepingTom::RESONANCE_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(55.213, 121.907)), module, PeepingTom::POLE_MIX_OUTPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(65.213, 121.907)), module, PeepingTom::RESONANCE_OUTPUT));

  }

};

Model* modelPeepingTom = createModel<PeepingTom, PeepingTomWidget>("PeepingTom");
