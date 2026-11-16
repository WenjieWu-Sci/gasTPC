#pragma once
/// @file PrimaryGeneratorAction.hh
/// @brief Configurable primary particle source using G4ParticleGun.
///
/// Particle type, kinetic energy, start position, and direction are read from
/// ConfigManager at construction time.  All parameters can also be overridden
/// interactively via /gun/ UI commands.  source_mode selects fixed, cone or
/// slit sampling (see the .cc file).

#include "G4ParticleGun.hh"
#include "G4ThreeVector.hh"
#include "G4VUserPrimaryGeneratorAction.hh"

#include <memory>
#include <string>

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override = default;

    void GeneratePrimaries(G4Event* event) override;

    G4ParticleGun* GetParticleGun() { return fParticleGun.get(); }

private:
    G4ThreeVector SampleConeDirection() const;
    G4ThreeVector SampleDiskPoint() const;
    bool ThreadsSlit(const G4ThreeVector& point, const G4ThreeVector& dir) const;

    std::unique_ptr<G4ParticleGun> fParticleGun;

    std::string   fSourceMode = "fixed";
    G4ThreeVector fPosition;
    G4ThreeVector fDirection = {0, 0, 1};

    // cone mode
    double fThetaMin = 0.0, fThetaMax = 0.0;
    double fPhiMin   = 0.0, fPhiMax   = 0.0;
    bool   fThetaCosine = false;

    // slit mode
    double        fDiskRadius = 5.0;  // mm (Geant4 units set in ctor)
    G4ThreeVector fSlitCenter;
    G4ThreeVector fSlitBottom;
    G4ThreeVector fSlitAxis;
    double        fSlitRadius = 0.25;
};
