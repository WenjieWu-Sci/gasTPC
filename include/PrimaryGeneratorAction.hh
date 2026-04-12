#pragma once
/// @file PrimaryGeneratorAction.hh
/// @brief Configurable primary particle source using G4ParticleGun.
///
/// Particle type, kinetic energy, start position, and direction are read from
/// ConfigManager at construction time.  All parameters can also be overridden
/// interactively via /gun/ UI commands.

#include "G4ParticleGun.hh"
#include "G4VUserPrimaryGeneratorAction.hh"

#include <memory>

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override = default;

    void GeneratePrimaries(G4Event* event) override;

    G4ParticleGun* GetParticleGun() { return fParticleGun.get(); }

private:
    std::unique_ptr<G4ParticleGun> fParticleGun;
};
