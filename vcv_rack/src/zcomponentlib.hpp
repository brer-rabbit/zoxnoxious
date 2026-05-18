#include "componentlibrary.hpp"


// plain simple UI skinning for ports and screws

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


// colors for lights within the style

struct ZoxAmberLight : GrayModuleLightWidget {
  ZoxAmberLight() {
    addBaseColor(nvgRGB(0xff, 0x9a, 0x35));
  }
};



//
// ZPushButtons -- Medium and Small
//

struct ZPushButtonSmallSvg {
  static constexpr const char* unlatched = "res/ZPushButtonSmall_unlatched.svg";
  static constexpr const char* latched   = "res/ZPushButtonSmall_latched.svg";
};

struct ZPushButtonMediumSvg {
  static constexpr const char* unlatched = "res/ZPushButtonMedium_unlatched.svg";
  static constexpr const char* latched   = "res/ZPushButtonMedium_latched.svg";
};

struct ZPushButtonMediumLeftSvg {
  static constexpr const char* unlatched = "res/ZPushButtonMedium_left_unlatched.svg";
  static constexpr const char* latched   = "res/ZPushButtonMedium_latched.svg";
};

struct ZPushButtonMediumRightSvg {
  static constexpr const char* unlatched = "res/ZPushButtonMedium_right_unlatched.svg";
  static constexpr const char* latched   = "res/ZPushButtonMedium_latched.svg";
};

struct ZPushButtonMediumUpSvg {
  static constexpr const char* unlatched = "res/ZPushButtonMedium_up_unlatched.svg";
  static constexpr const char* latched   = "res/ZPushButtonMedium_latched.svg";
};

struct ZPushButtonMediumDownSvg {
  static constexpr const char* unlatched = "res/ZPushButtonMedium_down_unlatched.svg";
  static constexpr const char* latched   = "res/ZPushButtonMedium_latched.svg";
};


template <typename TSvg>
struct ZPushButton : app::SvgSwitch {
  ZPushButton() {
    momentary = true;
    addFrame(Svg::load(asset::plugin(pluginInstance, TSvg::unlatched)));
    addFrame(Svg::load(asset::plugin(pluginInstance, TSvg::latched)));
  }
};


// Stateful latch, no light:
template <typename TSvg>
struct ZPushButtonStatefulLatch : app::SvgSwitch {
  std::shared_ptr<Svg> offSvg;
  std::shared_ptr<Svg> onSvg;

  ZPushButtonStatefulLatch() {
    momentary = false;
    latch = true;

    offSvg = Svg::load(asset::plugin(pluginInstance, TSvg::unlatched));
    onSvg  = Svg::load(asset::plugin(pluginInstance, TSvg::latched));

    addFrame(offSvg);
    addFrame(onSvg);
  }

  void step() override {
    app::SvgSwitch::step();

    engine::ParamQuantity* pq = this->getParamQuantity();
    if (pq) {
      bool on = pq->getValue() > 0.5f;
        if (on) {
          this->sw->setSvg(onSvg);
          this->shadow->opacity = 0.f;
        }
        else {
          this->sw->setSvg(offSvg);
          this->shadow->opacity = 0.15f;
        }
    }
  }
};



// stateful latch with light:
template <typename TSvg, typename TLight = MediumSimpleLight<WhiteLight>>
  struct ZPushButtonStatefulLightLatch : app::SvgSwitch {
    app::ModuleLightWidget* light;

    std::shared_ptr<Svg> offSvg;
    std::shared_ptr<Svg> onSvg;

    ZPushButtonStatefulLightLatch() {
      momentary = false;
      latch = true;

      offSvg = Svg::load(asset::plugin(pluginInstance, TSvg::unlatched));
      onSvg  = Svg::load(asset::plugin(pluginInstance, TSvg::latched));

      addFrame(offSvg);
      addFrame(onSvg);

      light = new TLight;
      light->box.pos = box.size.div(2).minus(light->box.size.div(2));
      addChild(light);
    }

    void step() override {
      app::SvgSwitch::step();

      engine::ParamQuantity* pq = this->getParamQuantity();
      if (pq) {
        bool on = pq->getValue() > 0.5f;
        if (on) {
          this->sw->setSvg(onSvg);
          this->shadow->opacity = 0.f;
        }
        else {
          this->sw->setSvg(offSvg);
          this->shadow->opacity = 0.15f;
        }
      }
    }

    app::ModuleLightWidget* getLight() {
      return light;
    }
};


// names to reference these by for use:
using ZPushButtonSmall = ZPushButton<ZPushButtonSmallSvg>;
using ZPushButtonMedium = ZPushButton<ZPushButtonMediumSvg>;

template <typename TLight = WhiteLight>
using ZLightPushButtonSmall = LightButton<ZPushButtonSmall, TLight>;

template <typename TLight = WhiteLight>
using ZLightPushButtonMedium = LightButton<ZPushButtonMedium, TLight>;

template <typename TLight>
using ZPushButtonSmallStatefulLightLatch =
	ZPushButtonStatefulLightLatch<ZPushButtonSmallSvg, TLight>;

template <typename TLight>
using ZPushButtonMediumStatefulLightLatch =
	ZPushButtonStatefulLightLatch<ZPushButtonMediumSvg, TLight>;

using ZPushButtonSmallStatefulLatch =
	ZPushButtonStatefulLatch<ZPushButtonSmallSvg>;

using ZPushButtonMediumStatefulLatch =
	ZPushButtonStatefulLatch<ZPushButtonMediumSvg>;

using ZPushButtonMediumLeft = ZPushButton<ZPushButtonMediumLeftSvg>;
using ZPushButtonMediumRight = ZPushButton<ZPushButtonMediumRightSvg>;
using ZPushButtonMediumUp = ZPushButton<ZPushButtonMediumUpSvg>;
using ZPushButtonMediumDown = ZPushButton<ZPushButtonMediumDownSvg>;




static constexpr float backgroundWidth = 2.5f;
static constexpr float x = backgroundWidth / 2.f;
static constexpr float grooveBottom = 62.f;
static constexpr float grooveTop = -5.f;
static constexpr float handleHeight = 0.f;
static constexpr float yMin = grooveBottom - handleHeight / 2.f;
static constexpr float yMax = grooveTop    + handleHeight / 2.f;

// slider model
struct ZoxSlider : app::SvgSlider {
  ZoxSlider() {
    setBackgroundSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ZoxSlider.svg")));
    setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ZoxSliderHandle.svg")));
    setHandlePos(math::Vec(x, yMin), math::Vec(x, yMax));
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
  const std::string *displayString = nullptr;
  std::shared_ptr<Font> font;
  std::string allSegments;

  CardTextDisplay() : allSegments(cardDefaultNumChars, '~') {
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/DSEG14Classic-BoldItalic.ttf"));
  }

  void setNumChars(size_t count) {
    allSegments.resize(count, '~');
  }

  void setText(const std::string *theString) {
    displayString = theString;
  }


  void draw(const DrawArgs& args) override {
    const auto vg = args.vg;

    // Save the drawing context to restore later
    nvgSave(vg);

    // Draw dark background
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, box.size.x, box.size.y);
    nvgFillColor(vg, nvgRGBA(0x3A, 0x1A, 0x00, 0xFF));
    nvgFill(vg);

    // If the track name is not empty, then display it
    if (displayString)  {
      const char *text = displayString->c_str();

      bndSetFont(font->handle);
      nvgFontFaceId(vg, font->handle);
      nvgTextLetterSpacing(vg, 0);

      // Set up font parameters
      nvgFontSize(vg, cardTextFontSize);
      nvgTextLetterSpacing(vg, cardTextLetterSpacing);

      //nvgFillColor(vg, nvgRGBA(0xff, 0x90, 0x10, 0xff));
      nvgFillColor(vg, nvgRGBA(0xff, 0xB0, 0x10, 0xff));
      nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);

      float bounds[4];
      nvgTextBoxBounds(vg, cardTextLeftMargin, 10, 100.0, text, NULL, bounds);
      float textHeight = bounds[3];
      nvgTextBox(vg, cardTextLeftMargin, (box.size.y / 2.0f) - (textHeight / 2.0f) + 8, 100.0, text, NULL);

      // light all segments of 14-segment LED with a transparency
      nvgFillColor(vg, nvgRGBA(255, 0x90, 0x10, 0x20));
      nvgTextBox(vg, cardTextLeftMargin, (box.size.y / 2.0f) - (textHeight / 2.0f) + 8, 100.0, allSegments.c_str(), NULL);

      bndSetFont(APP->window->uiFont->handle);
    }

    nvgRestore(vg);
  }

};
