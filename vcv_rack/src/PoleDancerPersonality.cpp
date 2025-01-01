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
    personalityNameString(names[rand() % sizeof(names)/sizeof(std::string)]), dirty(true) {

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
    personalityNameString = names[rand() % sizeof(names)/sizeof(std::string)];
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


struct PersonalityTextField : LedDisplayTextField {
  PoleDancerPersonality* module;

  void step() override {
    LedDisplayTextField::step();
    if (module && module->dirty) {
      setText(module->personalityNameString);
      module->dirty = false;
    }
  }

  void onChange(const ChangeEvent& e) override {
    LedDisplayTextField::onChange(e);
    if (module) {
      module->personalityNameString = getText();
    }
  }

  // this isn't quite right- cursor position doesn't match where
  // characters go.  Not sure how to fix that just yet.
  void draw(const DrawArgs& args) override {
    LedDisplayTextField::draw(args);
    const auto vg = args.vg;

    // Save the drawing context to restore later
    nvgSave(vg);

    // Draw dark background
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, box.size.x, box.size.y);
    nvgFillColor(vg, nvgRGBA(20, 20, 20, 255));
    nvgFill(vg);

    // If the track name is not empty, then display it
    if (module)  {
      // Set up font parameters
      nvgFontSize(vg, 9);
      nvgTextLetterSpacing(vg, 0);

      //nvgFillColor(vg, nvgRGBA(255, 215, 20, 0xff));
      nvgFillColor(vg, nvgRGBA(255, 105, 180, 0xff));
      nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

      float bounds[4];
      nvgTextBoxBounds(vg, 2, 10, 100.0, getText().c_str(), NULL, bounds);
      float textHeight = bounds[3];
      nvgTextBox(vg, 2, (box.size.y / 2.0f) - (textHeight / 2.0f) + 8, 100.0, getText().c_str(), NULL);
    }

    nvgRestore(vg);
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
    personalityDisplay->setModule(module);
    addChild(personalityDisplay);

  }

};

Model* modelPoleDancerPersonality = createModel<PoleDancerPersonality, PoleDancerPersonalityWidget>("PoleDancerPersonality");
