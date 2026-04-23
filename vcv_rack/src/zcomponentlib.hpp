#include "componentlibrary.hpp"



struct BNCPort : app::SvgPort {
  BNCPort() {
    setSvg(Svg::load(asset::plugin(pluginInstance, "res/BNCFemale.svg")));
  }
};


struct ScrewSlottedKnurled : app::SvgScrew {
  ScrewSlottedKnurled() {
    setSvg(Svg::load(asset::plugin(pluginInstance, "res/ScrewSlottedKnurled.svg")));
  }
};

//
// Button - VCV Rack button at about 2/3 the size
//

struct ZButton : app::SvgSwitch {
  ZButton() {
    momentary = true;
    addFrame(Svg::load(asset::plugin(pluginInstance, "res/ZButtonSmall_0.svg")));
    addFrame(Svg::load(asset::plugin(pluginInstance, "res/ZButtonSmall_1.svg")));
  }
};


struct ZLatch : ZButton {
  ZLatch() {
    momentary = false;
    latch = true;
  }
};

template <typename TLight>
struct ZLightButton : ZButton {
  app::ModuleLightWidget* light;

  ZLightButton() {
    light = new TLight;
    // Move center of light to center of box
    light->box.pos = box.size.div(2).minus(light->box.size.div(2));
    addChild(light);
  }

  app::ModuleLightWidget* getLight() {
    return light;
  }
};


template <typename TLight>
struct ZLightLatch : ZLightButton<TLight> {
  ZLightLatch() {
    this->momentary = false;
    this->latch = true;
  }
};







/** TriangleLeftLight
 * point left.  Usage is like other light sources, eg-
 * createLightCentered<TriangleLeftLight<MediumLight<RedLight>>>
 */
template <typename TBase>
struct TriangleLeftLight : TBase {
  void drawBackground(const widget::Widget::DrawArgs& args) override {
    // Derived from LightWidget::drawBackground()

    nvgBeginPath(args.vg);
    nvgMoveTo(args.vg, this->box.size.x, this->box.size.y);
    nvgLineTo(args.vg, 0, this->box.size.y / 2);
    nvgLineTo(args.vg, this->box.size.x, 0);
    nvgLineTo(args.vg, this->box.size.x, this->box.size.y);
    nvgClosePath(args.vg);

    // Background
    if (this->bgColor.a > 0.0) {
      nvgFillColor(args.vg, this->bgColor);
      nvgFill(args.vg);
    }

    // Border
    if (this->borderColor.a > 0.0) {
      nvgStrokeWidth(args.vg, 0.5);
      nvgStrokeColor(args.vg, this->borderColor);
      nvgStroke(args.vg);
    }
  }

  void drawLight(const widget::Widget::DrawArgs& args) override {
    // Derived from LightWidget::drawLight()

    // Foreground
    if (this->color.a > 0.0) {
      nvgBeginPath(args.vg);
      nvgMoveTo(args.vg, this->box.size.x, this->box.size.y);
      nvgLineTo(args.vg, 0, this->box.size.y / 2);
      nvgLineTo(args.vg, this->box.size.x, 0);
      nvgLineTo(args.vg, this->box.size.x, this->box.size.y);
      nvgClosePath(args.vg);

      nvgFillColor(args.vg, this->color);
      nvgFill(args.vg);
    }
  }
};


/** TriangleRightLight
 * point left.  Usage is like other light sources, eg-
 * createLightCentered<TriangleLeftLight<MediumLight<RedLight>>>
 * copied from RectangleLight.
 */
template <typename TBase>
struct TriangleRightLight : TBase {
  void drawBackground(const widget::Widget::DrawArgs& args) override {
    // Derived from LightWidget::drawBackground()
    nvgBeginPath(args.vg);
    nvgMoveTo(args.vg, 0, this->box.size.y);
    nvgLineTo(args.vg, this->box.size.x, this->box.size.y / 2);
    nvgLineTo(args.vg, 0, 0);
    nvgLineTo(args.vg, 0, this->box.size.y);
    nvgClosePath(args.vg);

    // Background
    if (this->bgColor.a > 0.0) {
      nvgFillColor(args.vg, this->bgColor);
      nvgFill(args.vg);
    }

    // Border
    if (this->borderColor.a > 0.0) {
      nvgStrokeWidth(args.vg, 0.5);
      nvgStrokeColor(args.vg, this->borderColor);
      nvgStroke(args.vg);
    }
  }

  void drawLight(const widget::Widget::DrawArgs& args) override {
    // Derived from LightWidget::drawLight()

    // Foreground
    if (this->color.a > 0.0) {
      nvgBeginPath(args.vg);
      nvgMoveTo(args.vg, 0, this->box.size.y);
      nvgLineTo(args.vg, this->box.size.x, this->box.size.y / 2);
      nvgLineTo(args.vg, 0, 0);
      nvgLineTo(args.vg, 0, this->box.size.y);
      nvgClosePath(args.vg);

      nvgFillColor(args.vg, this->color);
      nvgFill(args.vg);
    }
  }
};




//
// Text Display box
//

static const int cardTextFontSize = 6;
static const int cardTextLetterSpacing = 0;
static const int cardTextLeftMargin = 2;
static const int cardDefaultNumChars = 8;

struct CardTextDisplay : TransparentWidget {
  std::string *displayString;
  std::shared_ptr<Font> font;
  std::string allSegments;

  CardTextDisplay() : displayString(NULL), allSegments(cardDefaultNumChars, '~') {
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/DSEG14Classic-BoldItalic.ttf"));
  }

  void setNumChars(size_t count) {
    allSegments.resize(count, '~');
  }

  void draw(const DrawArgs& args) override {
    const auto vg = args.vg;

    // Save the drawing context to restore later
    nvgSave(vg);

    // Draw dark background
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, box.size.x, box.size.y);
    //nvgFillColor(vg, nvgRGBA(20, 20, 20, 255));
    nvgFillColor(vg, nvgRGBA(0x3A, 0x1A, 0x00, 0xFF));
    nvgFill(vg);

    // If the track name is not empty, then display it
    if (displayString)  {
      bndSetFont(font->handle);
      nvgFontFaceId(vg, font->handle);
      nvgTextLetterSpacing(vg, 0);

      // Set up font parameters
      nvgFontSize(vg, cardTextFontSize);
      nvgTextLetterSpacing(vg, cardTextLetterSpacing);

      //nvgFillColor(vg, nvgRGBA(255, 215, 20, 0xff));
      nvgFillColor(vg, nvgRGBA(255, 0x90, 0x10, 0xff));
      nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);

      float bounds[4];
      nvgTextBoxBounds(vg, cardTextLeftMargin, 10, 100.0, displayString->c_str(), NULL, bounds);
      float textHeight = bounds[3];
      nvgTextBox(vg, cardTextLeftMargin, (box.size.y / 2.0f) - (textHeight / 2.0f) + 8, 100.0, displayString->c_str(), NULL);

      // light all segments of 14-segment LED with a transparency
      nvgFillColor(vg, nvgRGBA(255, 0x90, 0x10, 0x20));
      nvgTextBox(vg, cardTextLeftMargin, (box.size.y / 2.0f) - (textHeight / 2.0f) + 8, 100.0, allSegments.c_str(), NULL);

      bndSetFont(APP->window->uiFont->handle);
    }

    nvgRestore(vg);
  }


  void setText(std::string *theString) {
    displayString = theString;
  }

};
