/// @file PrimaryGeneratorAction.cc
/// @brief Reads particle, energy, position and direction from ConfigManager.

#include "PrimaryGeneratorAction.hh"
#include "ConfigManager.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : fParticleGun(std::make_unique<G4ParticleGun>(1))
{
    auto& cfg = ConfigManager::Instance();

    // Particle type
    const std::string pName = cfg.GetString("particle", "gamma");
    auto* table = G4ParticleTable::GetParticleTable();
    auto* particleDef = table->FindParticle(pName);
    if (!particleDef) {
        G4cerr << "[PrimaryGeneratorAction] Particle '" << pName
               << "' not found – defaulting to gamma.\n";
        particleDef = table->FindParticle("gamma");
    }
    fParticleGun->SetParticleDefinition(particleDef);

    // Kinetic energy (read in keV from config)
    const double energy_keV = cfg.GetDouble("energy_keV", 662.0); // Cs-137 default
    fParticleGun->SetParticleEnergy(energy_keV * keV);

    // Start position – default to centre of TPC
    const double px = cfg.GetDouble("gun_pos_x_cm", 0.0) * cm;
    const double py = cfg.GetDouble("gun_pos_y_cm", 0.0) * cm;
    const double pz = cfg.GetDouble("gun_pos_z_cm", 0.0) * cm;
    fParticleGun->SetParticlePosition(G4ThreeVector(px, py, pz));

    // Direction – default along +Z
    const double dx = cfg.GetDouble("gun_dir_x", 0.0);
    const double dy = cfg.GetDouble("gun_dir_y", 0.0);
    const double dz = cfg.GetDouble("gun_dir_z", 1.0);
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(dx, dy, dz));
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
    fParticleGun->GeneratePrimaryVertex(event);
}
