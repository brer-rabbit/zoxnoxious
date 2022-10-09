#include "plugin.hpp"


Plugin* pluginInstance;


std::string getCardOutputName(uint8_t cardId, int outputNumber, int slot) {
    std::string cardName;
    if (cardId == 0x01) {
        cardName = "Audio Out";
    }
    else if (cardId == 0x02) {
        cardName = "3340";
    }
    else {
        cardName = "<unknown>";
    }

    return (slot == -1 ? "" : ( "(" + std::to_string(slot) + ") " )) +
        cardName +
        (outputNumber == 1 ? " Out 1" : (outputNumber == 2 ? " Out 2" : "") );
}


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
        p->addModel(modelZoxnoxious3340);
        p->addModel(modelPatchingMatrix);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
