#include <cmath>
#include <complex>
#include "plugin.hpp"
#include "modulehelpers.hpp"
#include "zcomponentlib.hpp"

namespace zox {

constexpr float MIN_DB = -36.f;
constexpr float MAX_DB = 24.f;
constexpr int POINTS = 256;

using Complex = std::complex<float>;

enum class DisplayMode { Bode, PoleZero };

// utility for range of drawing the scope
struct ScopeMode {
  // for bode
  float minW = 0.01f;
  float maxW = 100.f;
  // for pole/zero
  float realMin = -4.0;
  float realMax = 1.0;
  float imMin = -3.0;
  float imMax = 3.0;
  DisplayMode displayMode = DisplayMode::Bode;
};

struct PoleZeroRoots {
  Complex zeros[4] = {};
  Complex poles[4] = {};
};

struct PoleMixCoefficients {
  float weight[5] = {0.f, 0.f, 0.f, 0.f, 1.f};
  float fb[4] = {};
};


static float poleMixParamToCoeff(float v) {
  return v / POLEMIX_VOLTAGE_SCALE;
}

static constexpr float RESONANCE_VOLTAGE_SCALE = 4.f; // 2.5V == coefficient 1.0
static float resonanceParamToCoeff(float v) {
  return v * RESONANCE_VOLTAGE_SCALE;
}


static constexpr float sign[5] = {+1, -1, +1, -1, +1};

class PoleMixResponseModel {
public:
  PoleMixCoefficients coeffs;

  Complex eval(float w) const {
    Complex s(0.f, w);

    // pTerms = { p^4, p^3, p^2, p^1, p^0 }
    Complex pTerms[5];
    pTerms[3] = s + 1.f; 
    pTerms[2] = pTerms[3] * pTerms[3];
    pTerms[1] = pTerms[2] * pTerms[3];
    pTerms[0] = pTerms[2] * pTerms[2];
    pTerms[4] = 1.f;

    Complex numerator = 0.f;
    for (int i = 0; i < 5; ++i) {
      numerator += sign[i] * coeffs.weight[i] * pTerms[i];
    }

    Complex denominator = pTerms[0];
    for (int i = 0; i < 4; ++i) {
      denominator += coeffs.fb[i] * pTerms[i + 1];
    }

    return std::abs(denominator) < 1e-9f
      ? Complex(0.f, 0.f)
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
    SCOPE_MODE_PARAM,
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
  ScopeMode scopeMode;

  PersonalityMessage expanderMessages[2] = {};

  PoleDancerWorkbench() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(DRY_MIX_PARAM, 0.f, 10.f, 0.f, "Dry Mix", "%", 0.f, 10.f);
    configParam(POLE1_MIX_PARAM, 0.f, 10.f, 0.f, "Pole 1 Mix", "%", 0.f, 10.f);
    configParam(POLE2_MIX_PARAM, 0.f, 10.f, 0.f, "Pole 2 Mix", "%", 0.f, 10.f);
    configParam(POLE3_MIX_PARAM, 0.f, 10.f, 0.f, "Pole 3 Mix", "%", 0.f, 10.f);
    configParam(POLE4_MIX_PARAM, 0.f, 10.f, 10.f, "Pole 4 Mix", "%", 0.f, 10.f);
    configParam(RESONANCE_P1_PARAM, 0.f, 10.f, 10.f, "Pole 1 Feedback", "%", 0.f, 10.f);
    configParam(RESONANCE_P2_PARAM, 0.f, 10.f, 10.f, "Pole 2 Feedback", "%", 0.f, 10.f);
    configParam(RESONANCE_P3_PARAM, 0.f, 10.f, 10.f, "Pole 3 Feedback", "%", 0.f, 10.f);
    configParam(RESONANCE_P4_PARAM, 0.f, 10.f, 10.f, "Pole 4 Feedback", "%", 0.f, 10.f);
    configSwitch(SCOPE_SCALE_PARAM, 0.f, 2.f, 1.f, "Scope Scale", {"Wide", "Mid", "Narrow"});
    configSwitch(SCOPE_MODE_PARAM, 0.f, 1.f, 0.f, "Scope Mode", {"Bode", "Pole/Zero"});
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

    int mode = static_cast<int>(std::round(params[SCOPE_MODE_PARAM].getValue()));
    scopeMode.displayMode = (mode == 0) ? DisplayMode::Bode : DisplayMode::PoleZero;

    switch (static_cast<int>(std::round(params[SCOPE_SCALE_PARAM].getValue()))) {
    case 2:
      scopeMode.minW = 0.1;
      scopeMode.maxW = 10.f;
      scopeMode.realMin = -2.0;
      scopeMode.realMax = 0.5;
      scopeMode.imMin = -1.5;
      scopeMode.imMax = 1.5;
      break;
    case 1:
      scopeMode.minW = 0.01;
      scopeMode.maxW = 100.f;
      scopeMode.realMin = -4.0;
      scopeMode.realMax = 1.0;
      scopeMode.imMin = -3.0;
      scopeMode.imMax = 3.0;
      break;
    default:
      scopeMode.minW = 0.001;
      scopeMode.maxW = 1000.f;
      scopeMode.realMin = -8.0;
      scopeMode.realMax = 2.0;
      scopeMode.imMin = -6.0;
      scopeMode.imMax = 6.0;
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


float wToX(float w, const Rect& r, const ScopeMode& range) {
  float t = std::log(w / range.minW) / std::log(range.maxW / range.minW);
  return r.pos.x + t * r.size.x;
}

float dbToY(float db, const Rect& r) {

  float yNorm = (db - MIN_DB) / (MAX_DB - MIN_DB);
  yNorm = clamp(yNorm, 0.f, 1.f);

  return r.pos.y + (1.f - yNorm) * r.size.y;
}

// phase in terms of pi
float phaseToY(float phase, const Rect& r) {
  // phase range: -pi to +pi
  float yNorm = (phase + M_PI) / (2.f * M_PI);
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

// pole/zero helpers:
void buildNumeratorPoly(const PoleMixCoefficients& c, Complex out[5]) {
  for (int i = 0; i < 5; ++i) {
    out[i] = sign[i] * c.weight[i];
  }
}

void buildDenominatorPoly(const PoleMixCoefficients& c, Complex out[5]) {
  out[0] = 1.f;
  out[1] = c.fb[0];
  out[2] = c.fb[1];
  out[3] = c.fb[2];
  out[4] = c.fb[3];
}


Vec sToPoint(Complex s, const Rect& r, const ScopeMode& mode) {
  float xNorm = (s.real() - mode.realMin) / (mode.realMax - mode.realMin);
  float yNorm = (s.imag() - mode.imMin) / (mode.imMax - mode.imMin);

  return Vec(
    r.pos.x + xNorm * r.size.x,
    r.pos.y + (1.f - yNorm) * r.size.y
  );
}

static bool isNearZero(float x) {
    return std::fabs(x) < 1e-9f;
}

static Complex evalPolynomial(const Complex* coeffs, int degree, Complex z) {
  Complex y = coeffs[0];

  for (int i = 1; i <= degree; ++i) {
    y = y * z + coeffs[i];
  }

  return y;
}

static bool solvePolynomialDurandKerner(const Complex poly[5], Complex roots[4], int* rootCount) {
  constexpr float COEFF_EPS = 1e-7f;
  constexpr float DENOM_EPS = 1e-9f;
  constexpr float CONVERGE_EPS = 1e-5f;
  constexpr int MAX_ITERS = 80;

  *rootCount = 0;

  // Find first non-zero coefficient.
  int first = -1;
  for (int i = 0; i < 5; ++i) {
    if (std::abs(poly[i]) > COEFF_EPS) {
      first = i;
      break;
    }
  }

  // Zero polynomial or constant polynomial: no finite roots to draw.
  if (first < 0 || first == 4) {
    return false;
  }

  int degree = 4 - first;
  *rootCount = degree;

  // Compact coefficients so coeffs[0] is leading coefficient.
  Complex coeffs[5];
  for (int i = 0; i <= degree; ++i) {
    coeffs[i] = poly[first + i] / poly[first];
  }

  if (degree == 1) {
    roots[0] = -coeffs[1];
    return true;
  }

  // Initial guesses. Only first `degree` are used.
  roots[0] = Complex( 1.0f,  0.0f);
  roots[1] = Complex( 0.0f,  1.0f);
  roots[2] = Complex(-1.0f,  0.5f);
  roots[3] = Complex( 0.4f, -0.9f);

  for (int iter = 0; iter < MAX_ITERS; ++iter) {
    float maxDelta = 0.f;

    for (int i = 0; i < degree; ++i) {
      Complex denom = 1.f;

      for (int j = 0; j < degree; ++j) {
        if (i == j) {
          continue;
        }

        Complex diff = roots[i] - roots[j];

        if (std::abs(diff) < DENOM_EPS) {
          diff += Complex(DENOM_EPS * (i + 1), DENOM_EPS * (j + 1));
        }

        denom *= diff;
      }

      if (std::abs(denom) < DENOM_EPS) {
        *rootCount = 0;
        return false;
      }

      Complex delta = evalPolynomial(coeffs, degree, roots[i]) / denom;
      roots[i] -= delta;

      maxDelta = std::max(maxDelta, std::abs(delta));
    }

    if (maxDelta < CONVERGE_EPS) {
      return true;
    }
  }

  *rootCount = 0;
  return false;
}


template <typename Policy>
struct PoleDancerWorkbenchDisplay : LedDisplay {
  using ModuleType = PoleDancerWorkbench<Policy>;
  ModuleType* module = nullptr;
  PoleMixCoefficients lastCoeffs;
  std::vector<Vec> magPoints;
  std::vector<Vec> phasePoints;
  ScopeMode lastMode;
  Complex zeros[4];
  Complex poles[4];
  bool polesValid = false;
  bool zerosValid = false;
  int zeroCount = 0;
  int poleCount = 0;
  bool dirty = true;

  PoleDancerWorkbenchDisplay() {
    magPoints.reserve(POINTS);
    phasePoints.reserve(POINTS);
  }


  void rebuildRoots(const PoleMixCoefficients& c) {
    Complex numerator[5];
    Complex denominator[5];

    buildNumeratorPoly(c, numerator);
    buildDenominatorPoly(c, denominator);

    zeroCount = 0;
    poleCount = 0;

    zerosValid = solvePolynomialDurandKerner(numerator, zeros, &zeroCount);

    if (zerosValid) {
      for (int i = 0; i < zeroCount; ++i) {
        zeros[i] -= Complex(1.f, 0.f);
      }
    }

    // Special case: no feedback means D(p) = p^4
    if (isNearZero(c.fb[0]) &&
        isNearZero(c.fb[1]) &&
        isNearZero(c.fb[2]) &&
        isNearZero(c.fb[3])) {
      for (int i = 0; i < 4; ++i) {
        poles[i] = Complex(-1.f, 0.f); // already s-plane
      }

      polesValid = true;
      poleCount = 4;
    }
    else {
      polesValid = solvePolynomialDurandKerner(denominator, poles, &poleCount);

      if (polesValid) {
        for (int i = 0; i < poleCount; ++i) {
          poles[i] -= Complex(1.f, 0.f);
        }
      }
    }
  }


  void drawPoleZeroGrid(NVGcontext* vg, const Rect& r) {
    // Axis drawing is not guarded by scopeMode boundary
    // real axis: Im = 0.
    Vec left = sToPoint(Complex(lastMode.realMin, 0.f), r, lastMode);
    Vec right = sToPoint(Complex(lastMode.realMax, 0.f), r, lastMode);

    nvgBeginPath(vg);
    nvgMoveTo(vg, left.x, left.y);
    nvgLineTo(vg, right.x, right.y);
    nvgStrokeWidth(vg, 1.f);
    nvgStrokeColor(vg, nvgRGBA(180, 180, 180, 70));
    nvgStroke(vg);

    // imaginary axis: Re = 0
    Vec bottom = sToPoint(Complex(0.f, lastMode.imMin), r, lastMode);
    Vec top = sToPoint(Complex(0.f, lastMode.imMax), r, lastMode);

    nvgBeginPath(vg);
    nvgMoveTo(vg, bottom.x, bottom.y);
    nvgLineTo(vg, top.x, top.y);
    nvgStrokeWidth(vg, 1.f);
    nvgStrokeColor(vg, nvgRGBA(180, 180, 180, 90));
    nvgStroke(vg);
  }

  void drawZero(NVGcontext* vg, Vec p) {
    nvgBeginPath(vg);
    nvgCircle(vg, p.x, p.y, 3.5f);
    nvgStrokeWidth(vg, 1.2f);
    nvgStrokeColor(vg, nvgRGB(0xff, 0x9a, 0x35));
    nvgStroke(vg);
  }

  void drawPole(NVGcontext* vg, Vec p) {
    constexpr float r = 3.5f;

    nvgBeginPath(vg);
    nvgMoveTo(vg, p.x - r, p.y - r);
    nvgLineTo(vg, p.x + r, p.y + r);
    nvgMoveTo(vg, p.x - r, p.y + r);
    nvgLineTo(vg, p.x + r, p.y - r);

    nvgStrokeWidth(vg, 1.3f);
    nvgStrokeColor(vg, nvgRGB(80, 180, 255));
    nvgStroke(vg);
  }

  bool isVisible(Complex s, const ScopeMode& mode) {
    return s.real() >= mode.realMin && s.real() <= mode.realMax &&
      s.imag() >= mode.imMin   && s.imag() <= mode.imMax;
  }

  void drawPoleZeroPlot(NVGcontext* vg, const Rect& r) {
    drawPoleZeroGrid(vg, r);

    if (zerosValid) {
      for (int i = 0; i < zeroCount; ++i) {
        if (isVisible(zeros[i], lastMode))
          drawZero(vg, sToPoint(zeros[i], r, lastMode));
      }
    }

    if (polesValid) {
      for (int i = 0; i < poleCount; ++i) {
        if (isVisible(poles[i], lastMode))
          drawPole(vg, sToPoint(poles[i], r, lastMode));
      }
    }

  }


  void rebuildMagnitudePoints(const PoleMixCoefficients& c, const Rect& r, const ScopeMode& scopeMode) {
    magPoints.clear();

    PoleMixResponseModel model;
    model.coeffs = c;

    //Rect r = box.zeroPos().shrink(Vec(4, 4));

    for (int i = 0; i < POINTS; ++i) {
      float t = float(i) / float(POINTS - 1);
      float w = scopeMode.minW * std::pow(scopeMode.maxW / scopeMode.minW, t);

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


  void rebuildPhasePoints(const PoleMixCoefficients& c, const Rect& r, const ScopeMode& scopeMode) {
    phasePoints.clear();
    PoleMixResponseModel model;
    model.coeffs = c;

    for (int i = 0; i < POINTS; ++i) {
      float t = float(i) / float(POINTS - 1);
      float w = scopeMode.minW * std::pow(scopeMode.maxW / scopeMode.minW, t);
      auto h = model.eval(w);

      float phase = std::arg(h); // Value in (-π, π]

      // Map -pi to +pi across the vertical height of phaseRect
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

  void drawVerticalGridLine(NVGcontext* vg, const Rect& r, float w, NVGcolor color, float width, const ScopeMode& scopeMode) {

    if (w < scopeMode.minW || w > scopeMode.maxW)
      return;

    float x = wToX(w, r, scopeMode);

    nvgBeginPath(vg);
    nvgMoveTo(vg, x, r.pos.y);
    nvgLineTo(vg, x, r.pos.y + r.size.y);
    nvgStrokeWidth(vg, width);
    nvgStrokeColor(vg, color);
    nvgStroke(vg);
  }


  void drawGrid(NVGcontext* vg, const Rect& magRect, const Rect& phaseRect, const ScopeMode& scopeMode) {
    // 0 dB line only in magnitude plot
    float y0 = dbToY(0.f, magRect);

    // magnitude zero db
    nvgBeginPath(vg);
    nvgMoveTo(vg, magRect.pos.x, y0);
    nvgLineTo(vg, magRect.pos.x + magRect.size.x, y0);
    nvgStrokeWidth(vg, 1.0f);
    nvgStrokeColor(vg, nvgRGBA(180, 180, 180, 80));
    nvgStroke(vg);

    y0 = phaseToY(0.f, phaseRect);
    nvgBeginPath(vg);
    nvgMoveTo(vg, phaseRect.pos.x, y0);
    nvgLineTo(vg, phaseRect.pos.x + phaseRect.size.x, y0);
    nvgStrokeWidth(vg, 1.0f);
    nvgStrokeColor(vg, nvgRGBA(180, 180, 180, 80));
    nvgStroke(vg);

    // frequency grid lines, broken across both plots
    drawVerticalGridLine(vg, magRect, 0.5f, nvgRGBA(180, 180, 180, 100), 1.0f, scopeMode);
    drawVerticalGridLine(vg, magRect, 1.0f, nvgRGBA(180, 180, 180, 150), 1.0f, scopeMode);
    drawVerticalGridLine(vg, magRect, 2.0f, nvgRGBA(180, 180, 180, 100), 1.0f, scopeMode);

    drawVerticalGridLine(vg, phaseRect, 0.5f, nvgRGBA(180, 180, 180, 100), 1.0f, scopeMode);
    drawVerticalGridLine(vg, phaseRect, 1.0f, nvgRGBA(180, 180, 180, 150), 1.0f, scopeMode);
    drawVerticalGridLine(vg, phaseRect, 2.0f, nvgRGBA(180, 180, 180, 100), 1.0f, scopeMode);
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
      ScopeMode scopeMode = module ? module->scopeMode : ScopeMode{};

      // Split the box: 60% for Magnitude, 40% for Phase
      float splitY = box.size.y * 0.6f;
      Rect magRect = Rect(Vec(0, 0), Vec(box.size.x, splitY)).shrink(Vec(4, 4));
      Rect phaseRect = Rect(Vec(0, splitY), Vec(box.size.x, box.size.y - splitY)).shrink(Vec(4, 4));

      if (coeffsChanged(c, lastCoeffs) ||
          scopeMode.displayMode != lastMode.displayMode ||
          different(scopeMode.minW, lastMode.minW) ||
          different(scopeMode.maxW, lastMode.maxW) ||
          different(scopeMode.realMin, lastMode.realMin) ||
          different(scopeMode.realMax, lastMode.realMax) ||
          different(scopeMode.imMin, lastMode.imMin) ||
          different(scopeMode.imMax, lastMode.imMax) ||
          dirty) {
        dirty = false;
        lastCoeffs = c;
        lastMode = scopeMode;
        if (scopeMode.displayMode == DisplayMode::Bode) {
          rebuildMagnitudePoints(c, magRect, scopeMode);
          rebuildPhasePoints(c, phaseRect, scopeMode);
        }
        else {
          rebuildRoots(c);
        }
      }

      if (scopeMode.displayMode == DisplayMode::Bode) {
        drawLabel(args.vg, "MAG", magRect);
        drawLabel(args.vg, "PHASE", phaseRect);
        drawGrid(args.vg, magRect, phaseRect, scopeMode);
        drawCurve(args.vg, magPoints, nvgRGB(0xff, 0x9a, 0x35));
        drawCurve(args.vg, phasePoints, nvgRGB(80, 180, 255), phaseRect.size.y * 0.4f); // Blue Phase
      }
      else {
        Rect pzRect = box.zeroPos().shrink(Vec(4, 4));
        Vec origin = sToPoint(Complex(0.f, 0.f), pzRect, lastMode);
        drawLabel(args.vg, "Re(s)", Rect(Vec(pzRect.pos.x + pzRect.size.x - 26.f, origin.y + 3.f), Vec(24.f, 8.f)));
        drawLabel(args.vg, "Im(s)", Rect(Vec(origin.x + 3.f, pzRect.pos.y + 2.f), Vec(24.f, 8.f)));
        drawPoleZeroPlot(args.vg, pzRect);
      }
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

    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    auto* display = createWidget<PoleDancerWorkbenchDisplay<Policy> >(mm2px(Vec(5.5, 15.0)));
    display->box.size = mm2px(Vec(60.0, 60.0));
    display->module = module;
    addChild(display);

    if (Policy::hasControls()) {
      addParam(createParamCentered<Trimpot>(mm2px(Vec(13.323, 91.323)), module, ModuleType::DRY_MIX_PARAM));
      addParam(createParamCentered<Trimpot>(mm2px(Vec(24.448, 91.323)), module, ModuleType::POLE1_MIX_PARAM));
      addParam(createParamCentered<Trimpot>(mm2px(Vec(35.573, 91.323)), module, ModuleType::POLE2_MIX_PARAM));
      addParam(createParamCentered<Trimpot>(mm2px(Vec(46.698, 91.323)), module, ModuleType::POLE3_MIX_PARAM));
      addParam(createParamCentered<Trimpot>(mm2px(Vec(57.823, 91.323)), module, ModuleType::POLE4_MIX_PARAM));
      addParam(createParamCentered<CKSS>(mm2px(Vec(13.323, 110.526)), module, ModuleType::SCOPE_MODE_PARAM));
      addParam(createParamCentered<CKSSThree>(mm2px(Vec(41.698, 110.526)), module, ModuleType::SCOPE_SCALE_PARAM));
    }
    else {
      addParam(createParamCentered<CKSS>(mm2px(Vec(13.323, 90.526)), module, ModuleType::SCOPE_MODE_PARAM));
      addParam(createParamCentered<CKSSThree>(mm2px(Vec(41.698, 90.526)), module, ModuleType::SCOPE_SCALE_PARAM));
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

