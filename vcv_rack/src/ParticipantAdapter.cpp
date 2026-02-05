#include "ParticipantAdapter.hpp"


namespace zox {


ParticipantAdapter::ParticipantAdapter() {
  tryAttachDivider.setDivision(10000);
}

ParticipantAdapter::~ParticipantAdapter() {
  lifecycle.detach();
}


void ParticipantAdapter::setParticipant(Participant* p) {
  participant = p;
}

void ParticipantAdapter::process(const ProcessArgs& args) {
  if (tryAttachDivider.process()) {
    lifecycle.tryAttach(participant);
  }
}

void ParticipantAdapter::onAdd(const AddEvent& e) {
  Module::onAdd(e);
  lifecycle.tryAttach(participant);
}

void ParticipantAdapter::onRemove(const RemoveEvent& e) {
  lifecycle.detach();
  Module::onRemove(e);
}


} // namespace zox
