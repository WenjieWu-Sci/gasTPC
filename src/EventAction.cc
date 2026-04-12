/// @file EventAction.cc
/// @brief Clears hit buffer at event start; forwards hits to RunAction at end.

#include "EventAction.hh"
#include "RunAction.hh"
#include "TPCHit.hh"

#include "G4Event.hh"
#include "G4SystemOfUnits.hh"

// Thread-local G4Allocator for TPCHit (declared extern in TPCHit.hh)
G4ThreadLocal G4Allocator<TPCHit>* TPCHitAllocator = nullptr;

// ---- TPCHit::Print (defined here to avoid a separate TPCHit.cc) ----------

void TPCHit::Print() {
    G4cout << "TPCHit | pos=("
           << position.x() / mm << ", "
           << position.y() / mm << ", "
           << position.z() / mm
           << ") mm | Edep=" << energyDeposit / keV << " keV"
           << " | t=" << time / ns << " ns"
           << " | nElectrons=" << nElectrons
           << " | PDG=" << pdgCode
           << G4endl;
}

// ---- EventAction ---------------------------------------------------------

EventAction::EventAction(RunAction* runAction)
    : fRunAction(runAction)
{}

void EventAction::BeginOfEventAction(const G4Event* /*event*/) {
    fHits.clear();
}

void EventAction::EndOfEventAction(const G4Event* event) {
    const int evtID = event->GetEventID();

    if (!fHits.empty())
        fRunAction->FillEvent(evtID, fHits);
}

void EventAction::AddHit(const TPCHit& hit) {
    fHits.push_back(hit);
}
