#pragma once
#include <iostream>

#include <rack.hpp>


const int maxChannels = 31;

/** ZoxnoxiousControlMsg:
 * messages originating from hardware cards (expansion modules) are
 * received from the left.  The expansion module populates channels it
 * is responsible for and passes the message to the right.
 * Only one midiMessage per clock tick is allowed- modules should queue
 * messages if the bus is in use.  The module is responsible for completely
 * clearing out the midiMessage before use.
 */

struct ZoxnoxiousControlMsg {
    float frame[maxChannels];
    bool midiMessageSet;
    midi::Message midiMessage;
};

static const uint8_t midiProgramChangeStatus = 0xC;
//static const ZoxnoxiousControlMsg controlEmpty = { .midiMessageSet = true };


/** ZoxnoxiousCommandMsg 
 * messages originating from backplane card (the only required card)
 * are received from the right expansion.  The purpose is to define
 * channel ownership for signals on the ZoxnoxiousControlMsg.
 */

static const int maxCards = 8;
static const int invalidSlot = maxCards;
static const int invalidCvChannelOffset = -1;
static const int invalidMidiChannel = -1;

struct ChannelAssignment {
    uint8_t cardId; // physical card's identifier
    int cvChannelOffset;  // offset to write channel to the controlmsg frame
    int midiChannel;
    bool assignmentOwned; // true if a card claimed this assignment
};

struct ZoxnoxiousCommandMsg {
    bool authoritativeSource;
    int test;
    struct ChannelAssignment channelAssignments[maxCards];
};


// this "empty command" is used for both initialization and when
static const ZoxnoxiousCommandMsg commandEmpty =
  {
      // authoritativeSource
      false,
      // test int
      1,
      // cardId, cvChannelOffset, midiChannel, assignmentOwned
      { { 0, -1, -1, false}, { 0, -1, -1, false}, { 0, -1, -1, false}, { 0, -1, -1, false},
        { 0, -1, -1, false}, { 0, -1, -1, false}, { 0, -1, -1, false}, { 0, -1, -1, false}
      }
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

public:
    ZoxnoxiousModule() :
        validRightExpander(false), validLeftExpander(false), hasChannelAssignment(false),
        cvChannelOffset(invalidCvChannelOffset), midiChannel(invalidMidiChannel), slot(invalidSlot) {

        // command
        leftExpander.producerMessage = &zCommand_a;
        leftExpander.consumerMessage = &zCommand_b;
        initCommandMsgState();

        // control
        rightExpander.producerMessage = &zControl_a;
        rightExpander.consumerMessage = &zControl_b;
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
            INFO("Z Expander: module id %" PRId64 " replacing a valid %s expander", getId(), e.side ? "right" : "left");
        }

        if (dynamic_cast<ZoxnoxiousModule*>(expander.module) != NULL) {
            // hey, this module looks familiar
            *validExpander = true;
            INFO("Z Expander: module id %" PRId64 " VALID %s expander", getId(), e.side ? "right" : "left");
        }
        else {
            // I don't know you
            *validExpander = false;
            INFO("Z Expander: module id %" PRId64 " invalid %s expander", getId(), e.side ? "right" : "left");
        }

        // command msg changed (added or removed) on the right- reset/mark (un)authoritative
        // so this passes along to the left 
        initCommandMsgState();
    }




protected:
    ZoxnoxiousControlMsg zControl_a;
    ZoxnoxiousControlMsg zControl_b;

    ZoxnoxiousCommandMsg zCommand_a;
    ZoxnoxiousCommandMsg zCommand_b;

    // data acquired from the ZoxnoxiousCommandMsg-
    // this data will be used when writing to the ZonxnoxiousControlMsg
    bool hasChannelAssignment;
    int cvChannelOffset;
    int midiChannel;
    int slot;


    /** processExpander
     * to make use of the expander, this method ought to be
     * called from the derived class's process()
     */
    virtual void processExpander(const ProcessArgs &args) {

        if (rightExpander.module != NULL && validRightExpander) {
            //
            // Left Consumer Message Passing to our RightExpander Producer
            //

            // find the correct right producer message to modify
            ZoxnoxiousControlMsg *rightExpanderProducerMessage =
                rightExpander.producerMessage == &zControl_a ? &zControl_a : &zControl_b;

            // get the message from the left, if we have a left expander, so it can be relayed
            if (leftExpander.module != NULL && validLeftExpander) {
                ZoxnoxiousControlMsg *leftExpanderConsumerMessage = static_cast<ZoxnoxiousControlMsg*>(leftExpander.module->rightExpander.consumerMessage);

                *rightExpanderProducerMessage = *leftExpanderConsumerMessage;  // daisy chain
            }

            // call the concrete module to fill in the control message
            processControlMessage(static_cast<ZoxnoxiousControlMsg*>(rightExpanderProducerMessage));
            rightExpander.messageFlipRequested = true;

            //
            // Right Consumer Message Passing to our LeftExpander Producer
            //

            ZoxnoxiousCommandMsg *rightExpanderConsumerMessage = static_cast<ZoxnoxiousCommandMsg*>(rightExpander.module->leftExpander.consumerMessage);
                
            // Do any reading of the rightExpanderConsumerMessage here.  Copy
            // the right's consumer message to our left producer
            // message.  Effectively, this daisy chains it to the
            // left.  To do the copy, determine which buffer point to
            // the ProducerMessage and write to it.
            ZoxnoxiousCommandMsg *leftExpanderProducerMessage =
                leftExpander.producerMessage == &zCommand_a ? &zCommand_a : &zCommand_b;

            *leftExpanderProducerMessage = *rightExpanderConsumerMessage; // copy/daisy chain

            // Extract channel assignment, claim ownership.
            // Do this by iterating over the leftExpanderProducerMessage,
            // finding if we own anything there.  This may/will modify
            // the message if we claim ownership of a
            // ChannelAssignment.
            processZoxnoxiousCommand(leftExpanderProducerMessage);

            leftExpanderProducerMessage->test++;
            leftExpander.messageFlipRequested = true;


            if (APP->engine->getFrame() % 60000 == 0) {
                //INFO("Z Expander: frame %" PRId64 ": module id %" PRId64 " : requested message flip: authoritative: %d : zCommand_a int: %d", APP->engine->getFrame(), getId(), leftExpanderProducerMessage->authoritativeSource, leftExpanderProducerMessage->test);
            }
        }

    }



    /** processControlMessage
     *
     * Intended  behavior is to fill in what channels we're responsible for.
     * The controlMsg will be modified.
     * Subclass will need to implement this.
     */
    virtual void processControlMessage(ZoxnoxiousControlMsg *controlMsg) = 0;



    /** getCardHardwareId
     * return the hardware Id of the card.  Derived class needs to implement this.
     */
    virtual uint8_t getHardwareId() = 0;


    /** processZoxnoxiousCommand
     * parse the ZoxnoxiousCommand to get a cvChannelOffset and a midiChannel.
     * If we find a cardId matching our Id, and the assignment isn't owned, set it to owned.
     * pre:
     * (1) zCommand message state has list of channelassignment with hardware ids
     * post:
     * (1) this object sets hasChannelAssignment, cvChannelOffset, midiChannel.
     * (2) the zCommand message has a ChannelAssignment.assignmentOwned flipped from false to true
     */
    virtual void processZoxnoxiousCommand(ZoxnoxiousCommandMsg *zCommand) {

        if (! hasChannelAssignment && zCommand->authoritativeSource) {
            // if we have an authoritative source and no channel assignment,
            // run through the available assignments and pick one up if possible
            for (slot = 0; slot < maxCards; ++slot) {
                if (zCommand->channelAssignments[slot].cardId == getHardwareId() &&
                    ! zCommand->channelAssignments[slot].assignmentOwned) {
                    zCommand->channelAssignments[slot].assignmentOwned = true; // I OWNEZ THEE
                    midiChannel = zCommand->channelAssignments[slot].midiChannel;
                    cvChannelOffset = zCommand->channelAssignments[slot].cvChannelOffset;
                    hasChannelAssignment = true;

                    INFO("Z Expander: frame %" PRId64 ": module id %" PRId64 " : hasChannelAssignment at slot %d", APP->engine->getFrame(), getId(), slot);
                    break;
                }
            }
        }
        else if (! zCommand->authoritativeSource) {
            // received message from a non-auth source, reset whatever we know
            if (hasChannelAssignment) {
                INFO("Z Expander: frame %" PRId64 ": module id %" PRId64 " : unset ChannelAssignment", APP->engine->getFrame(), getId());
            }

            hasChannelAssignment = false;
        }
        else {
            // typical case -- prob ought to optimize this if/else block
            zCommand->channelAssignments[slot].assignmentOwned = true;
        }
    }



    /** initCommandMsgState
     *
     * set both zCommand_a and zCommand_b to an (identical) initial
     * state.  Since the data is synthesized here, it gets marked as
     * from a not-authoritative source.
     * Reset any state variables since they may change with an expander change.
     */
    virtual void initCommandMsgState() {
        zCommand_a = commandEmpty;
        zCommand_b = commandEmpty;

        hasChannelAssignment = false;
        cvChannelOffset = invalidCvChannelOffset;
        midiChannel = invalidMidiChannel;
    }




};
