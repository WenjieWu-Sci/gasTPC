#pragma once
/// @file EventAction.hh
/// @brief Accumulates TPCHit records per event and forwards them to RunAction.

#include "TPCHit.hh"
#include "G4UserEventAction.hh"

#include <vector>

class RunAction;

class EventAction : public G4UserEventAction {
public:
    explicit EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction  (const G4Event* event) override;

    /// Called by SteppingAction for every qualifying step.
    void AddHit(const TPCHit& hit);

private:
    RunAction*           fRunAction;
    std::vector<TPCHit>  fHits;
};
