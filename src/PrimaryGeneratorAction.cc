/// @file PrimaryGeneratorAction.cc
/// @brief Reads particle, energy, position and direction from ConfigManager.
///
/// source_mode:
///   "fixed" – single direction from config (stock behaviour)
///   "cone"  – uniform sampling in theta in [cone_theta_min_deg, cone_theta_max_deg]
///             and phi in [cone_phi_min_deg, cone_phi_max_deg] (degrees, theta from +Z)
///   "slit"  – uniform points on a horizontal source disk; isotropic downward
///             directions rejected unless they thread the tilted cylindrical
///             slit hole (geometric transmission, no slit material simulated)

#include "PrimaryGeneratorAction.hh"
#include "ConfigManager.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

#include <cmath>
#include <sstream>
#include <string>

namespace {

G4ThreeVector ParseVec3(const std::string& s, const G4ThreeVector& def) {
    std::stringstream ss(s);
    double x, y, z;
    char c1, c2;
    if (ss >> x >> c1 >> y >> c2 >> z) return {x, y, z};
    return def;
}

} // namespace

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : fParticleGun(std::make_unique<G4ParticleGun>(1))
{
    auto& cfg = ConfigManager::Instance();

    G4long seed = cfg.GetDouble("random_seed", 12345);
    G4Random::setTheSeed(seed);

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
    fPosition = G4ThreeVector(px, py, pz);
    fParticleGun->SetParticlePosition(fPosition);

    fSourceMode = cfg.GetString("source_mode", "fixed");

    if (fSourceMode == "cone") {
        fThetaMin = cfg.GetDouble("cone_theta_min_deg", 90.0) * deg;
        fThetaMax = cfg.GetDouble("cone_theta_max_deg", 90.0) * deg;
        fPhiMin   = cfg.GetDouble("cone_phi_min_deg", -10.23) * deg;
        fPhiMax   = cfg.GetDouble("cone_phi_max_deg",  10.23) * deg;
        // false: theta uniform in [min,max];  true: cos(theta) uniform
        // (uniform over solid angle) — sensitivity check for the report's
        // unspecified convention.
        fThetaCosine = cfg.GetBool("cone_theta_cosine", false);
    } else if (fSourceMode == "slit") {
        // Fixed emission point is replaced by disk sampling; keep fPosition as
        // the disk centre (use gun_pos_*_cm as before).
        fDiskRadius = 0.5 * cfg.GetDouble("source_disk_diameter_mm", 10.0) * mm;
        fSlitCenter = ParseVec3(cfg.GetString("slit_center_mm", "0,0,119.0"),
                                G4ThreeVector(0, 0, 119.0)) * mm;
        fSlitRadius = 0.5 * cfg.GetDouble("slit_hole_diameter_mm", 0.5) * mm;
        const double tilt     = cfg.GetDouble("slit_tilt_deg", 30.0) * deg;
        const double azim     = cfg.GetDouble("slit_tilt_azimuth_deg", 0.0) * deg;
        const double thick    = cfg.GetDouble("slit_thickness_mm", 1.0) * mm;
        // Hole axis, pointing downward into the TPC.
        fSlitAxis = G4ThreeVector(std::cos(tilt) * std::cos(azim),
                                  std::cos(tilt) * std::sin(azim),
                                  -std::sin(tilt)).unit();
        // Hole length through the tilted plate: plate thickness / sin(tilt).
        fSlitBottom = fSlitCenter + fSlitAxis * (thick / std::sin(tilt));
    } else {
        // "fixed": single direction from config (stock behaviour)
        const double dx = cfg.GetDouble("gun_dir_x", 0.0);
        const double dy = cfg.GetDouble("gun_dir_y", 0.0);
        const double dz = cfg.GetDouble("gun_dir_z", 1.0);
        fDirection = G4ThreeVector(dx, dy, dz).unit();
        fParticleGun->SetParticleMomentumDirection(fDirection);
    }
}

G4ThreeVector PrimaryGeneratorAction::SampleConeDirection() const {
    double theta;
    if (fThetaCosine) {
        // uniform in cos(theta) over [thetaMin, thetaMax]
        const double cmin = std::cos(fThetaMax);  // cos decreases with theta
        const double cmax = std::cos(fThetaMin);
        theta = std::acos(cmin + (cmax - cmin) * G4UniformRand());
    } else {
        theta = fThetaMin + (fThetaMax - fThetaMin) * G4UniformRand();
    }
    const double phi   = fPhiMin   + (fPhiMax   - fPhiMin  ) * G4UniformRand();
    return {std::sin(theta) * std::cos(phi),
            std::sin(theta) * std::sin(phi),
            std::cos(theta)};
}

G4ThreeVector PrimaryGeneratorAction::SampleDiskPoint() const {
    const double r   = fDiskRadius * std::sqrt(G4UniformRand());
    const double phi = 2.0 * M_PI * G4UniformRand();
    return fPosition + G4ThreeVector(r * std::cos(phi), r * std::sin(phi), 0.0);
}

bool PrimaryGeneratorAction::ThreadsSlit(const G4ThreeVector& point,
                                         const G4ThreeVector& dir) const {
    // Ray must cross the top opening plane within the hole radius of the top
    // centre, and the bottom opening plane within the hole radius of the
    // bottom centre (the two openings are the limiting apertures).
    const double denom = dir.dot(fSlitAxis);
    if (denom <= 0.0) return false;  // must travel along the hole axis
    for (const G4ThreeVector& c : {fSlitCenter, fSlitBottom}) {
        const double t = (c - point).dot(fSlitAxis) / denom;
        if (t <= 0.0) return false;
        const G4ThreeVector hit = point + t * dir;
        if ((hit - c).mag() > fSlitRadius) return false;
    }
    return true;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
    if (fSourceMode == "cone") {
        fParticleGun->SetParticlePosition(fPosition);
        fParticleGun->SetParticleMomentumDirection(SampleConeDirection());
    } else if (fSourceMode == "slit") {
        G4ThreeVector point, dir;
        do {
            point = SampleDiskPoint();
            // Isotropic over the downward hemisphere.
            const double u  = G4UniformRand();          // cos(theta), theta from -Z
            const double ph = 2.0 * M_PI * G4UniformRand();
            const double s  = std::sqrt(std::max(0.0, 1.0 - u * u));
            dir = {s * std::cos(ph), s * std::sin(ph), -u};
        } while (!ThreadsSlit(point, dir));
        fParticleGun->SetParticlePosition(point);
        fParticleGun->SetParticleMomentumDirection(dir);
    }
    fParticleGun->GeneratePrimaryVertex(event);
}
