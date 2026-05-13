#pragma once
#include <rack.hpp>


using namespace rack;

/** getCardOutputName
 * provide a string to name the card's output.  Inputs:
 * cardId : numeric id for card
 * outputNumber : cards have two outputs, 1 or 2.  Any other value suppresses outputNumber
 * slot : card slot (which is also midi channel).  use -1 to suppress slot in the string.
 * Example: "A1 3340 VCO"
 *
 * getCardName returns the card signal for the output number without the slot or output number in the string.
 * Example: "3340 VCO"
 *
 */
std::string getCardOutputName(uint8_t cardId, int outputNumber, int slot);
std::string getCardName(uint8_t cardId, int outputNumber);


// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelPoleDancer;
extern Model* modelPoleDancerPersonality;
extern Model* modelZoxnoxious3340;
extern Model* modelZoxnoxious3372;
extern Model* modelZoxnoxious5524;
extern Model* modelOutputInterface;
extern Model* modelPeepingTom;
