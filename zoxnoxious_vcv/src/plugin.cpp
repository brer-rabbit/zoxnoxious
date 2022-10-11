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

std::string getCardOutputName(uint8_t cardId, int outputNumber, int slot) {
    std::string cardName;
    char slotName = 'A' + slot;
    std::string slotNameString(1, slotName);

    if (cardId == 0x01) {
        cardName = "Audio Out";
    }
    else if (cardId == 0x02) {
        cardName = "3340";
    }
    else {
        return "----";
    }

    return cardName +
        (outputNumber == 1 ? " Out 1" : (outputNumber == 2 ? " Out 2" : "") ) +
        (slot == -1 ? "" : ( ":" + slotNameString ));
}


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
        p->addModel(modelZoxnoxious3340);
        p->addModel(modelPatchingMatrix);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
