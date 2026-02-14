#pragma once
#include <vector>

#include "plugin.hpp"
#include "common.hpp"


namespace zox {

struct ButtonMapping {
  ParamId param;
  LightId light;
  std::vector<int8_t> midiPrograms; // array of MIDI program mapping
  bool isMomentary; // versus latched button lights
};


struct ButtonState {
  int currentValue = INT_MIN;   // initialization sentinel
};

class ButtonMidiController {
public:
  ButtonMidiController(const ButtonMapping* mappings, size_t count) :
    mappings(mappings), count(count), states(count) {}

  bool process(rack::engine::Module* module,
               int8_t midiChannel,
               midi::Message& out) {
    for (size_t i = 0; i < count; ++i) {
      const auto& map = mappings[i];
      auto& state = states[i];

      int nextValue = static_cast<int>(m->params[map.param].getValue() + 0.5f);

      if (state.currentValue == INT_MIN || nextValue != state.currentValue) {
        state.currentValue = nextValue;

        uint8_t program = map.midiProogram[nextValue];
        setMidiMessage(out, midiChannel, program);

        return true; // send only one per call
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
    const ButtonMapping* mappings;
    size_t count;
    std::vector<ButtonState> states;
};




} // namespace zox
