/// @file PhysicsList.cc
/// @brief G4EmStandardPhysics_option4 + decay for accurate low-energy EM.

#include "PhysicsList.hh"

#include "ConfigManager.hh"

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
    // Production cuts are configurable for low-pressure-gas CPU control.
    // Defaults reproduce the stock values.
    auto& cfg = ConfigManager::Instance();
    SetCutValue(cfg.GetDouble("prod_cut_gamma_mm",    0.1) * mm, "gamma");
    SetCutValue(cfg.GetDouble("prod_cut_electron_mm", 0.1) * mm, "e-");
    SetCutValue(cfg.GetDouble("prod_cut_positron_mm", 0.1) * mm, "e+");
    SetCutValue(cfg.GetDouble("prod_cut_proton_mm",   1.0) * mm, "proton");
}
