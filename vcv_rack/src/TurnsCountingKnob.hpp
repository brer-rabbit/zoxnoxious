#pragma once
#include <rack.hpp>

using namespace rack;

// ---------------------------------------------------------------------------
//  TurnsCountingKnob
//
//  A VCV Rack 2 knob widget that mimics a 10-turn counting dial.
//
//  Visual behaviour
//  ----------------
//  • The knob body rotates continuously across its full param range,
//    completing exactly 10 full revolutions from min to max.
//  • A small rectangular "counter window" is drawn on the face of the knob.
//    It shows a single digit (0-9) indicating which revolution is current.
//  • Both the rotation and the digit are derived purely from the normalised
//    param value, so they are always in sync with undo/redo and CV modulation.
//
//  Usage
//  -----
//  1.  Call TurnsCountingKnob::init() once inside your Module::init() or at
//      plugin load time to register the SVG.
//
//  2.  In your ModuleWidget constructor:
//          auto* knob = createParamCentered<TurnsCountingKnob>(
//              mm2px(Vec(10, 20)), module, MY_PARAM);
//          addParam(knob);
//
//  Customisation
//  -------------
//  All visual constants are public so subclasses or construction-time code can
//  adjust them before the first draw call.
//
//  num_turns      – total revolutions across the full param range (default 10)
//  windowRelPos   – centre of the counter window in knob-local normalised
//                   coords, e.g. Vec(0.0, -0.35) means slightly above centre
//  windowSize     – size of the counter window in pixels (at 1× zoom)
//  fontSize       – nvgFontSize for the digit
//  fontFaceId     – NVGcontext font (loaded once, stored per context)
// ---------------------------------------------------------------------------

struct TurnsCountingKnob : app::SvgKnob {
  widget::SvgWidget *bg;
  widget::SvgWidget *fg;

  // ---- tuneable constants ------------------------------------------------
  float windowRelX = 0.0f;   // fraction of knob radius, +right
  float windowRelY = -0.30f; // fraction of knob radius, +down
  float windowW = 7.5f;  // window width
  float windowH = 8.0f;  // window height
  float fontSize = 6.0f;   // nvgFontSize pixels
  float sweepCompensation = 1.0f; // sweep range by numTurns speed times this
  // -----------------------------------------------------------------------

  TurnsCountingKnob();
  // set the number of turns for the multiturn knob
  void setTurns(int turns);

  // Override to multiply the rotation by NUM_TURNS and draw the digit.
  void draw(const DrawArgs& args) override;

private:
  std::shared_ptr<Font> counterFont;
  int numTurns = 10;
  // Compute the current turn index (0 .. NUM_TURNS-1) from the param value.
  int currentTurn();
  
  // Draw the small counter-window rectangle and the digit inside it.
  void drawCounterWindow(const DrawArgs& args, int turn, float knobRadius) const;
};
