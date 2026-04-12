/// @file PhysicsList.cc
/// @brief G4EmStandardPhysics_option4 + decay for accurate low-energy EM.

#include "PhysicsList.hh"

#include "G4DecayPhysics.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4SystemOfUnits.hh"

PhysicsList::PhysicsList() : G4VModularPhysicsList() {
    SetVerboseLevel(0);
    // High-accuracy EM physics (Urban model, best for gas detectors)
    RegisterPhysics(new G4EmStandardPhysics_option4(0));
    // Decay physics for unstable secondaries
    RegisterPhysics(new G4DecayPhysics(0));
}

void PhysicsList::SetCuts() {
    SetCutValue(0.1 * mm, "gamma");
    SetCutValue(0.1 * mm, "e-");
    SetCutValue(0.1 * mm, "e+");
    SetCutValue(1.0 * mm, "proton");
}
