#pragma once
#include <vector>

#include "plugin.hpp"
#include "common.hpp"


namespace zox {

// helper class for setting MIDI message and associated UI lights.

template <typename ModuleT>
struct ButtonMapping {
  using ParamId = typename ModuleT::ParamId;
  using LightId = typename ModuleT::LightId;

  ParamId param;
  LightId light;
  std::vector<int8_t> midiPrograms; // array of MIDI program mapping
  bool isMomentary; // versus latched button lights
};


struct ButtonState {
  int latchedValue = INT_MIN; // initialization sentinel
};


template <typename ModuleT>
class ButtonMidiController {
public:
  using Mapping = ButtonMapping<ModuleT>;

  explicit ButtonMidiController(const Mapping* mappings, size_t count) :
    mappings(mappings), count(count), states(count) {}

  bool process(rack::engine::Module* module,
               int8_t midiChannel,
               midi::Message& midiOut) {
    for (size_t i = 0; i < count; ++i) {
      const auto& map = mappings[i];
      auto& state = states[i];

      int curValue = static_cast<int>(module->params[map.param].getValue() + 0.5f);

      if (state.latchedValue != curValue) {
        state.latchedValue = curValue;

        uint8_t program = map.midiProogram[curValue];
        setMidiMessage(midiOut, midiChannel, program);

        return true; // only single midi::Message is set per call, so early return here
      }
    }
    return false;
  }

  void updateLights(Module* m) {
    for (size_t i = 0; i < count; ++i) {
      const auto& map = mappings[i];
      bool on = m->params[map.param].getValue() > 0.f;
      m->lights[map.light].setBrightness(on);
    }
  }


private:
    const Mapping* mappings;
    size_t count;
    std::vector<ButtonState> states;
};




} // namespace zox
