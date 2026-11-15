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

    // Primary vertex/direction for the events-level record.
    G4ThreeVector pos(0.0, 0.0, 0.0), dir(0.0, 0.0, 1.0);
    if (const auto* vtx = event->GetPrimaryVertex(0)) {
        pos = vtx->GetPosition();
        if (const auto* prim = vtx->GetPrimary())
            dir = prim->GetMomentumDirection();
    }

    // Always forward, even with zero hits: the events file needs the full
    // denominator for acceptance fractions.
    fRunAction->FillEvent(evtID, fHits, pos, dir);
}

void EventAction::AddHit(const TPCHit& hit) {
    fHits.push_back(hit);
}
