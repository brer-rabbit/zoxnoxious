#include "plugin.hpp"

Plugin* pluginInstance;


// (A) card1 Out1
// (A) card1 Out2
// (B) card2 Out1
// (B) card2 Out2
// (C) card3 Out1
// (C) card3 Out2
// (D) card4 Out1
// (D) card4 Out2
// (E) card5 Out1
// (E) card5 Out2
// (F) card6 Out1
// (F) card6 Out2
// (G) card7 Out1
// (G) card7 Out2
// (H) card8 Out1
// (H) card8 Out2

std::string getCardName(uint8_t cardId, int outputNumber) {
  if (cardId == 0x01) return "Audio Out";
  if (cardId == 0x02) return "3340!VCO";
  if (cardId == 0x03) return "3372!VCF";
  if (cardId == 0x04) return outputNumber == 1 ? "5524!VCF" : "5524!VCO1";
  if (cardId == 0x06) return "Pole Dancer";
  if (cardId == 0x07) return "Audio IO";
  return "----";
}

std::string getCardOutputName(uint8_t cardId, int outputNumber, int slot) {
  char slotName = 'A' + slot;
  return string::f("%c%d!%s", slotName, outputNumber, getCardName(cardId, outputNumber).c_str());
}



void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
        p->addModel(modelPoleDancer);
        p->addModel(modelPoleDancerPersonality);
        p->addModel(modelZoxnoxious3372);
        p->addModel(modelZoxnoxious5524);
        p->addModel(modelZoxnoxious3340);
        p->addModel(modelOutputInterface);
        p->addModel(modelPeepingTom);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
