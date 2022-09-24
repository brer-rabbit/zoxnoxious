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

static const int maxCards = 8;

struct ChannelAssignment {
    unsigned char cardId; // physical card's identifier
    int channelOffset;  // offset to write channel to the controlbus frame
    bool assignmentOwned; // true if a card claimed this assignment
};

struct ZoxnoxiousCommandBus {
    struct ChannelAssignment channelAssignments[maxCards];
    bool authoritativeSource;
    int test;
};


static const ZoxnoxiousCommandBus commandEmpty =
  {
      // channelAssignmens
      { { 0, 0, false}, { 0, 0, false}, { 0, 0, false}, { 0, 0, false},
        { 0, 0, false}, { 0, 0, false}, { 0, 0, false}, { 0, 0, false}
      },
      // authoritativeSource
      false,
      // test int
      1
  };
      
    

// What an expander can do:
//
// in constructor:
// init/setup memory for right & left / producer & consumer messages
//
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

 
public:
    ZoxnoxiousModule() : validRightExpander(false), validLeftExpander(false) {
        leftExpander.producerMessage = &zCommand_a;
        leftExpander.consumerMessage = &zCommand_b;

        initCommandBus();

        //rightExpander.producerMessage = &controlProducerRight;
        //rightExpander.consumerMessage = &controlConsumerRight;
    }


    void onExpanderChange (const ExpanderChangeEvent &e) override {
        // if the expander change is on the right side,
        // and it's a module we recognize,
        // reset the channel assignments and send a new list
        Expander expander;
        bool *validExpander;

        if (e.side) {  // right
            expander = getRightExpander();
            validExpander = &validRightExpander;
        }
        else {  // left
            expander = getLeftExpander();
            validExpander = &validLeftExpander;
        }

        // if we previously had a validExpander, it's been replaced
        if (*validExpander) {
            INFO("Z Expander: replacing a valid %s expander", e.side ? "right" : "left");
        }

        if (dynamic_cast<ZoxnoxiousModule*>(expander.module) != NULL) {
            // hey, this module looks familiar
            *validExpander = true;
            INFO("Z Expander: valid %s expander", e.side ? "right" : "left");
        }
        else {
            // I don't know you
            *validExpander = false;
            INFO("Z Expander: invalid %s expander", e.side ? "right" : "left");
        }

        // command bus changed (added or removed) on the right- reset/mark (un)authoritative
        // so this passes along to the left 
        initCommandBus();
        // thinking this isn't necessary as it's already set:
        //leftExpander.producerMessage = &zCommand_a;
        //leftExpander.consumerMessage = &zCommand_b;
    }


protected:
    ZoxnoxiousCommandBus zCommand_a;
    ZoxnoxiousCommandBus zCommand_b;

    /** processExpander
     * to make use of the expander, this method ought to be
     * called from the derived class's process()
     */
    virtual void processExpander(const ProcessArgs &args) {

        Expander rightExpander = getRightExpander();

        // The _only_ thing you can do with the Module pointer is read
        // from the consumerMessage.  Write back to it via the
        // producerMessage on local Expander and call set messageFlipRequested.
        if (rightExpander.module != NULL && validRightExpander) {
            ZoxnoxiousCommandBus *rightConsumerMessage = static_cast<ZoxnoxiousCommandBus*>(rightExpander.module->leftExpander.consumerMessage);
                
            // Do any reading of the rightConsumerMessage here Copy
            // the right's consumer message to our left producer
            // message, effectively daisy chaining it to the left.  To
            // do the copy, determine which buffer is the
            // ProducerMessage and write to it.
            ZoxnoxiousCommandBus *leftExpanderProducerMessage =
                leftExpander.producerMessage == &zCommand_a ? &zCommand_a : &zCommand_b;

            *leftExpanderProducerMessage = *rightConsumerMessage; // copy / daisy chain

            // TODO: extract channel assignment, claim ownership.  Do
            // this by iterating over the leftExpanderProducerMessage,
            // finding if we own anything there.



            leftExpanderProducerMessage->test++;
            leftExpander.messageFlipRequested = true;


            if (APP->engine->getFrame() % 60000 == 0) {
                INFO("Z Expander: frame %ld : module id %ld : requested message flip: authoritative: %d : zCommand_a int: %d", APP->engine->getFrame(), getId(), leftExpanderProducerMessage->authoritativeSource, leftExpanderProducerMessage->test);
            }
        }

    }


    /** initCommandBus
     *
     * set both zCommand_a and zCommand_b to an (identical) initial
     * state.  Since the data is synthesized here, it gets marked as
     * from a not-authoritative source.
     */
    virtual void initCommandBus() {
        zCommand_a = commandEmpty;
        zCommand_b = commandEmpty;
    }

};
