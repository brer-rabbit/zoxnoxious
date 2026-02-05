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

private:
  rack::dsp::ClockDivider tryAttachDivider;
};

} // namespace zox
