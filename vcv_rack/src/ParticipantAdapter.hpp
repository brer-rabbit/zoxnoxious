#pragma once

#include "Participant.hpp"

namespace zox {

// ParticipantAdapter owns a Participant.  ParticipantAdapter manages lifecycle,
// while Participant defines orchestrator-facing behavior.  An implemented concrete module
// will wire these together
struct ParticipantAdapter : Module {

protected:
    ParticipantLifecycle lifecycle;
    Participant* participant = nullptr;

public:
    void setParticipant(Participant* p) {
        participant = p;
    }

    void process(const ProcessArgs& args) final {
        lifecycle.tryAttach(participant);
    }

    void onAdd(const AddEvent& e) override {
        Module::onAdd(e);
        lifecycle.tryAttach(participant);
    }

    void onRemove(const RemoveEvent& e) override {
        lifecycle.detach();
        Module::onRemove(e);
    }

    virtual ~ParticipantAdapter() {
        lifecycle.detach();
    }
};

} // namespace zox
