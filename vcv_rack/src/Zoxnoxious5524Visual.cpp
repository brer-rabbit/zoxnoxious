#include <algorithm>
#include "plugin.hpp"
#include "modulehelpers.hpp"
#include "zcomponentlib.hpp"
#include "Zoxnoxious5524.hpp"

namespace zox {


struct Zoxnoxious5524Visual : Module {
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
  CouplingDisplayState couplingState;

  Zoxnoxious5524Visual() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    clockDivider.setDivision(256);
  }


  void process(const ProcessArgs& args) override {

    if (clockDivider.process()) {

      if (leftPresent()) {
        Zoxnoxious5524* z5524 = dynamic_cast<Zoxnoxious5524*>(leftExpander.module);
        if (z5524) {
          couplingState = z5524->getCouplingDisplayState();
        }
        
      }
      else {
        couplingState = {};
      }
    }

  }


  // policy method for template
  bool leftPresent() const {
    return leftExpander.module && leftExpander.module->model == modelZoxnoxious5524;
  }
};


struct CoupledVoiceTopologyDisplay : LedDisplay {
  CouplingDisplayState* state = nullptr;

  enum class DisplayColor { Green, Amber };

  static constexpr float W = 120.f;
  static constexpr float H = 120.f;

  enum WaveMask {
    WAVE_PULSE = 1 << 0,
    WAVE_B    = 1 << 1, // RAW: SAW, SHAPED: HALF-SINE
    WAVE_C    = 1 << 2, // RAW: TRI, SHAPED: SINE
  };

  CoupledVoiceTopologyDisplay() {
    box.size = Vec(W, H);
  }

  float clamp01(float v) const {
    return clamp(v, 0.f, 1.f);
  }


  float tzfmPulseActivity(const CouplingDisplayState& s) const {
    return clamp01(std::max(hasRawPulse(s) ? s.vco2RawToVco1Tzfm : 0.f,
                            hasShapedPulse(s) ? s.vco2ShapedToVco1Tzfm : 0.f));
  }

  float tzfmActivity(const CouplingDisplayState& s) const {
    return clamp01(std::max(
                     { tzfmPulseActivity(s),
                         (hasSaw(s) || hasTri(s)) ? s.vco2RawToVco1Tzfm : 0.f,
                         (hasHalfSine(s) || hasSine(s)) ? s.vco2ShapedToVco1Tzfm : 0.f
                         }));
  }

  float tzfmRawActivity(const CouplingDisplayState& s) const {
    return clamp01(hasRawPulse(s) || hasSaw(s) || hasTri(s) ? s.vco2RawToVco1Tzfm : 0.f);
  }

  float tzfmShapedActivity(const CouplingDisplayState& s) const {
    return clamp01(hasShapedPulse(s) || hasHalfSine(s) || hasSine(s) ? s.vco2ShapedToVco1Tzfm : 0.f);
  }

  bool hasRawPulse(const CouplingDisplayState& s) const {
    return s.vco2RawWaveMask & WAVE_PULSE;
  }

  bool hasShapedPulse(const CouplingDisplayState& s) const {
    return s.vco2ShapedWaveMask & WAVE_PULSE;
  }

  bool hasSaw(const CouplingDisplayState& s) const {
    return s.vco2RawWaveMask & WAVE_B;
  }

  bool hasTri(const CouplingDisplayState& s) const {
    return s.vco2RawWaveMask & WAVE_C;
  }

  bool hasHalfSine(const CouplingDisplayState& s) const {
    return s.vco2ShapedWaveMask & WAVE_B;
  }

  bool hasSine(const CouplingDisplayState& s) const {
    return s.vco2ShapedWaveMask & WAVE_C;
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

  NVGcolor pathColor(float activity, float baseAlpha = 0.12f) const {
    float a = baseAlpha + 0.88f * clamp01(activity);
    return nvgRGBAf(0.58f, 1.00f, 0.72f, a);
  }

  NVGcolor displayTextColor(DisplayColor color, float a = 1.f) const {
    switch (color) {
    case DisplayColor::Amber:
      return nvgRGBAf(0.88f, 0.62f, 0.24f, a);
    case DisplayColor::Green:
    default:
      return nvgRGBAf(0.70f, 0.88f, 0.78f, a);
    }
  }

  NVGcolor displayPathColor(DisplayColor color, float activity, float baseAlpha = 0.12f) const {
    float a = baseAlpha + 0.88f * clamp(activity, 0.f, 1.f);

    switch (color) {
    case DisplayColor::Amber:
      return nvgRGBAf(0.88f, 0.62f, 0.24f, a);
    case DisplayColor::Green:
    default:
      return nvgRGBAf(0.58f, 1.00f, 0.72f, a);
    }
  }


  void drawText(NVGcontext* vg, Vec p, const char* text, float size,
                int align = NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE,
                float alpha = 1.f, DisplayColor color = DisplayColor::Green) {
    nvgFontSize(vg, size);
    nvgFontFaceId(vg, APP->window->uiFont->handle);
    nvgTextAlign(vg, align);
    nvgFillColor(vg, displayTextColor(color, alpha));
    nvgText(vg, p.x, p.y, text, NULL);
  }

  void drawNode(NVGcontext* vg, Rect r, const char* label) {
    nvgBeginPath(vg);
    nvgRoundedRect(vg, r.pos.x, r.pos.y, r.size.x, r.size.y, 2.5f);
    nvgFillColor(vg, nodeFill());
    nvgFill(vg);

    nvgStrokeWidth(vg, 0.9f);
    nvgStrokeColor(vg, nodeStroke(0.95f));
    nvgStroke(vg);

    drawText(vg, r.getCenter(), label, 7.0f);
  }

  void drawArrowLine(NVGcontext* vg,
                     Vec a,
                     Vec b,
                     float activity,
                     const char* label = nullptr,
                     float labelOffsetY = 0.f,
                     bool secondary = false,
                     DisplayColor color = DisplayColor::Green) {
    float labelAlpha = activity > 0.f
      ? (secondary ? 0.72f : 0.94f)
      : (secondary ? 0.28f : 0.34f);

    float normalized = clamp(activity, 0.f, 1.f);
    float overdrive = clamp(activity - 1.f, 0.f, 0.5f);
    float width = secondary ? 0.65f : 0.95f + 2.35f * normalized + 2.0f * overdrive;

    Vec d = b.minus(a);
    float len = std::sqrt(d.x * d.x + d.y * d.y);

    if (len < 0.001f) {
      return;
    }

    d = d.div(len);
    Vec n(-d.y, d.x);

    float ah = secondary ? 3.2f : 4.8f;
    float aw = secondary ? 1.9f : 2.9f;
    float alpha = secondary ? 0.08f : 0.14f;

    // Stop the shaft before the arrowhead
    Vec shaftEnd = b.minus(d.mult(ah * 0.9f));

    //
    // Line shaft
    //
    nvgBeginPath(vg);
    nvgMoveTo(vg, a.x, a.y);
    nvgLineTo(vg, shaftEnd.x, shaftEnd.y);
    nvgStrokeWidth(vg, width);
    nvgStrokeColor(vg, displayPathColor(color, activity, alpha));
    nvgStroke(vg);

    //
    // Arrowhead
    //
    Vec p1 = b;
    Vec p2 = b.minus(d.mult(ah)).plus(n.mult(aw));
    Vec p3 = b.minus(d.mult(ah)).minus(n.mult(aw));

    nvgBeginPath(vg);
    nvgMoveTo(vg, p1.x, p1.y);
    nvgLineTo(vg, p2.x, p2.y);
    nvgLineTo(vg, p3.x, p3.y);
    nvgClosePath(vg);
    nvgFillColor(vg, displayPathColor(color, activity, alpha));
    nvgFill(vg);

    //
    // Label
    //
    if (label && label[0]) {
      Vec mid = a.plus(b).mult(0.5f);

      drawText(vg,
               Vec(mid.x, mid.y + labelOffsetY),
               label,
               secondary ? 4.2f : 5.4f,
               NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE,
               labelAlpha);
    }
  }


  void drawBusRail(NVGcontext* vg, float x, float y1, float y2, float activity) {
    NVGcolor c = pathColor(activity, 0.10f);

    nvgBeginPath(vg);
    nvgMoveTo(vg, x, y1);
    nvgLineTo(vg, x, y2);
    nvgStrokeWidth(vg, 0.7f);
    nvgStrokeColor(vg, c);
    nvgStroke(vg);
  }

  void drawWaveLamp(NVGcontext* vg, Vec p, const char* label, bool active, float pathActivity, DisplayColor color = DisplayColor::Green) {
    float a = active ? (pathActivity > 0.f ? 1.f : 0.35f) : 0.16f;

    nvgBeginPath(vg);
    nvgRoundedRect(vg, p.x - 6.2f, p.y - 4.0f, 12.4f, 8.0f, 1.8f);
    nvgFillColor(vg, nvgRGBAf(0.05f, 0.10f, 0.09f, active ? 0.85f : 0.28f));
    nvgFill(vg);

    nvgStrokeWidth(vg, active ? 0.9f : 0.45f);
    nvgStrokeColor(vg, displayTextColor(color, a));
    nvgStroke(vg);

    drawText(vg, p, label, 3.7f, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE, a, color);
  }

  void drawWaveBank(NVGcontext* vg, const CouplingDisplayState& s) {
    float a = tzfmPulseActivity(s);
    bool tzfmActive = tzfmRawActivity(s) || tzfmShapedActivity(s);

    drawText(vg, Vec(20, 110), "TZFM SRC:", 4.0f,
             NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE,
             tzfmActive ? 0.83f : 0.48f);

    drawWaveLamp(vg, Vec(52, 110), "PULSE", hasRawPulse(s) || hasShapedPulse(s), a);
    drawWaveLamp(vg, Vec(66, 110), "SAW", hasSaw(s), s.vco2RawToVco1Tzfm);
    drawWaveLamp(vg, Vec(80, 110), "TRI", hasTri(s), s.vco2RawToVco1Tzfm);
    drawWaveLamp(vg, Vec(94, 110), "½SIN", hasHalfSine(s), s.vco2ShapedToVco1Tzfm, DisplayColor::Amber);
    drawWaveLamp(vg, Vec(108, 110), "SINE", hasSine(s), s.vco2ShapedToVco1Tzfm, DisplayColor::Amber);
  }

  void drawLayer(const DrawArgs& args, int layer) override {
    LedDisplay::drawLayer(args, layer);
    if (layer != 1)
      return;

    NVGcontext* vg = args.vg;
    nvgSave(vg);

    nvgScale(vg, box.size.x / W, box.size.y / H);

    const CouplingDisplayState emptyState;
    const CouplingDisplayState& s = state ? *state : emptyState;

    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, W, H);
    nvgFillColor(vg, bgColor());
    nvgFill(vg);

    Rect vco1(Vec(9, 22), Vec(30, 18));
    Rect vco2(Vec(81, 22), Vec(30, 18));
    Rect vcf(Vec(45, 72), Vec(30, 18));

    // Rails for secondary/control relationships
    float syncActivity = (s.syncHardSub || s.syncSoft) ? 1.f : 0.f;
    float waveSelActivity = s.vco1ToVco2WaveSelect ? 1.f : 0.f;

    drawBusRail(vg, 41, 12, 50, std::max(syncActivity, waveSelActivity));
    drawBusRail(vg, 79, 12, 50, std::max(syncActivity, waveSelActivity));

    // Secondary rail paths
    const char* syncLabel =
      (s.syncHardSub && s.syncSoft) ? "H&S SYNC" :
      (s.syncHardSub ? "HARD SUB" :
       (s.syncSoft ? "SOFT SYNC" : "SYNC"));

    drawArrowLine(vg, Vec(79, 15), Vec(41, 15),
                  syncActivity, syncLabel, -3.0f, true);

    drawArrowLine(vg, Vec(41, 49), Vec(79, 49),
                  waveSelActivity, "SHAPE MOD", 3.0f, true);

    // Primary box-to-box paths
    drawArrowLine(vg, Vec(80, 24.2f), Vec(41, 24.2f),
                  tzfmRawActivity(s) > 0.f ?
                  0.75f * tzfmRawActivity(s) :
                  (tzfmShapedActivity(s) > 0.f ? 0.01f : 0.f),
                  "TZFM", -4.0f);

    drawArrowLine(vg, Vec(80, 29.8f), Vec(41, 29.8f),
                  0.75f * tzfmShapedActivity(s),
                  nullptr, -4.0f, false,
                  DisplayColor::Amber);

    drawArrowLine(vg, Vec(40, 35), Vec(79, 35),
                  s.vco1ToVco2ExpFm, "EXP FM", 4.0f);

    // VCO-to-VCF modulation paths
    drawArrowLine(vg, Vec(28, 40), Vec(50, 71),
                  s.vco1ToVcfExpFm, nullptr);

    drawText(vg, Vec(31, 57), "EXP", 4.6f,
             NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE,
             s.vco1ToVcfExpFm > 0.f ? 0.94f : 0.34f);


    drawArrowLine(vg, Vec(92, 40), Vec(70, 71),
                  s.vco2ToVcfLinFm, nullptr);

    drawText(vg, Vec(91, 57), "LINEAR", 4.6f,
             NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE,
             s.vco2ToVcfLinFm > 0.f ? 0.94f : 0.34f);

    drawNode(vg, vco1, "VCO1");
    drawNode(vg, vco2, "VCO2");
    drawNode(vg, vcf, "VCF");

    drawWaveBank(vg, s);

    nvgRestore(vg);
  }
};


struct Zoxnoxious5524VisualWidget : ModuleWidget {

  Zoxnoxious5524VisualWidget(Zoxnoxious5524Visual* module) {
    setModule(module);

    setPanel(createPanel(asset::plugin(pluginInstance, "res/Zoxnoxious5524Viz.svg")));

    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    if (module) {
      auto* display = createWidget<CoupledVoiceTopologyDisplay>(mm2px(Vec(5.5, 15.0)));
      display->box.size = mm2px(Vec(60.0, 60.0));
      display->state = &module->couplingState;
      addChild(display);
    }

  }

};


} // namespace zox

Model* modelZoxnoxious5524Visual = createModel<zox::Zoxnoxious5524Visual, zox::Zoxnoxious5524VisualWidget>("Zoxnoxious5524Visual");
