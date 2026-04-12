/// @file SteppingAction.cc
/// @brief Records ionising deposits inside the TPC drift volume.

#include "SteppingAction.hh"
#include "ConfigManager.hh"
#include "EventAction.hh"
#include "TPCHit.hh"

#include "G4LogicalVolume.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"

#include <algorithm>
#include <cmath>

SteppingAction::SteppingAction(EventAction* eventAction,
                                const IonizationModel& model)
    : fEventAction(eventAction),
      fModel(model),
      fDriftVolumeName("DriftRegion")
{
    // Cathode face z-position (Geant4 mm units) for drift-distance calculation.
    // Cathode sits at –halfTPCLength + cathodeThickness/2; approximate as –halfTPCLength.
    const double tpcLen_cm     = ConfigManager::Instance().GetDouble("tpc_length_cm", 20.0);
    const double cathThick_cm  = ConfigManager::Instance().GetDouble("cathode_thickness_cm", 0.1);
    fCathodeZ_mm = -(tpcLen_cm / 2.0 - cathThick_cm / 2.0) * 10.0; // cm → mm
}

void SteppingAction::UserSteppingAction(const G4Step* step) {
    // ---- Check volume ----------------------------------------------------
    const G4LogicalVolume* vol =
        step->GetPreStepPoint()
            ->GetTouchableHandle()
            ->GetVolume()
            ->GetLogicalVolume();

    if (vol->GetName() != fDriftVolumeName) return;

    // ---- Energy deposit --------------------------------------------------
    const G4double edep = step->GetTotalEnergyDeposit();
    if (edep <= 0.0) return;

    // ---- Kinematics ------------------------------------------------------
    const G4ThreeVector pos  = step->GetPreStepPoint()->GetPosition();
    const G4double      time = step->GetPreStepPoint()->GetGlobalTime();
    const G4Track*      trk  = step->GetTrack();

    // Drift distance from the cathode face (mm → cm)
    const double driftDist_cm =
        std::max(0.0, (pos.z() - fCathodeZ_mm) / 10.0); // mm to cm

    // ---- Ionisation ------------------------------------------------------
    const double edep_eV  = edep / eV;
    const int nElectrons  = fModel.ComputeElectrons(edep_eV, driftDist_cm);

    // ---- Record hit ------------------------------------------------------
    TPCHit hit;
    hit.position      = pos;
    hit.energyDeposit = edep;
    hit.time          = time;
    hit.trackID       = trk->GetTrackID();
    hit.pdgCode       = trk->GetDefinition()->GetPDGEncoding();
    hit.nElectrons    = nElectrons;

    fEventAction->AddHit(hit);
}
