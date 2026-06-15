#include "plugin.hpp"

#include "zcomponentlib.hpp"
#include "OutputInterface.hpp"

namespace zox {

struct OutputInterfaceVisualizer : Module {
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

  OutputInterfaceVisualizer() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    clockDivider.setDivision(512);
  }

  void process(const ProcessArgs& args) override {
    if (clockDivider.process()) {
      if (leftExpander.module && leftExpander.module->model == modelOutputInterface) {
        INFO("found OutputInterface");
      }
    }
  }

};


struct OutputInterfaceVisualizerWidget : ModuleWidget {

  OutputInterfaceVisualizerWidget(OutputInterfaceVisualizer* module) {
    setModule(module);

    setPanel(createPanel(asset::plugin(pluginInstance, "res/OutputInterfaceVisualizer.svg")));

    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSlottedKnurled>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    if (module) {
    }
  }
};


} // namespace zox

Model* modelOutputInterfaceVisualizer = createModel<zox::OutputInterfaceVisualizer, zox::OutputInterfaceVisualizerWidget>("OutputInterfaceVisualizer");
