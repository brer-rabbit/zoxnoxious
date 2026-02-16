#include "plugin.hpp"
#include "common.hpp"


namespace zox {

void setMidiProgramChangeMessage(rack::midi::Message& midiOutMessage, int8_t midiChannel, int8_t program) {
  midiOutMessage.setSize(2);
  midiOutMessage.setChannel(midiChannel);
  midiOutMessage.setStatus(midiProgramChangeStatus);
  midiOutMessage.setNote(program);
}

} // namespace zox
