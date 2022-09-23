#pragma once
#include <rack.hpp>

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


/** ZoxnoxiousCommandBus 
 * messages originating from backplane card (the only required card)
 * are received from the right expansion.  The purpose is to define
 * channel ownership for signals on the ZoxnoxiousControlVoltageBusMessage.
 */

struct ChannelAssignment {
    //Model *model;
    int channelOffset;
    bool owned;
};

struct ZoxnoxiousCommandBus {
    struct ChannelAssignment channelAssignments[4];    
};



struct ZoxnoxiousModule : Module {



};
