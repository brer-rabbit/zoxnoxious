#pragma once

#include <vector>
#include <rack.hpp>
#include "common.hpp"


namespace zox {


struct Participant;

struct Broker {

  Broker();

  struct Snapshot {
    Participant* participants[maxCards] = {nullptr};
    int64_t moduleIds[maxCards] = {0};
    size_t count = 0;
  };


  // Broker snapshot ownership & threading model
  // - The Broker maintains TWO Snapshot buffers (stoargeA and storageB)
  // - Exactly ONE Snapshot is "published" at any time via the atomic `published`.
  // - The AUDIO THREAD:
  //     * May ONLY read from the Snapshot pointed to by `published`.
  //     * Must never write to any Snapshot.
  //
  // - The UI THREAD:
  //     * Must NEVER modify the Snapshot currently pointed to by `published`.
  //     * Must copy the published Snapshot into the *other* buffer,
  //       mutate that buffer, then publish it via an atomic store.
  //
  // Buffer selection rule (UI thread only):
  //     const Snapshot* current = published.load(...);
  //     Snapshot& next = (&current == &storageA) ? storageB : storageA;
  //
  // Violating this invariant results in bad things.
  // -----------------------------------------------------------------------------


  // Audio-thread to accesses Snapshot via this pointer
  std::atomic<const Snapshot*> published { nullptr };

  // pre-allocated work areas
  Snapshot storageA;
  Snapshot storageB;


  bool registerParticipant(int64_t moduleId, Participant *p);
  bool unregisterParticipant(int64_t moduleId);

  // client on audio thread: use snapshot() to get view of data
  const Snapshot& snapshot() const;

private:
  bool addAndSort(Snapshot& s, int64_t id, Participant* p);
};



struct Participant {
  virtual ~Participant() = default;

  // the rack moduleId for the instance
  virtual int64_t getModuleId() = 0;

  // Zoxnoxious hardware Id set in voice card ROM
  virtual uint8_t getHardwareId() const = 0;



  // Participants requiring Rack's ProcessArgs will instead use methods provided here.

  // pullSamples() is called at the audio rate and replaces process().  Client
  // fills in the sharedFrame with samples starting at the offset.
  virtual void pullSamples(const rack::engine::Module::ProcessArgs& args, rack::dsp::Frame<maxChannels> &sharedFrame, int offset) = 0;


  // pullMidi()  Client may set the midi::Message owned by the caller.  Return value of
  // true signals midi message is set and to be returned.  Client to return
  // false is no message is set.
  // Called at 100-200 Hz (TBD).
  // This is also a good place to do any module state changes for UI elements such as lights.
  virtual bool pullMidi(const rack::engine::Module::ProcessArgs& args, int midiChannel, rack::midi::Message &midiMessage) = 0;

};



struct ParticipantLifecycle {
  Participant *participant = nullptr;
  Broker* broker = nullptr;
  bool attached = false;

  void tryAttach(Participant *p);
  void detach();
};

} // namespace zox
