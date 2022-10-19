#pragma once
#include <rack.hpp>


using namespace rack;

/** getCardOutputName
 * provide a string to name the card's output.  Inputs:
 * cardId : numeric id for card
 * outputNumber : cards have two outputs, 1 or 2.  Any other value suppresses outputNumber
 * slot : card slot (which is also midi channel).  use -1 to suppress slot in the string.
 */
std::string getCardOutputName(uint8_t cardId, int outputNumber, int slot);


// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
//extern Model* modelMyModule;
extern Model* modelZoxnoxious3340;
extern Model* modelPatchingMatrix;
