#pragma once
#include "plugin.hpp"

const int maxChannels = 31;


/** ZoxnoxiousControlVoltageBusMessage:
 * messages originating from hardware cards (expansion modules) are
 * received from the left.  The expansion module populates channels it
 * is responsible for and passes the message to the right.
 */

class ZoxnoxiousControlVoltageBusMessage {
public:
    float frame[maxChannels];
    // TODO: add MIDI event
};
