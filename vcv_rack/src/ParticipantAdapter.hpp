#pragma once

#include "Participant.hpp"

namespace zox {

// ParticipantAdapter owns a Participant.  ParticipantAdapter manages lifecycle,
// while Participant defines orchestrator-facing behavior.  An implemented concrete module
// will wire these together
struct ParticipantAdapter : rack::Module {

public:
  ParticipantAdapter();
  virtual ~ParticipantAdapter();

  void setParticipant(Participant* p);
  void process(const ProcessArgs& args) final;

  void onAdd(const AddEvent& e) override;
  void onRemove(const RemoveEvent& e) override;

protected:
    ParticipantLifecycle lifecycle;
    Participant* participant = nullptr;

  // shortcut function that takes the expander light enum to set it based on attached state.
  // May be overridden if you don't want to go with the flow.
  // setLightEnum to be called during initialization, and
  // setAttachedLightStatus gets called during process()
  virtual void setLightEnum(int lightEnum);
  virtual void setAttachedLightStatus();

private:
  rack::dsp::ClockDivider tryAttachDivider;
  int myLightEnum;
};

} // namespace zox
