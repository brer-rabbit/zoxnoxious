#include "TurnsCountingKnob.hpp"
#include "plugin.hpp"

namespace zox {

// ---------------------------------------------------------------------------
//  Implementation notes
//
//  VCV Rack's SvgKnob::draw() applies a rotation transform that maps the
//  normalised param value linearly across [minAngle, maxAngle]
//  (defaults: -150° to +150°, i.e. 300° total swing).
//
//  We want 10 full rotations instead.  The cleanest way to achieve that
//  without fighting the base-class rotation logic is to:
//
//    1.  Set minAngle / maxAngle so the base class rotates 360 * numTurns
//        across the full 0..1 param range.
//
//    2.  Override draw() to:
//          a.  Call the base class draw() – this paints the SVG with the
//              correct rotation already applied.
//          b.  Draw the counter window on top, in *unrotated* screen space
//              relative to the widget centre, so it always faces the user.
//
//  Why unrotated counter window?
//  The digit readout on a physical turns-counting dial is in a fixed cutout
//  on the knob face; the number wheel behind it rotates.  Faking that with
//  a single-digit nvgText() in fixed screen space is simpler and reads more
//  clearly than trying to counter-rotate the text.
//  For an effect, give it an LED font to make the appearance of some
//  electronics behind the curtain.
// ---------------------------------------------------------------------------

TurnsCountingKnob::TurnsCountingKnob() {
  // Load (or reuse) the standard round-knob SVG that ships with VCV Rack.
  // Swap the path for your own artwork if desired.
  setSvg(Svg::load(asset::plugin(pluginInstance, "res/TurnsCountingDial.svg")));

  bg = new widget::SvgWidget;
  bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/TurnsCountingDial_bg.svg")));
  fb->addChildBelow(bg, tw);

  fg = new widget::SvgWidget;
  fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/TurnsCountingDial_fg.svg")));
  fb->addChildAbove(fg, tw);

  // Set the rotation sweep so one full normalised range = numTurns * 360
  // SvgKnob interprets these as radians.
  setTurns(numTurns);

  // Allow smooth wrapping
  snap = false;

  counterFont = APP->window->loadFont(asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf"));
  //shadow->opacity = 0.00f;
}

void TurnsCountingKnob::setTurns(int turns) {
  numTurns = turns;
  const float fullSweep = numTurns * 2.0f * M_PI;
  minAngle = -fullSweep / 2.0f;
  maxAngle = fullSweep / 2.0f;

  speed = numTurns * sweepCompensation;
}


void TurnsCountingKnob::draw(const DrawArgs& args) {
  // 1. Let SvgKnob paint the rotated knob body.
  SvgKnob::draw(args);

  // 2. Overlay the counter window in screen-aligned (unrotated) space.
  //    The widget origin is already at the knob centre for SvgKnob, so we
  //    only need to save/restore around our own drawing.
  NVGcontext* vg = args.vg;
  nvgSave(vg);

  // Determine the radius from the bounding box (works for any SVG size).
  float knobRadius = std::min(box.size.x, box.size.y) * 0.5f;

  // The SvgKnob widget centres the SVG at its own origin, so the knob centre
  // in widget-local coords is (box.size.x/2, box.size.y/2).
  // Translate to knob centre so our relative coords work correctly.
  nvgTranslate(vg, box.size.x * 0.5f, box.size.y * 0.5f);

  int turn = currentTurn();
  drawCounterWindow(args, turn, knobRadius);

  nvgRestore(vg);
}




// private methods:

int TurnsCountingKnob::currentTurn() {
  ParamQuantity* pq = getParamQuantity();
  if (!pq) {
    return 0;
  }

  // get display value and truncate toward zero to get the major digit
  float displayVal = pq->getDisplayValue();
  int turn = static_cast<int>(std::floor(displayVal));
  return turn;
}


void TurnsCountingKnob::drawCounterWindow(const DrawArgs& args,
                                          int turn,
                                          float knobRadius) const {
  NVGcontext* vg = args.vg;

  // Position of the window center in widget-local pixels.
  // Window should sit slightly above the knob.
  //float cx = windowRelX * knobRadius;
  //float cy = windowRelY * knobRadius - knobRadius;
  float windowOffsetX = -0.0f;
  float windowOffsetY = -0.0f; // knob radii, negative = upward
  float cx = windowOffsetX * knobRadius;
  float cy = windowOffsetY * knobRadius;

  float hw = windowW * 0.5f;
  float hh = windowH * 0.5f;

  // ---- background --------------------------------------------------------
  nvgBeginPath(vg);
  nvgRoundedRect(vg, cx - hw, cy - hh, windowW, windowH, 0.1f);
  //nvgFillColor(vg, nvgRGBAf(0.08f, 0.08f, 0.08f, 0.92f));
  nvgFillColor(vg, nvgRGBA(0x20, 0x1A, 0x00, 0xFF));
  nvgFill(vg);

  // ---- border ------------------------------------------------------------
  //nvgStrokeColor(vg, nvgRGBAf(0.8f, 0.8f, 0.8f, 0.55f));
  //nvgStrokeWidth(vg, 0.6f);
  //nvgStroke(vg);

  // ---- digit -------------------------------------------------------------
  nvgFontSize(vg, fontSize);
  nvgFontFaceId(vg, counterFont->handle);
  nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
  nvgFillColor(vg, nvgRGBA(255, 0x90, 0x10, 0xff));

  char buf[3] = { '0', '\0', '\0' };
  if (turn < -9 || turn > 9) {
    // this is out of range!
    buf[0] = '0';
  }
  else if (turn >= 0) {
    buf[0] = static_cast<char>('0' + turn);
  }
  else {
    buf[0] = '.';
    buf[1] = static_cast<char>('0' + (-turn));
  }

  nvgText(vg, cx, cy, buf, nullptr);

  // then display all segment with a low alpha for realism
  nvgFillColor(vg, nvgRGBA(255, 0x90, 0x10, 0x30));
  nvgText(vg, cx, cy, ".8", nullptr);

}

}
