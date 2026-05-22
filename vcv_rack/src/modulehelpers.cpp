#include "modulehelpers.hpp"

namespace zox {


struct AddZoxExpanderItem : MenuItem {
  Model* expanderModel = nullptr;
  Vec targetPos;

  void onAction(const event::Action& e) override {
    Module* expanderModule = expanderModel->createModule();
    APP->engine->addModule(expanderModule);

    ModuleWidget* expanderWidget =
      expanderModel->createModuleWidget(expanderModule);

    if (expanderWidget) {
      APP->scene->rack->setModulePosNearest(expanderWidget, targetPos);
      APP->scene->rack->addModule(expanderWidget);

      history::ModuleAdd* h = new history::ModuleAdd;
      h->name = "add expander";
      h->setModule(expanderWidget);
      APP->history->push(h);
    }
  }
};



void InstantiateExpanderItem::onAction(const event::Action &e) {
  module = model->createModule();
  APP->engine->addModule(module);

  ModuleWidget* mw = model->createModuleWidget(module);
  if (mw) {
    APP->scene->rack->setModulePosNearest(mw, posit);
    APP->scene->rack->addModule(mw);

    history::ModuleAdd *h = new history::ModuleAdd;
    h->name = "create expander module";
    h->setModule(mw);
    APP->history->push(h);
  }
}

}
