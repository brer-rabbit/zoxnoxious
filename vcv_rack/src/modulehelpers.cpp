#include "modulehelpers.hpp"

namespace zox {


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
