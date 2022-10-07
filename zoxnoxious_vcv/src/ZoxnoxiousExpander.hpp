#pragma once

#include <rack.hpp>


static const int maxChannels = 31;

// command messages are only once every this many clock cycles.
// The data doesn't change that often, really it's only important
// for when the expander connection changes.  But since stuff is daisy
// chained we can't rely on onExpanderChange alone.
static const int commandMessageClockDivider = 10000;

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
static const ZoxnoxiousControlMsg controlEmpty = { };


/** ZoxnoxiousCommandMsg 
 * messages originating from backplane card (the only required card)
 * are received from the right expansion.  The purpose is to define
 * channel ownership for signals on the ZoxnoxiousControlMsg.
 */

static const int maxCards = 8; // need this just to size channelAssignments array, that's it
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
    struct ChannelAssignment channelAssignments[maxCards];
};


// this "empty command" is used for initialization and to reset when the expander changes.  8 cards.
static const ZoxnoxiousCommandMsg commandEmpty =
  {
      // authoritativeSource
      false,
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

public:

    ZoxnoxiousModule() :
        validRightExpander(false), validLeftExpander(false), controlMessageIndex(0), hasChannelAssignment(false),
        cvChannelOffset(invalidCvChannelOffset), midiChannel(invalidMidiChannel), slot(invalidSlot),
        isPrimary(false),
        commandMessageClockDividerCount(APP->engine->getFrame() % commandMessageClockDivider) {

        // command
        leftExpander.producerMessage = &zCommand_a;
        leftExpander.consumerMessage = &zCommand_b;
        initCommandMsgState();

        // control
        for (int i = 0; i < 8; ++i) {
            zControlMessages[i] = controlEmpty;
        }
        rightExpander.producerMessage = &zControlMessages[0];
        rightExpander.consumerMessage = &zControlMessages[7];
    }


    /** setExpanderPrimary
     *
     * declare this ZoxnoxiousExpander to be the primary, it will be responsible for audio and midi output.
     */
    void setExpanderPrimary() {
        // ...and this isn't actually used for anything yet
        isPrimary = true;
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

        *validExpander = dynamic_cast<ZoxnoxiousModule*>(expander.module) ?
            true : false;

        // command msg changed (added or removed) on the right- reset/mark (un)authoritative
        // so this passes along to the left 
        initCommandMsgState();
    }




protected:
    bool validRightExpander;
    bool validLeftExpander;

    // if we're the authority, hand these out on every clock
    ZoxnoxiousControlMsg zControlMessages[8];
    int controlMessageIndex;

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

            ZoxnoxiousControlMsg *leftExpanderConsumerMessage;
            // left most module is the message owner
            if (leftExpander.module != NULL && validLeftExpander) {
                leftExpanderConsumerMessage =
                    static_cast<ZoxnoxiousControlMsg*>(leftExpander.module->rightExpander.consumerMessage);
            }
            else {
                // this is the left most module.  Clear it, then send.
                zControlMessages[controlMessageIndex] = controlEmpty;
                leftExpanderConsumerMessage =
                    &zControlMessages[controlMessageIndex];
                if (++controlMessageIndex == 8) {
                    controlMessageIndex = 0;
                }
            }


            // call the concrete module to fill in the control message
            processZoxnoxiousControl(leftExpanderConsumerMessage);
            rightExpander.producerMessage = leftExpanderConsumerMessage;
            rightExpander.messageFlipRequested = true;

            //
            // Right Consumer Message Passing to our LeftExpander Producer
            //

            if (++commandMessageClockDividerCount > commandMessageClockDivider) {
                commandMessageClockDividerCount = 0;

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
                leftExpander.messageFlipRequested = true;
            }
        }

    }



    /** processZoxnoxiousControl
     *
     * Intended  behavior is to fill in what channels we're responsible for.
     * The controlMsg will be modified.
     * Subclass will need to implement this, this class doesn't know the specifics.
     */
    virtual void processZoxnoxiousControl(ZoxnoxiousControlMsg *controlMsg) = 0;



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
                    // then copy the relevant info
                    midiChannel = zCommand->channelAssignments[slot].midiChannel;
                    cvChannelOffset = zCommand->channelAssignments[slot].cvChannelOffset;
                    hasChannelAssignment = true;
                    //INFO("Z Expander: frame %" PRId64 ": module id %" PRId64 " : hasChannelAssignment at slot %d", APP->engine->getFrame(), getId(), slot);
                    break;
                }
            }
        }
        else if (! zCommand->authoritativeSource) {
            // received message from a non-auth source, reset whatever we know
            hasChannelAssignment = false;
        }
        else {
            // typical case -- we have a channel assignemnt already.
            // Claim our assignment so the command can be daisy chained along.
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


    /** setLeftExpanderLight
     *
     * convenience function to set the RGB lights for left expander.
     * Intended to provide consistency among modules.
     * Assumes the lightEnum passed in is an RGB.
     * what is communicated to the user:
     * Green: channelassignment && valid left:
     * yellow: valid left expander
     * red: none of the above
     */
    virtual void setLeftExpanderLight(int lightEnum) {
        if (hasChannelAssignment && validLeftExpander) {
            lights[lightEnum + 0].setBrightness(0.f);
            lights[lightEnum + 1].setBrightness(1.f);
            lights[lightEnum + 2].setBrightness(0.f);
        }
        else if (validLeftExpander) {
            lights[lightEnum + 0].setBrightness(1.f);
            lights[lightEnum + 1].setBrightness(1.f);
            lights[lightEnum + 2].setBrightness(0.f);
        }
        else {
            lights[lightEnum + 0].setBrightness(1.f);
            lights[lightEnum + 1].setBrightness(0.f);
            lights[lightEnum + 2].setBrightness(0.f);
        }
    }

    /** setRightExpanderLight
     *
     * convenience function to set the RGB lights for right expander.
     * See above.  Logic is slightly different:
     * Green: has channel assignment (also implies valid expander)
     * Yellow: valid right expander
     * Red: none of the above
     */
    virtual void setRightExpanderLight(int lightEnum) {
        if (hasChannelAssignment) {
            lights[lightEnum + 0].setBrightness(0.f);
            lights[lightEnum + 1].setBrightness(1.f);
            lights[lightEnum + 2].setBrightness(0.f);
        }
        else if (validRightExpander) {
            lights[lightEnum + 0].setBrightness(1.f);
            lights[lightEnum + 1].setBrightness(1.f);
            lights[lightEnum + 2].setBrightness(0.f);
        }
        else {
            lights[lightEnum + 0].setBrightness(1.f);
            lights[lightEnum + 1].setBrightness(0.f);
            lights[lightEnum + 2].setBrightness(0.f);
        }
    }

private:
    bool isPrimary;
    int commandMessageClockDividerCount;

};
