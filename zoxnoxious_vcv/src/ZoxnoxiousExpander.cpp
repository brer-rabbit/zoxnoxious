#include "plugin.hpp"
#include "ZoxnoxiousExpander.hpp"

// command messages are only once every this many clock cycles.
// The data doesn't change that often, really it's only important
// for when the expander connection changes.  But since stuff is daisy
// chained we can't rely on onExpanderChange alone.
static const int commandMessageClockDivider = 10000;

// an empty control message for initializing
static const ZoxnoxiousControlMsg controlEmpty = { };



// this "empty command" is used for initialization and to reset when the expander changes.  8 cards.
static const ZoxnoxiousCommandMsg commandEmpty =
  {
      // authoritativeSource
      false,
      // cardId, cvChannelOffset, midiChannel, assignmentOwned
      {
          { invalidCardId, invalidCvChannelOffset, invalidMidiChannel, false},
          { invalidCardId, invalidCvChannelOffset, invalidMidiChannel, false},
          { invalidCardId, invalidCvChannelOffset, invalidMidiChannel, false},
          { invalidCardId, invalidCvChannelOffset, invalidMidiChannel, false},
          { invalidCardId, invalidCvChannelOffset, invalidMidiChannel, false},
          { invalidCardId, invalidCvChannelOffset, invalidMidiChannel, false},
          { invalidCardId, invalidCvChannelOffset, invalidMidiChannel, false},
          { invalidCardId, invalidCvChannelOffset, invalidMidiChannel, false}
      }
  };


ZoxnoxiousModule::ZoxnoxiousModule() :
    validRightExpander(false), validLeftExpander(false), controlMessageIndex(0), hasChannelAssignment(false),
    cvChannelOffset(invalidCvChannelOffset), midiChannel(invalidMidiChannel), slot(invalidSlot),
    isPrimary(false),
    commandMessageClockDividerCount(APP->engine->getFrame() % commandMessageClockDivider) {

    // command
    leftExpander.producerMessage = &zCommand_a;
    leftExpander.consumerMessage = &zCommand_b;
    initCommandMsgState();

    // control
    for (int i = 0; i < maxCards; ++i) {
        zControlMessages[i] = controlEmpty;
    }
    rightExpander.producerMessage = &zControlMessages[0];
    rightExpander.consumerMessage = &zControlMessages[maxCards - 1];

    cardOutputNames.resize(maxCards * 2);
    onChannelAssignmentLost();
}

void ZoxnoxiousModule::setExpanderPrimary() {
    // ...and this isn't actually used for anything yet
    isPrimary = true;
}


void ZoxnoxiousModule::onExpanderChange (const ExpanderChangeEvent &e) {
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



void ZoxnoxiousModule::processExpander(const ProcessArgs &args) {

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


void ZoxnoxiousModule::processZoxnoxiousCommand(ZoxnoxiousCommandMsg *zCommand) {

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
                onChannelAssignmentEstablished(zCommand);
                break;
            }
        }
    }
    else if (! zCommand->authoritativeSource) {
        // received message from a non-auth source, reset whatever we know
        if (hasChannelAssignment) {
            onChannelAssignmentLost();
        }
        hasChannelAssignment = false;
    }
    else {
        // typical case -- we have a channel assignemnt already.
        // Claim our assignment so the command can be daisy chained along.
        zCommand->channelAssignments[slot].assignmentOwned = true;
    }
}


void ZoxnoxiousModule::initCommandMsgState() {
    zCommand_a = commandEmpty;
    zCommand_b = commandEmpty;

    hasChannelAssignment = false;
    cvChannelOffset = invalidCvChannelOffset;
    midiChannel = invalidMidiChannel;
}


void ZoxnoxiousModule::setLeftExpanderLight(int lightEnum) {
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


void ZoxnoxiousModule::setRightExpanderLight(int lightEnum) {
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



void ZoxnoxiousModule::onChannelAssignmentEstablished(ZoxnoxiousCommandMsg *zCommand) {
    uint8_t cardId;

    for (int i = 0; i < maxCards; ++i) {
        cardId = zCommand->channelAssignments[i].cardId;
        if (cardId != invalidCardId) {
            cardOutputNames[i * 2].assign(getCardOutputName(cardId, 1, i));
            cardOutputNames[i * 2 + 1].assign(getCardOutputName(cardId, 2, i));
        }
        else {
            cardOutputNames[i * 2].assign("----");
            cardOutputNames[i * 2 + 1].assign("----");
        }
    }
}


void ZoxnoxiousModule::onChannelAssignmentLost() {
    for (int i = 0; i < maxCards; ++i) {
        cardOutputNames[i * 2].assign("----");
        cardOutputNames[i * 2 + 1].assign("----");
    }
}


