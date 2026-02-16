#pragma once
#include <vector>

#include "plugin.hpp"
#include "common.hpp"


namespace zox {

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
          INFO("new: button param %zu latched value %d MIDI program %d",
               i,
               curValue,
               program);
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




} // namespace zox
