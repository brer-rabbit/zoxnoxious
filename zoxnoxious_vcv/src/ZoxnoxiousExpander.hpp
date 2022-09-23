#pragma once
#include <iostream>

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
    bool commandReceived;
};



struct ZoxnoxiousModule : Module {

private:
    bool validRightExpander;
    bool validLeftExpander;

protected:
    ZoxnoxiousCommandBus commandOutput_a;
    ZoxnoxiousCommandBus commandOutput_b;

public:
    ZoxnoxiousModule() : validRightExpander(false) {
        commandOutput_a.channelAssignments[0] = { 0, false };
        commandOutput_a.channelAssignments[1] = { 6, false };
        commandOutput_a.channelAssignments[2] = { 12, false };
        commandOutput_a.channelAssignments[3] = { 18, false };
        commandOutput_a.commandReceived = false;
        commandOutput_b.channelAssignments[0] = { 0, false };
        commandOutput_b.channelAssignments[1] = { 6, false };
        commandOutput_b.channelAssignments[2] = { 12, false };
        commandOutput_b.channelAssignments[3] = { 18, false };
        commandOutput_b.commandReceived = false;

        // from Module:
        leftExpander.producerMessage = &commandOutput_a;
        leftExpander.consumerMessage = &commandOutput_b;
    }


    void onExpanderChange (const ExpanderChangeEvent &e) override {
        // if the expander change is on the right side,
        // and it's a module we recognize,
        // reset the channel assignments and send a new list
        Expander expander = e.side? getRightExpander() : getLeftExpander();

        // expander changed: for now, just be concerned if we've got a valid expansion
        // on the right.
        if (e.side) { // right side
            // if we previously had a validRightExpander, it's been replaced and we should reset state
            if (validRightExpander) {
                INFO("Z Expander: replacing a valid right expander");
            }

            if (dynamic_cast<ZoxnoxiousModule*>(expander.module) != NULL) {
                // hey, this module looks familiar
                validRightExpander = true;
                INFO("Z Expander: valid right expander");
            }
            else {
                validRightExpander = false;
                INFO("Z Expander: invalid right expander");
            }

        }

    }


    void process(const ProcessArgs &args) override {
        Expander rightExpander = getRightExpander();

        if (rightExpander.module != NULL && validRightExpander) {
            ZoxnoxiousCommandBus *message = static_cast<ZoxnoxiousCommandBus*>(rightExpander.module->leftExpander.producerMessage);
                
            message->commandReceived = true;
            rightExpander.messageFlipRequested = true;

            if (APP->engine->getFrame() % 40000 == 0) {
                INFO("Z Expander: requested message flip");
            }
        }

    }


};
