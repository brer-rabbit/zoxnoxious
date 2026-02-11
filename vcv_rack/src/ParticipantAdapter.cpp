#include "ParticipantAdapter.hpp"


namespace zox {

static constexpr int invalidLightEnum = -1;

ParticipantAdapter::ParticipantAdapter() : myLightEnum(invalidLightEnum) {
  // TODO: 10000 ?
  tryAttachDivider.setDivision(10000);
}

ParticipantAdapter::~ParticipantAdapter() {
  // destructors called from UI thread- this is a safe backup if onRemove went wonky
  lifecycle.detach();
}


void ParticipantAdapter::setParticipant(Participant* p) {
  participant = p;
}

void ParticipantAdapter::process(const ProcessArgs& args) {
  if (tryAttachDivider.process()) {
    lifecycle.tryAttach(participant);
    if (myLightEnum != invalidLightEnum) {
      setAttachedLightStatus();
    }
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


// enable setting attached status light
void ParticipantAdapter::setLightEnum(int lightEnum) {
  myLightEnum = lightEnum;
}

void ParticipantAdapter::setAttachedLightStatus() { 
  if (lifecycle.attached) {
    lights[myLightEnum + 0].setBrightness(0.f);
    lights[myLightEnum + 1].setBrightness(1.f);
    lights[myLightEnum + 2].setBrightness(0.f);
  }
  /* yellow case
     else if (validRightExpander) {
     lights[myLightEnum + 0].setBrightness(1.f);
     lights[myLightEnum + 1].setBrightness(1.f);
     lights[myLightEnum + 2].setBrightness(0.f);
     }
  */
  else {
    lights[myLightEnum + 0].setBrightness(1.f);
    lights[myLightEnum + 1].setBrightness(0.f);
    lights[myLightEnum + 2].setBrightness(0.f);
  }
}


} // namespace zox
