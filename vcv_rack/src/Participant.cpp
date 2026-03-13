#include <atomic>
#include <vector>
#include "Participant.hpp"
#include "AudioIO.hpp"

namespace zox {


Broker::Broker() : nameService(std::make_shared<HardwareNameService>()) {
  // do not initialize published: nullptr value indicates no ParticipantProperties
  // registered.
}


bool Broker::registerDevices(ParticipantProperty *devices, size_t count) {
  size_t i = 0;

  for (; i < count; ++i) {
    // store to both copies so it won't matter which one is actually used first
    std::memcpy(&storageA.slots[i].props, &devices[i], sizeof(ParticipantProperty));
    std::memcpy(&storageB.slots[i].props, &devices[i], sizeof(ParticipantProperty));
    // output table need to be indexed by slot number
    if (devices[i].slotNum >= 0 && devices[i].slotNum < kMaxModules) {
      nameService->setName(devices[i].slotNum * 2, getCardOutputName(devices[i].hardwareId, 1, devices[i].slotNum));
      nameService->setName(devices[i].slotNum * 2 + 1, getCardOutputName(devices[i].hardwareId, 2, devices[i].slotNum));
      INFO("register index %zu slot %d: %s :: %s", i, devices[i].slotNum,
           nameService->getNamePtr( devices[i].slotNum * 2 )->c_str(),
           nameService->getNamePtr( devices[i].slotNum * 2 + 1 )->c_str());
    }
    else {
      WARN("unexpected slot number: %d", devices[i].slotNum);
    }
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
// On successful register returns the slot

SlotAndNameService Broker::registerParticipant(int64_t moduleId, Participant *p) {
  SlotAndNameService slotAndName = { invalidSlot, nameService };
  if (p == nullptr) {
    WARN("Broker received a null pointer for registration: moduleId %" PRId64, moduleId);
    return slotAndName;
  }

  if (published.load(std::memory_order_acquire) == nullptr) {
    WARN("registration denied for moduleId %" PRId64
         ": hardware detail pending", moduleId);
    return slotAndName;
  }

  // 1. Grab the "other" snapshot (the one the audio thread isn't using)
  const Snapshot &current = *published.load(std::memory_order_acquire);
  Snapshot &next = (&current == &storageA) ? storageB : storageA;

  // 2. Clone current state into the next snapshot
  next = current;

  // 3. Perform bizlogic to determine if we can register participant.
  // Any manipulation is allowed on next since it is not used by audio thread.
  slotAndName.slotNum = findSlot(next, moduleId, p);

  // 4. Atomic Swap: Now the audio thread sees the new participant
  // (and yes, still do this even if the previous step failed)
  published.store(&next, std::memory_order_release);

  return slotAndName;
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


int8_t Broker::findSlot(Snapshot& s, int64_t moduleId, Participant* p) {
  uint8_t hardwareId = p->getHardwareId();

  // TODO: may want to safeguard against incoming s.slots[i].props.moduleId == moduleId
  for (int i = 0; i < maxVoiceCards; ++i) {
    if (s.slots[i].props.isAllocated == false &&
        s.slots[i].props.hardwareId == hardwareId) {
      s.slots[i].participant = p;
      s.slots[i].props.moduleId = moduleId;
      s.slots[i].props.isAllocated = true;
      return i;
    }
  }

  return invalidSlot;
}



const Broker::Snapshot& Broker::snapshot() const {
  auto* s = published.load(std::memory_order_acquire);
  static const Snapshot empty{};  // safe fallback
  return s ? *s : empty;
}



const std::shared_ptr<HardwareNameService> Broker::getHardwareNameService() const {
  return nameService;
}


bool ParticipantLifecycle::wantAttach() const {
  return state.load(std::memory_order_acquire) == AttachState::AttachRequested;
}

bool ParticipantLifecycle::isAttached() const {
  return state.load(std::memory_order_acquire) == AttachState::Attached;
}

bool ParticipantLifecycle::heartbeat() {
  AudioIO* current = AudioIO::instance.load(std::memory_order_acquire);

  if (isAttached() && (!current || &current->getBroker() != broker)) {
    // Orchestrator vanished or changed
    // clear fields then publish state
    INFO("heartbeat failed, detaching");
    participant = nullptr;
    broker = nullptr;
    state.store(AttachState::AttachRequested);
    return false;
  }
  return true;
}


// completeAttach() is called from the AudioIO manager on scanning modules for those that
// have an ATTACH_REQUESTED state.  This avoids the Participant modules calling attach themselves.
// Return true on state change to attached.
bool ParticipantLifecycle::completeAttach(Broker* b, Participant* p) {
  if (!b || !p) {
    WARN("completeAtttach received nullptr");
    return false;
  }

  if (state.load(std::memory_order_acquire) != AttachState::AttachRequested) {
    return false;
  }

  SlotAndNameService slotAndName = b->registerParticipant(p->getModuleId(), p);

  if (slotAndName.slotNum == invalidSlot) {
    return false;
  }

  broker = b;
  participant = p;
  slotNum = slotAndName.slotNum;
  nameService = slotAndName.nameService;

  state.store(AttachState::Attached, std::memory_order_release);
  return true;
}


// detach() this must remain idempotent
void ParticipantLifecycle::completeDetach() {
  if (!isAttached()) {
    return;
  }
  if (broker && participant) {
    INFO("ParticipantLifecycle calling detach on %" PRId64, participant->getModuleId());
    broker->unregisterParticipant(participant->getModuleId());
  }

  participant = nullptr;
  broker = nullptr;
  state.store(AttachState::Detached, std::memory_order_release);
}


} // namespace zox
