#pragma once
#include <iostream>

#include <rack.hpp>

const int maxChannels = 31;

/** ZoxnoxiousControlBus:
 * messages originating from hardware cards (expansion modules) are
 * received from the left.  The expansion module populates channels it
 * is responsible for and passes the message to the right.
 */

class ZoxnoxiousControlBus {
public:
    float frame[maxChannels];
    // TODO: add MIDI event
};


/** ZoxnoxiousCommandBus 
 * messages originating from backplane card (the only required card)
 * are received from the right expansion.  The purpose is to define
 * channel ownership for signals on the ZoxnoxiousControlBus.
 */

struct ChannelAssignment {
    //Model *model; // think about need here- should be int Id
    int channelOffset;
    bool owned;
};

struct ZoxnoxiousCommandBus {
    struct ChannelAssignment channelAssignments[4];
    bool commandReceived;
    int test;
};


// in constructor:
// init/setup right & left / producer & consumer messages to memory

// outside of constructor:
// can write to:
// rightExpander.producerMessage
// leftExpander.producerMessage
// can read from:
// rightExpander.module->leftExpander.consumerMessage
// leftExpander.module->rightExpander.consumerMessage


struct ZoxnoxiousModule : Module {

private:
    bool validRightExpander;
    bool validLeftExpander;
    //ZoxnoxiousControlBus controlProducerLeft;
    //ZoxnoxiousControlBus controlConsumerLeft;

    ZoxnoxiousCommandBus commandProducer_a;
    ZoxnoxiousCommandBus commandProducer_b;

public:
    ZoxnoxiousModule() : validRightExpander(false), validLeftExpander(false) {
        leftExpander.producerMessage = &commandProducer_a;
        leftExpander.consumerMessage = &commandProducer_b;

        // daisy chain the leftExpander received messages to
        // rightExpander
        commandProducer_a.channelAssignments[0] = { 0, false };
        commandProducer_a.channelAssignments[1] = { 6, false };
        commandProducer_a.channelAssignments[2] = { 12, false };
        commandProducer_a.channelAssignments[3] = { 18, false };
        commandProducer_a.commandReceived = false;
        commandProducer_a.test = 1;
        //rightExpander.producerMessage = &controlProducerRight;
        //rightExpander.consumerMessage = &controlConsumerRight;
    }


    void onExpanderChange (const ExpanderChangeEvent &e) override {
        // if the expander change is on the right side,
        // and it's a module we recognize,
        // reset the channel assignments and send a new list
        Expander expander;
        bool *validExpander;

        if (e.side) {
            expander = getRightExpander();
            validExpander = &validRightExpander;
        }
        else {
            expander = getLeftExpander();
            validExpander = &validLeftExpander;
        }

        // if we previously had a validExpander, it's been replaced and we should reset state
        if (*validExpander) {
            INFO("Z Expander: replacing a valid %s expander", e.side ? "right" : "left");
        }

        if (dynamic_cast<ZoxnoxiousModule*>(expander.module) != NULL) {
            // hey, this module looks familiar
            *validExpander = true;
            INFO("Z Expander: valid %s expander", e.side ? "right" : "left");
            // reset producer
            commandProducer_a.commandReceived = false;
            commandProducer_a.test = 1;

        }
        else {
            // I don't know you
            *validExpander = false;
            INFO("Z Expander: invalid %s expander", e.side ? "right" : "left");
        }

    }


protected:
    /** processExpander
     * to make use of the expander, this method ought to be
     * called from the derived class's process().
     */
    virtual void processExpander(const ProcessArgs &args) {

        Expander rightExpander = getRightExpander();

        // The _only_ thing you can do with the Module pointer is
        // write to its producerMessage or read
        // from its consumerMessage and then set messageFlipRequested
        if (rightExpander.module != NULL && validRightExpander) {
            ZoxnoxiousCommandBus *rightConsumerMessage = static_cast<ZoxnoxiousCommandBus*>(rightExpander.module->leftExpander.consumerMessage);
            ZoxnoxiousCommandBus *rightProducerMessage = static_cast<ZoxnoxiousCommandBus*>(rightExpander.module->leftExpander.producerMessage);
                
            // Do any reading of the rightConsumerMessage here
            // Copy the producer message (which we may have altered)
            // and daisy chain it to the left.
            commandProducer_a = *rightConsumerMessage;
            commandProducer_a.commandReceived = false;
            commandProducer_a.test++;


            leftExpander.producerMessage = &commandProducer_a;
            leftExpander.messageFlipRequested = true;



            if (APP->engine->getFrame() % 40000 == 0) {
                INFO("Z Expander: frame %ld : module id %ld : requested message flip: commandProducer_a int: %d", APP->engine->getFrame(), getId(), commandProducer_a.test);
            }
        }

    }


};
