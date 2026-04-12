#pragma once
/// @file SteppingAction.hh
/// @brief Records ionising energy deposits inside the TPC drift volume.
///
/// For every step that occurs in the "DriftRegion" logical volume the action
/// - reads position, time, energy deposit, track ID and PDG code,
/// - calls IonizationModel to compute the number of surviving electrons, and
/// - appends a TPCHit to the current EventAction.

#include "IonizationModel.hh"
#include "G4UserSteppingAction.hh"

#include <string>

class EventAction;

class SteppingAction : public G4UserSteppingAction {
public:
    SteppingAction(EventAction* eventAction, const IonizationModel& model);
    ~SteppingAction() override = default;

    void UserSteppingAction(const G4Step* step) override;

private:
    EventAction*   fEventAction;
    IonizationModel fModel;
    std::string    fDriftVolumeName; ///< Logical-volume name to monitor
    double         fCathodeZ_mm;     ///< Z-position of cathode face (mm, Geant4 units)
};
