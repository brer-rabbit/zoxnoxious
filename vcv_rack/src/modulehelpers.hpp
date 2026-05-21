#pragma once
#include <string>
#include "constants.hpp"
#include "plugin.hpp"

namespace zox {

//----------------------------------------------------------------------------
// helper class for setting MIDI message and associated UI lights.
// Common use case among Z-card classes is to have UI button elements that
// get sent to Zoxnoxious voice cards as MIDI messages.  MIDI Program Change
// is the way this is done.  This class simplifies the commoon pattern.

// this struct is intended to be static to a Z-Card module.
// It'd be set something like:
//  const std::vector<ButtonMapping<Zoxnoxious3340> > ZCard::buttonMappings = {
//    { SYNC_HARD_BUTTON_PARAM, SYNC_HARD_BUTTON_LIGHT, {0,1} }, ...
// where the {0,1} are the MIDI ProgramChange values that get sent by
// toggling the UI element.
// class variables:
//   static const std::vector<ButtonMapping<ZCard> > buttonMappings;
//   std::vector<ButtonState> buttonStates;
//   ButtonMidiController<ZCard> buttonMidiController;
// and initialized:
//   buttonStates(buttonMappings.size()),
//   buttonMidiController(buttonMappings),
//
// Then at business time to check the buttons and possibly get midi message set use:
//    buttonMidiController.process(this, midiChannel, midiMessage);
//    buttonMidiController.updateLights(this);


template <typename ModuleT>
struct ButtonMapping {
  using ParamId = typename ModuleT::ParamId;
  using LightId = typename ModuleT::LightId;

  ParamId param;
  LightId light;
  std::vector<int8_t> midiPrograms; // mapping from param to MIDI Progrma
};


struct ButtonState {
  int latchedValue = INT_MIN; // initialization sentinel
};


template <typename ModuleT>
class ButtonMidiController {
public:
  using Mapping = ButtonMapping<ModuleT>;

  explicit ButtonMidiController(const std::vector<Mapping>& mappings) :
    mappings(mappings), states(mappings.size()) {}

  bool process(rack::engine::Module* module,
               int8_t midiChannel,
               midi::Message& midiOut) {

    for (size_t i = 0; i < mappings.size(); ++i) {
      const auto& map = mappings[i];
      auto& state = states[i];

      int curValue = static_cast<int>(module->params[map.param].getValue() + 0.5f);

      if (state.latchedValue != curValue) {
        state.latchedValue = curValue;

        if (curValue >= 0 && curValue < static_cast<int>(map.midiPrograms.size())) {
          int8_t program = map.midiPrograms[curValue];
          setMidiProgramChangeMessage(midiOut, midiChannel, program);
          //INFO("new: button param %zu latched value %d MIDI program %d", i, curValue, program);
          return true; // only single midi::Message is set per call, so early return here
        }
        else {
          WARN("value %d out of range: expected %d : %zu",
               curValue, 0, map.midiPrograms.size() - 1);
        }
      }
    }
    return false;
  }

  void updateLights(rack::engine::Module* m) {
    for (size_t i = 0; i < mappings.size(); ++i) {
      const auto& map = mappings[i];
      bool on = m->params[map.param].getValue() > 0.f;
      m->lights[map.light].setBrightness(on);
    }
  }

private:
  const std::vector<ButtonMapping<ModuleT> > &mappings;
  std::vector<ButtonState> states;

};



//----------------------------------------------------------------------------
// helper to read jack+knob and output to an audio frame + manage clip timer.
// I had this sprinkled all over:
//
//  v = params[CUTOFF_KNOB_PARAM].getValue() + inputs[CUTOFF_INPUT].getVoltage() / 10.f;
//  controlMsg->frame[outputDeviceId].samples[cvChannelOffset + CUTOFF] = clamp(v, 0.f, 1.f);
//  if (controlMsg->frame[outputDeviceId].samples[cvChannelOffset + CUTOFF] != v) {
//      cutoffClipTimer = clipTime;
//  }
//
// This helper avoid the pattern.  To use:
// declare an array:
//   std::array<CvRoute,8> routes;
// then initialize it:
//    routes{{
//      {LINEAR_KNOB_PARAM, LINEAR_INPUT, LINEAR_CHANNEL, &linearClipTimer, nullptr} ...
// call it to fill in samples in audio frame:
// processCvRoutes(routes.data(),
//             routes.size(),
//             clipTime,
//             offset,
//             sharedFrame.samples,
//             params.data(),
//             inputs.data());


using CvTransform = float (*)(float);

enum class CvOperation {
  Add,
  MultiplyNormalled // VCA behavior, unipolar [0,1]
};

struct CvRoute {
  int knobParam;
  int inputId;
  int channel;
  float divisor;
  float* timer;
  CvTransform transform; // may be nullptr for identity transform
  CvOperation op;
};

inline void processCvRoutes(
    const CvRoute* routes,
    int count,
    float clipTime,
    int cvChannelOffset,
    float* frame,
    Param* params,
    Input* inputs)
{
  for (int i = 0; i < count; ++i) {
    const auto& r = routes[i];
    float knob = params[r.knobParam].getValue();
    float in = inputs[r.inputId].getVoltage() / r.divisor;
    float v;
    switch (r.op) {
      case CvOperation::Add:
        v = knob + in;
        break;
      case CvOperation::MultiplyNormalled:
        v = inputs[r.inputId].isConnected() ? knob * in : knob;
        break;
    default:
        v = 0.f;
    }

    if (r.transform) {
      v = r.transform(v);
    }

    bool clipped = (v < 0.f) || (v > 1.f);
    float clamped = clamp(v, 0.f, 1.f);
    frame[cvChannelOffset + r.channel] = clamped;

    if (clipped) {
      *r.timer = clipTime;
    }
  }
}

inline float dualLinearSwitch0_8(float v) {
  return v < 0.8f ? v * 0.6f : 2.6f * v - 1.6f;
}




//----------------------------------------------------------------------------
// Helper functions for up/down param selector switching.
// used where an up button/down button combo sets a +1/-1 to a state.
// A string is set based on the new value, and the passed on midi
// message populated with a payload.  Unlike other helpers this does
// not take the light arg; any button lights to be lit by the caller
//

inline int wrapIncrement(int value, int delta, int max) {
  value += delta;
  if (value > max) {
    value = 0;
  }
  else if (value < 0) {
    value = max;
  }
  return value;
}

template <typename NameLookup> inline bool handleUpDownSelector(
  Param& upButton,
  Param& downButton,
  Param& valueParam,
  int maxIndex,
  NameLookup nameLookup,
  std::string& nameString,
  const int8_t* midiPrograms,
  rack::midi::Message& midiMessage,
  int8_t midiChannel) {

  int delta = 0;

  if (upButton.getValue()) {
    upButton.setValue(0.f);
    delta += 1;
  }

  if (downButton.getValue()) {
    downButton.setValue(0.f);
    delta -= 1;
  }

  if (delta == 0) {
    return false;
  }

  int value = static_cast<int>(valueParam.getValue());
  int newValue = wrapIncrement(value, delta, maxIndex);

  if (newValue == value) {
    return false;
  }

  valueParam.setValue(newValue);

  nameString = nameLookup(newValue);

  setMidiProgramChangeMessage(midiMessage, midiChannel, midiPrograms[newValue]);
  return true;
}



// for adding a module via right-click menu
struct InstantiateExpanderItem : MenuItem {
  Module* module;
  Model* model;
  Vec posit;
  void onAction(const event::Action &e) override;
};

} // namespace zox
