#pragma once

#include <array>
#include <memory>
#include <vector>
#include <rack.hpp>
#include "constants.hpp"


namespace zox {


constexpr int hnsMaxModules = 7;
constexpr int hnsOutputsPerModule = 2;

// HardwareNameService is string storage for orchestration layer
class HardwareNameService {
public:
  std::array<std::string, hnsMaxModules * hnsOutputsPerModule> names;
  std::array<std::string, hnsMaxModules * hnsOutputsPerModule> shortNames;

  HardwareNameService() {
    names.fill(invalidCardOutputName);
    shortNames.fill(invalidCardOutputName);
  }

  const std::string* getNamePtr(int index) {
    return &names[index];
  }

  const std::string* getShortNamePtr(int index) const {
    return &shortNames[index];
  }

  void setName(int index, const std::string& value) {
    names[index] = value;
  }

  void setShortName(int index, const std::string& value) {
    shortNames[index] = value;
  }

};


struct Participant;

struct ParticipantProperty {
  int64_t moduleId;
  uint8_t hardwareId;
  int8_t cvChannelOffset;
  int8_t outputDeviceId;
  int8_t midiChannel;
  int8_t slotNum;
  bool isAllocated;
};

struct Slot {
  Participant *participant = nullptr;
  ParticipantProperty props{};
};


struct SlotAndNameService {
  int8_t slotNum;
  std::shared_ptr<HardwareNameService> nameService;
};

struct Broker {

  Broker();

  // Broker invariant:
  // if published == nullptr  -> discovery report not yet latched
  // if published != nullptr  -> device tree initialized and stable
  // what that means:
  // published is initialized to nullptr.
  // registerDevices may be called once (and only once) to set published non-null.
  // (un)registerParticipant returns false if published is nullptr.
  //
  // Broker snapshot ownership & threading model
  // - The Broker maintains TWO Snapshot buffers (stoargeA and storageB)
  // - Exactly ONE Snapshot is "published" at any time via the atomic `published`.
  // - The AUDIO THREAD:
  //     * May ONLY read from the Snapshot pointed to by `published` (which is initially null).
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

  struct Snapshot {
    Slot slots[maxVoiceCards];
  } storageA, storageB;

  // Audio-thread to accesses Snapshot via this pointer
  std::atomic<const Snapshot*> published { nullptr };

  bool registerDevices(ParticipantProperty *devices, size_t count);

  SlotAndNameService registerParticipant(int64_t moduleId, Participant *p);
  bool unregisterParticipant(int64_t moduleId);

  // client on audio thread: use snapshot() to get view of data
  const Snapshot& snapshot() const;

  // attached participants use HNS to discover names
  const std::shared_ptr<HardwareNameService> getHardwareNameService() const;

private:
  int8_t findSlot(Snapshot& s, int64_t moduleId, Participant* p);
  std::shared_ptr<HardwareNameService> nameService;
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
  virtual void pullSamples(const rack::engine::Module::ProcessArgs& args, rack::dsp::Frame<maxAudioChannels> &sharedFrame, int offset) = 0;


  // pullMidi()  Client may set the midi::Message owned by the caller.  Return value of
  // true signals midi message is set and to be returned.  Client to return
  // false is no message is set.
  // Called at 100-200 Hz (TBD).  Actual frequency can be inferred from clockDivision.
  // This is also a good place to do any module state changes for UI elements such as lights.
  virtual bool pullMidi(const rack::engine::Module::ProcessArgs& args, uint32_t clockDivision, int midiChannel, rack::midi::Message &midiMessage) = 0;

};



struct ParticipantLifecycle {
  Participant *participant = nullptr;
  Broker* broker = nullptr;

  enum class AttachState : uint8_t {
    Detached,
    AttachRequested,
    Attached,
    DetachRequested
  };

  // ALL reads of broker / participant / slotNum / nameService
  // must occur only after reading state with acquire.
  std::atomic<AttachState> state { AttachState::AttachRequested };

  int8_t slotNum = invalidSlot;
  std::shared_ptr<HardwareNameService> nameService = nullptr;

  bool wantAttach() const;
  bool isAttached() const;
  bool heartbeat();
  bool completeAttach(Broker *b, Participant *p);
  void completeDetach();
};

} // namespace zox
