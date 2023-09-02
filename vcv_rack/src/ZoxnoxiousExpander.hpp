#pragma once

#include <rack.hpp>


// maximum number of channels on a USB Audio interface
static const int maxChannels = 31;

// This doesn't go here-- figure out a home for it. nothing to do with expander.
static const uint8_t midiProgramChangeStatus = 0xC;

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



/** ZoxnoxiousCommandMsg stuff
 * messages originating from backplane card (the only required card)
 * are received from the right expansion.  The purpose is to define
 * channel ownership for signals on the ZoxnoxiousControlMsg.
 */

static const int maxCards = 8; // need this just to size channelAssignments array, that's it
static const int invalidSlot = maxCards;
static const int invalidCardDeviceId = -1;
static const int invalidCvChannelOffset = -1;
static const int invalidMidiChannel = -1;
static const uint8_t invalidCardId = 0;
static const std::string invalidCardOutputName = "----";

struct ChannelAssignment {
    uint8_t cardId; // physical card's identifier
    int cardDeviceId; // device id: usb device
    int cvChannelOffset;  // offset to write channel to the controlmsg frame
    int midiChannel;
    bool assignmentOwned; // true if a card claimed this assignment
};

struct ZoxnoxiousCommandMsg {
    bool authoritativeSource;
    struct ChannelAssignment channelAssignments[maxCards];
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
    ZoxnoxiousModule();

    /** setExpanderPrimary
     *
     * declare this ZoxnoxiousExpander to be the primary, it will be responsible for audio and midi output.
     */
    void setExpanderPrimary();
    void onExpanderChange (const ExpanderChangeEvent &e) override;

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
    int cardDeviceId;
    int cvChannelOffset;
    int midiChannel;
    int slot;

    std::vector<std::string> cardOutputNames;

    /** processExpander
     * to make use of the expander, this method ought to be
     * called from the derived class's process()
     */
    virtual void processExpander(const ProcessArgs &args);


    /** processZoxnoxiousControl
     *
     * Intended  behavior is to fill in what channels we're responsible for.
     * The controlMsg will be modified.
     * Subclass will need to implement this, this class doesn't know the specifics.
     */
    virtual void processZoxnoxiousControl(ZoxnoxiousControlMsg *controlMsg) = 0;


    /** processZoxnoxiousCommand
     * parse the ZoxnoxiousCommand to get a cvChannelOffset and a midiChannel.
     * If we find a cardId matching our Id, and the assignment isn't owned, set it to owned.
     * pre:
     * (1) zCommand message state has list of channelassignment with hardware ids
     * post:
     * (1) this object sets hasChannelAssignment, cvChannelOffset, midiChannel.
     * (2) the zCommand message has a ChannelAssignment.assignmentOwned flipped from false to true
     */
    virtual void processZoxnoxiousCommand(ZoxnoxiousCommandMsg *zCommand);


    /** initCommandMsgState
     *
     * set both zCommand_a and zCommand_b to an (identical) initial
     * state.  Since the data is synthesized here, it gets marked as
     * from a not-authoritative source.
     * Reset any state variables since they may change with an expander change.
     */
    virtual void initCommandMsgState();


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
    virtual void setLeftExpanderLight(int lightEnum);

    /** setRightExpanderLight
     *
     * convenience function to set the RGB lights for right expander.
     * See above.  Logic is slightly different:
     * Green: has channel assignment (also implies valid expander)
     * Yellow: valid right expander
     * Red: none of the above
     */
    virtual void setRightExpanderLight(int lightEnum);


    /** channelAssignmentEstablished
     *
     * override this when a module establishes a channel assignment.
     * run through the command message, find what cards we have.  Populate
     * cardOutputNames based on this.
     */
    virtual void onChannelAssignmentEstablished(ZoxnoxiousCommandMsg *zCommand);

    /** channelAssignmentLost
     *
     * override this when a module loses a channel assignment
     * (eg- module moved so expander connectivity lost, primary
     * module removed, hardware issue detected, etc)
     */
    virtual void onChannelAssignmentLost();

    /** getCardHardwareId
     * return the hardware Id of the card.  Derived class needs to implement this.
     */
    virtual uint8_t getHardwareId() = 0;


private:
    bool isPrimary;
    int commandMessageClockDividerCount;

};
