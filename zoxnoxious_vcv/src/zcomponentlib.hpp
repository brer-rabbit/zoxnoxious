#include "componentlibrary.hpp"


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


//
// Text Display box
//

static const int cardTextFontSize = 8;
static const int cardTextLetterSpacing = 0;
static const int cardTextLeftMargin = 2;

struct CardTextDisplay : TransparentWidget {

    void draw(const DrawArgs& args) override {
        const auto vg = args.vg;

        // Save the drawing context to restore later
        nvgSave(vg);

        // Draw dark background
        nvgBeginPath(vg);
        //nvgRotate(vg, -3.1416 / 4.0);
        nvgRect(vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(vg, nvgRGBA(20, 20, 20, 255));
        nvgFill(vg);

        //
        // Draw track names
        //
        if (1) {
            std::string to_display("VCO 3340 (1)");

            // If the track name is not empty, then display it
            if(to_display != "")  {
                // Set up font parameters
                nvgFontSize(vg, cardTextFontSize);
                nvgTextLetterSpacing(vg, cardTextLetterSpacing);

                nvgFillColor(vg, nvgRGBA(255, 215, 20, 0xff));
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

                float bounds[4];
                nvgTextBoxBounds(vg, cardTextLeftMargin, 10, 100.0, to_display.c_str(), NULL, bounds);
                float textHeight = bounds[3];
                nvgTextBox(vg, cardTextLeftMargin, (box.size.y / 2.0f) - (textHeight / 2.0f) + 8, 100.0, to_display.c_str(), NULL);
            }
        }

        nvgRestore(vg);
    }

};
