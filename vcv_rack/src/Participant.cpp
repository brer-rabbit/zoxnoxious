#include <atomic>
#include <vector>
#include "Participant.hpp"
#include "AudioIO.hpp"

namespace zox {


Broker::Broker() {
  // do not initialize published: nullptr value indicates no ParticipantProperties
  // registered.
}


bool Broker::registerDevices(ParticipantProperty *devices, size_t count) {
  size_t i = 0;
  for (; i < count; ++i) {
    // store to both copies so it won't matter which one is actually used first
    std::memcpy(&storageA.slots[i].props, &devices[i], sizeof(ParticipantProperty));
    std::memcpy(&storageB.slots[i].props, &devices[i], sizeof(ParticipantProperty));
  }

  // flag anything not specified as invalid
  for (; i < maxVoiceCards; ++i) {
    storageA.slots[i].props.hardwareId = invalidCardId;
    storageB.slots[i].props.hardwareId = invalidCardId;
  }

  // set published non-null: allow the participant registrations to proceed
  published.store(&storageA, std::memory_order_release);

  return true;
}


// register the participant Id.  Double buffer the storage, so UI and audio threads
// can co-mingle.  Mutation is only to happen on UI thread.  Audio thread gets a
// pointer to a published stable version.

bool Broker::registerParticipant(int64_t moduleId, Participant *p) {
  if (p == nullptr) {
    WARN("Broker received a null pointer for registration: moduleId %" PRId64, moduleId);
    return false;
  }

  if (published.load(std::memory_order_acquire) == nullptr) {
    WARN("registration denied for moduleId %" PRId64
         ": hardware detail pending", moduleId);
    return false;
  }

  // 1. Grab the "other" snapshot (the one the audio thread isn't using)
  const Snapshot &current = *published.load(std::memory_order_acquire);
  Snapshot &next = (&current == &storageA) ? storageB : storageA;

  // 2. Clone current state into the next snapshot
  next = current;

  // 3. Perform bizlogic to determine if we can register participant.
  // Any manipulation is allowed on next since it is not used by audio thread.
  bool retval = findSlot(next, moduleId, p);

  // 4. Atomic Swap: Now the audio thread sees the new participant
  // (and yes, still do this even if the previous step failed)
  published.store(&next, std::memory_order_release);

  return retval;
}



bool Broker::unregisterParticipant(int64_t moduleId) {
  bool removed = false;

  // select inactive snapshot
  const Snapshot &current = *published.load(std::memory_order_acquire);
  Snapshot &next = (&current == &storageA) ? storageB : storageA;

  // clone current state into next published state
  next = current;

  // find then remove moduleId by clearing isAllocated flag
  for (size_t i = 0; i < maxVoiceCards; ++i) {
    if (next.slots[i].props.moduleId == moduleId) {
      next.slots[i].props.isAllocated = false;
      removed = true;
      break;
    }
  }

  published.store(&next, std::memory_order_release);
  return removed;
}


bool Broker::findSlot(Snapshot& s, int64_t moduleId, Participant* p) {
  uint8_t hardwareId = p->getHardwareId();

  for (size_t i = 0; i < maxVoiceCards; ++i) {
    if (s.slots[i].props.isAllocated == false &&
        s.slots[i].props.hardwareId == hardwareId) {
      s.slots[i].participant = p;
      s.slots[i].props.moduleId = moduleId;
      s.slots[i].props.isAllocated = true;
      INFO("found slot %zu for moduleId %" PRId64  " hardwareId %" PRIu8,
           i, moduleId, hardwareId);
      return true;
    }
  }

  return false;
}



const Broker::Snapshot& Broker::snapshot() const {
  auto* s = published.load(std::memory_order_acquire);
  static const Snapshot empty{};  // safe fallback
  return s ? *s : empty;
}



// tryAttach() may be called from audio thread or GUI thread.  For the common case of
// the module is attached and a valid AudioIO instance exists return quick.
void ParticipantLifecycle::tryAttach(Participant *p) {
  auto* inst = AudioIO::instance.load(std::memory_order_relaxed);

  if (attached && inst) {
    return;
  }

  if (attached && !inst) {
    // orchestrator vanished: detach
    participant = nullptr;
    broker = nullptr;
    attached = false;
  }

  if (!inst) {
    // no orchestrator to attach to
    return;
  }

  // else: not attached, but do have a valid orchestrator.  Effect the attachment.
  broker = &inst->getBroker();
  if (broker->registerParticipant(p->getModuleId(), p)) {
    participant = p;
    attached = true;
  }
}


// detach () this must remain idempotent
void ParticipantLifecycle::detach() {
  if (!attached) {
    return;
  }
  if (broker && participant) {
    INFO("ParticipantLifecycle calling detach on %" PRId64, participant->getModuleId());
    broker->unregisterParticipant(participant->getModuleId());
  }

  participant = nullptr;
  broker = nullptr;
  attached = false;
}


} // namespace zox
