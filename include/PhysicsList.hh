#pragma once
/// @file PhysicsList.hh
/// @brief Physics list based on G4EmStandardPhysics_option4 (accurate EM).
///
/// G4EmStandardPhysics_option4 provides the best accuracy for low-energy
/// electromagnetic processes – well suited for ionisation in gas detectors.

#include "G4VModularPhysicsList.hh"

class PhysicsList : public G4VModularPhysicsList {
public:
    PhysicsList();
    ~PhysicsList() override = default;

    void SetCuts() override;
};
