#include "ParticipantAdapter.hpp"


namespace zox {

static constexpr int invalidLightEnum = -1;

ParticipantAdapter::ParticipantAdapter() :
  participant(nullptr), myLightEnum(invalidLightEnum) {
  lightDivider.setDivision(APP->engine->getSampleRate() / 120);
}

ParticipantAdapter::~ParticipantAdapter() {
  // destructors called from UI thread- this is a safe backup if onRemove went wonky
  lifecycle.completeDetach();
}


void ParticipantAdapter::setParticipant(Participant* p) {
  participant = p;
}

Participant* ParticipantAdapter::getParticipant() {
  return participant;
}


void ParticipantAdapter::process(const ProcessArgs& args) {
  lifecycle.heartbeat();

  if (lightDivider.process()) {
    if (myLightEnum != invalidLightEnum) {
      setAttachedLightStatus();
    }
  }
}


void ParticipantAdapter::onAdd(const AddEvent& e) {
  Module::onAdd(e);
}

void ParticipantAdapter::onRemove(const RemoveEvent& e) {
  lifecycle.completeDetach();
  Module::onRemove(e);
}


void ParticipantAdapter::onAttach() {}

ParticipantLifecycle& ParticipantAdapter::getLifecycle() {
  return lifecycle;
}



// enable setting attached status light
void ParticipantAdapter::setLightEnum(int lightEnum) {
  myLightEnum = lightEnum;
}

void ParticipantAdapter::setAttachedLightStatus() { 
  if (lifecycle.isAttached()) {
    /* green */
    lights[myLightEnum + 0].setBrightness(0.f);
    lights[myLightEnum + 1].setBrightness(0.5f);
    lights[myLightEnum + 2].setBrightness(0.f);
  }
  else if (lifecycle.wantAttach()) {
    /* yellow */
    lights[myLightEnum + 0].setBrightness(1.f);
    lights[myLightEnum + 1].setBrightness(1.f);
    lights[myLightEnum + 2].setBrightness(0.f);
  }
  else {
    /* red */
    lights[myLightEnum + 0].setBrightness(1.f);
    lights[myLightEnum + 1].setBrightness(0.f);
    lights[myLightEnum + 2].setBrightness(0.f);
  }
}


} // namespace zox
