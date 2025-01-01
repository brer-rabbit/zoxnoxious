#include "plugin.hpp"
#include "zcomponentlib.hpp"

std::string names[] = { "Lola", "Candy", "Ginger", "Anastasia", "Cherry", "Destiny", "Scarlett", "Bambi", "Trixie", "Diamond", "Sky", "Delight", "Jezebel", "Journey", "Kitty", "Roxanne", "Portia" };

static const int numVoltages = 5;

struct PoleDancerPersonality : Module {
  enum ParamId {
    DRY_MIX_KNOB_PARAM,
    POLE1_MIX_KNOB_PARAM,
    POLE2_MIX_KNOB_PARAM,
    POLE3_MIX_KNOB_PARAM,
    POLE4_MIX_KNOB_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    INPUTS_LEN
  };
  enum OutputId {
    POLE_MIX_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };

  std::string personalityNameString;
  bool dirty;

  dsp::ClockDivider clockDivider;
  float voltages[numVoltages];

  PoleDancerPersonality() :
    personalityNameString(names[APP->engine->getFrame() % sizeof(names)/sizeof(std::string)]), dirty(true) {

    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(DRY_MIX_KNOB_PARAM, 0.f, 10.f, 0.f, "Dry Mix", "%", 0.f, 10.f);
    configParam(POLE1_MIX_KNOB_PARAM, 0.f, 10.f, 0.f, "Pole 1 Mix", "%", 0.f, 10.f);
    configParam(POLE2_MIX_KNOB_PARAM, 0.f, 10.f, 0.f, "Pole 2 Mix", "%", 0.f, 10.f);
    configParam(POLE3_MIX_KNOB_PARAM, 0.f, 10.f, 0.f, "Pole 3 Mix", "%", 0.f, 10.f);
    configParam(POLE4_MIX_KNOB_PARAM, 0.f, 10.f, 10.f, "Pole 4 Mix", "%", 0.f, 10.f);
    configOutput(POLE_MIX_OUTPUT, "Pole Mix Voltage Series");

    clockDivider.setDivision(512);
    memset(voltages, 0, sizeof(float) * numVoltages);
  }


  void process(const ProcessArgs& args) override {
    if (clockDivider.process() && outputs[POLE_MIX_OUTPUT].isConnected()) {
      for (int i = 0; i < numVoltages; ++i) {
        voltages[i] = params[DRY_MIX_KNOB_PARAM + i].getValue();
      }

      outputs[POLE_MIX_OUTPUT].setChannels(numVoltages);
      outputs[POLE_MIX_OUTPUT].writeVoltages(voltages);
    }
  }


  void onReset() override {
    // using getFrame as a rand source
    personalityNameString = names[APP->engine->getFrame() % sizeof(names)/sizeof(std::string)];
    dirty = true;
  }


  void fromJson(json_t* rootJ) override {
    Module::fromJson(rootJ);
    json_t* textJ = json_object_get(rootJ, "personalityNameString");
    if (textJ) {
      personalityNameString = json_string_value(textJ);
    }
    dirty = true;
  }

  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "personalityNameString", json_stringn(personalityNameString.c_str(), personalityNameString.size()));
    return rootJ;
  }

  void dataFromJson(json_t* rootJ) override {
    json_t* textJ = json_object_get(rootJ, "personalityNameString");
    if (textJ) {
      personalityNameString = json_string_value(textJ);
    }
    dirty = true;
  }


};


static const int fontSize = 9;

struct PersonalityTextField : LedDisplayTextField {
  PoleDancerPersonality* module;

  // override to set color & posiiton
  PersonalityTextField() {
    LedDisplayTextField();
    color = nvgRGB(255, 105, 180);
    textOffset = math::Vec(0, -6);
  }

  void step() override {
    LedDisplayTextField::step();
    if (module && module->dirty) {
      setText(module->personalityNameString);
      module->dirty = false;
    }
  }

  void onChange(const ChangeEvent& e) override {
    if (module) {
      module->personalityNameString = getText();
    }
  }

  // override here to set text size
  void drawLayer(const DrawArgs& args, int layer) override {
    nvgScissor(args.vg, RECT_ARGS(args.clipBox));

    if (layer == 1) {
      // Text
      std::shared_ptr<window::Font> font = APP->window->loadFont(fontPath);
      if (font && font->handle >= 0) {
        bndSetFont(font->handle);

        NVGcolor highlightColor = color;
        highlightColor.a = 0.5;
        int begin = std::min(cursor, selection);
        int end = (this == APP->event->selectedWidget) ? std::max(cursor, selection) : -1;
        bndIconLabelCaret(args.vg,
                          textOffset.x, textOffset.y,
                          box.size.x - 2 * textOffset.x, box.size.y - 2 * textOffset.y,
                          -1, color, fontSize, text.c_str(), highlightColor, begin, end);

        bndSetFont(APP->window->uiFont->handle);
      }
    }

    Widget::drawLayer(args, layer);
    nvgResetScissor(args.vg);
  }


  // override here to set text size
  int getTextPosition(math::Vec mousePos) override {
    std::shared_ptr<window::Font> font = APP->window->loadFont(fontPath);
    if (!font || !font->handle)
      return 0;

    bndSetFont(font->handle);
    int textPos = bndIconLabelTextPosition(APP->window->vg,
                                           textOffset.x, textOffset.y,
                                           box.size.x - 2 * textOffset.x, box.size.y - 2 * textOffset.y,
                                           -1, fontSize, text.c_str(), mousePos.x, mousePos.y);
    bndSetFont(APP->window->uiFont->handle);
    return textPos;
  }



};


struct PersonalityDisplay : LedDisplay {
  void setModule(PoleDancerPersonality* module) {
    PersonalityTextField *textField = createWidget<PersonalityTextField>(Vec(0, 0));
    textField->box.size = box.size;
    textField->multiline = false;
    textField->module = module;
    addChild(textField);
  }
};


struct PoleDancerPersonalityWidget : ModuleWidget {
  PoleDancerPersonalityWidget(PoleDancerPersonality* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/PoleDancerPersonality.svg")));

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.760, 18.524)), module, PoleDancerPersonality::DRY_MIX_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.760, 33.120)), module, PoleDancerPersonality::POLE1_MIX_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.760, 47.670)), module, PoleDancerPersonality::POLE2_MIX_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.760, 62.273)), module, PoleDancerPersonality::POLE3_MIX_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.760, 76.849)), module, PoleDancerPersonality::POLE4_MIX_KNOB_PARAM));

    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.213, 121.907)), module, PoleDancerPersonality::POLE_MIX_OUTPUT));

    // personality name
    PersonalityDisplay *personalityDisplay = createWidget<PersonalityDisplay>(mm2px(Vec(3.457, 8.411)));
    personalityDisplay->box.size = mm2px(Vec(23.513, 3.636));
    //personalityDisplay->box.size = mm2px(Vec(35, 10));
    personalityDisplay->setModule(module);
    addChild(personalityDisplay);

  }

};

Model* modelPoleDancerPersonality = createModel<PoleDancerPersonality, PoleDancerPersonalityWidget>("PoleDancerPersonality");
