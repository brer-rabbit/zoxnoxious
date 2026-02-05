#include <atomic>
#include <vector>
#include "Participant.hpp"
#include "AudioIO.hpp"

namespace zox {


Broker::Broker() {
  published.store(&storageA, std::memory_order_relaxed);
}


// register the participant Id.  Double buffer the storage, so UI and audio threads
// can co-mingle.  Mutation is only to happen on UI thread.  Audio thread gets a
// pointer to a published stable version.

bool Broker::registerParticipant(int64_t moduleId, Participant *p) {
  // 1. Grab the "other" snapshot (the one the audio thread isn't using)
  const Snapshot &current = *published.load(std::memory_order_acquire);
  Snapshot &next = (&current == &storageA) ? storageB : storageA;

  // 2. Clone current state into the next snapshot
  next = current; 

  // 3. Perform your logic (Add/Remove/Sort) on the 'next' snapshot
  // Since 'next' is not being read by the audio thread yet, we can sort freely
  bool retval = addAndSort(next, moduleId, p);

  // 4. Atomic Swap: Now the audio thread sees the new, perfectly sorted list
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

  // find then remove moduleId, shifting remainder left
  for (size_t i = 0; i < next.count; ++i) {
    if (next.moduleIds[i] == moduleId) {
      removed = true;
      // Shift left
      for (size_t j = i; j + 1 < next.count; ++j) {
        next.moduleIds[j] = next.moduleIds[j + 1];
        next.participants[j] = next.participants[j + 1];
      }
      --next.count;
      break;
    }
  }

  published.store(&next, std::memory_order_release);
  return removed;
}


// simple sort routine given we only have maxCards elements.  Assumes
// array is already sorted going on.  Find element, if it's there in duplicate,
// remove.  Insert to next position and shift remaining.
// Given the conditions this is faster than std::sort.
// Return true if element inserted or false if insert failed (array full).
bool Broker::addAndSort(Snapshot& s, int64_t id, Participant* p) {
  // Remove existing
  for (size_t i = 0; i < s.count; ++i) {
    if (s.moduleIds[i] == id) {
      for (size_t j = i; j + 1 < s.count; ++j) {
        s.moduleIds[j] = s.moduleIds[j + 1];
        s.participants[j] = s.participants[j + 1];
      }
      --s.count;
      break;
    }
  }

  // this is where number of voice cards is enforced
  if (s.count >= maxCards) {
    return false;
  }

  // Insert sorted
  size_t pos = s.count;
  while (pos > 0 && s.moduleIds[pos - 1] > id) {
    s.moduleIds[pos] = s.moduleIds[pos - 1];
    s.participants[pos] = s.participants[pos - 1];
    --pos;
  }

  s.moduleIds[pos] = id;
  s.participants[pos] = p;
  ++s.count;
  return true;
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
  broker->registerParticipant(p->getModuleId(), p);
  participant = p;
  attached = true;
}


// detach () this must remain idempotent
void ParticipantLifecycle::detach() {
  if (!attached) {
    return;
  }
  if (broker && participant) {
    broker->unregisterParticipant(participant->getModuleId());
  }

  participant = nullptr;
  broker = nullptr;
  attached = false;
}


} // namespace zox
