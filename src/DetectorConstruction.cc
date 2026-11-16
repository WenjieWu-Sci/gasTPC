/// @file DetectorConstruction.cc
/// @brief Builds TPC geometry (builtin C++ or GDML) and applies the E-field.

#include "DetectorConstruction.hh"
#include "ConfigManager.hh"
#include "ElectricField.hh"

#include "G4Box.hh"
#include "G4ChordFinder.hh"
#include "G4ClassicalRK4.hh"
#include "G4Colour.hh"
#include "G4Element.hh"
#include "G4EqMagElectricField.hh"
#include "G4FieldManager.hh"
#ifdef HAS_GDML
#include "G4GDMLParser.hh"
#endif
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4Version.hh"
#if G4VERSION_NUMBER >= 1140
  #include "G4IntegrationDriver.hh"
#else
  #include "G4MagInt_Driver.hh"
#endif
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"

#include <iostream>
#include <stdexcept>

// ---- constructor ---------------------------------------------------------

DetectorConstruction::DetectorConstruction() {
    auto& cfg = ConfigManager::Instance();

    fMode             = cfg.GetString("geometry_mode", "builtin");
    fGDMLFile         = cfg.GetString("gdml_file",     "gdml/simple_tpc.gdml");
    fGasName          = cfg.GetString("gas",            "Xe");
    fGasPressure_bar  = cfg.GetDouble("gas_pressure_bar", 1.0);

    // Convert cm (config) → mm (Geant4 internal)
    fTPCRadius        = cfg.GetDouble("tpc_radius_cm",       10.0) * cm;
    fTPCLength        = cfg.GetDouble("tpc_length_cm",       20.0) * cm;
    fAnodeThickness   = cfg.GetDouble("anode_thickness_cm",   0.1) * cm;
    fCathodeThickness = cfg.GetDouble("cathode_thickness_cm", 0.1) * cm;
    fFieldStrength    = cfg.GetDouble("field_strength_V_per_cm", 500.0);
}

// ---- Construct -----------------------------------------------------------

G4VPhysicalVolume* DetectorConstruction::Construct() {
    if (fMode == "gdml") return ConstructFromGDML();
    return ConstructBuiltin();
}

// ---- Material helpers ----------------------------------------------------

void DetectorConstruction::DefineGasMaterial(const std::string& gasName) {
    auto* nist = G4NistManager::Instance();

    if (gasName == "Ar" || gasName == "Xe") {
        // Noble gas at configurable pressure (ideal-gas density scaling from
        // STP at 293.15 K).  A dedicated material per gas keeps NIST usage
        // untouched when pressure is not configured.
        const bool isAr = (gasName == "Ar");
        const std::string name = isAr ? "ArGas" : "XeGas";
        if (G4Material::GetMaterial(name, false)) return;
        const double z    = isAr ? 18. : 54.;
        const double a    = isAr ? 39.948 : 131.293;
        const double rho  = isAr ? 1.7837e-3 : 5.8582e-3;  // g/cm3 at STP
        const double p_atm = fGasPressure_bar * bar / atmosphere;
        new G4Material(name, z, a * g / mole, rho * p_atm * (g / cm3),
                       kStateGas, 293.15 * kelvin, fGasPressure_bar * bar);
        return;
    }

    if (gasName == "Xe") {
        // Use NIST xenon; falls back to manual definition if unavailable.
        if (!nist->FindOrBuildMaterial("G4_Xe")) {
            new G4Material("G4_Xe", 54., 131.293 * g / mole,
                           5.458e-3 * g / cm3,
                           kStateGas, 293.15 * kelvin, 1.0 * atmosphere);
        }
        return;
    }

    if (gasName == "SF6") {
        if (G4Material::GetMaterial("SF6Gas", false)) return;
        G4Element* S = nist->FindOrBuildElement("S");
        G4Element* F = nist->FindOrBuildElement("F");
        auto* sf6 = new G4Material("SF6Gas", 6.164e-3 * g / cm3, 2,
                                   kStateGas, 293.15 * kelvin, 1.0 * atmosphere);
        sf6->AddElement(S, 1);
        sf6->AddElement(F, 6);
        return;
    }

    if (gasName == "SeF6") {
        if (G4Material::GetMaterial("SeF6Gas", false)) return;
        G4Element* Se = nist->FindOrBuildElement("Se");
        G4Element* F  = nist->FindOrBuildElement("F");
        auto* sef6 = new G4Material("SeF6Gas", 7.887e-3 * g / cm3, 2,
                                    kStateGas, 293.15 * kelvin, 1.0 * atmosphere);
        sef6->AddElement(Se, 1);
        sef6->AddElement(F,  6);
        return;
    }

    G4cerr << "[DetectorConstruction] Unknown gas '" << gasName
           << "' – defaulting to Xe.\n";
    DefineGasMaterial("Xe");
}

static G4Material* GetGasMaterial(const std::string& gasName) {
    if (gasName == "Ar") {
        G4Material* m = G4Material::GetMaterial("ArGas", false);
        if (m) return m;
    } else if (gasName == "Xe") {
        G4Material* m = G4Material::GetMaterial("XeGas", false);
        if (m) return m;
        m = G4NistManager::Instance()->FindOrBuildMaterial("G4_Xe");
        if (m) return m;
    } else if (gasName == "SF6") {
        G4Material* m = G4Material::GetMaterial("SF6Gas", false);
        if (m) return m;
    } else if (gasName == "SeF6") {
        G4Material* m = G4Material::GetMaterial("SeF6Gas", false);
        if (m) return m;
    }
    return G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
}

// ---- ConstructBuiltin ----------------------------------------------------

G4VPhysicalVolume* DetectorConstruction::ConstructBuiltin() {
    DefineGasMaterial(fGasName);

    auto* nist   = G4NistManager::Instance();
    auto* air    = nist->FindOrBuildMaterial("G4_AIR");
    auto* copper = nist->FindOrBuildMaterial("G4_Cu");
    auto* gas    = GetGasMaterial(fGasName);

    G4cout << "[DetectorConstruction] gas=" << fGasName
           << " material=" << gas->GetName()
           << " density=" << gas->GetDensity() / (g / cm3) << " g/cm3"
           << " Z=" << gas->GetZ() << G4endl;

    // ---- World -----------------------------------------------------------
    const G4double worldHalf = 2.0 * m;
    auto* worldSolid  = new G4Box("World", worldHalf, worldHalf, worldHalf);
    auto* worldLogic  = new G4LogicalVolume(worldSolid, air, "World");
    auto* worldPhys   = new G4PVPlacement(nullptr, G4ThreeVector(),
                                          worldLogic, "World",
                                          nullptr, false, 0, true);

    // ---- Drift region (full gas cylinder) --------------------------------
    const G4double halfTPCZ = fTPCLength / 2.0;
    auto* driftSolid = new G4Tubs("DriftRegion",
                                   0., fTPCRadius, halfTPCZ,
                                   0., 360. * deg);
    fDriftLogical = new G4LogicalVolume(driftSolid, gas, "DriftRegion");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0),
                      fDriftLogical, "DriftRegion",
                      worldLogic, false, 0, true);

    // ---- Cathode (copper disk, –Z end) -----------------------------------
    const G4double cathHalf = fCathodeThickness / 2.0;
    auto* cathSolid  = new G4Tubs("Cathode", 0., fTPCRadius, cathHalf, 0., 360. * deg);
    auto* cathLogic  = new G4LogicalVolume(cathSolid, copper, "Cathode");
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, -halfTPCZ + cathHalf),
                      cathLogic, "Cathode",
                      fDriftLogical, false, 0, true);

    // ---- Anode (copper disk, +Z end) -------------------------------------
    const G4double anodeHalf = fAnodeThickness / 2.0;
    auto* anodeSolid = new G4Tubs("Anode", 0., fTPCRadius, anodeHalf, 0., 360. * deg);
    auto* anodeLogic = new G4LogicalVolume(anodeSolid, copper, "Anode");
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, halfTPCZ - anodeHalf),
                      anodeLogic, "Anode",
                      fDriftLogical, false, 0, true);

    // ---- Visualisation attributes ----------------------------------------
    worldLogic->SetVisAttributes(G4VisAttributes::GetInvisible());

    auto* driftVis = new G4VisAttributes(G4Colour(0.2, 0.6, 1.0, 0.25));
    driftVis->SetVisibility(true);
    fDriftLogical->SetVisAttributes(driftVis);

    auto* cuVis = new G4VisAttributes(G4Colour(0.72, 0.45, 0.20));
    cuVis->SetForceSolid(true);
    cathLogic->SetVisAttributes(cuVis);
    anodeLogic->SetVisAttributes(cuVis);

    return worldPhys;
}

// ---- ConstructFromGDML ---------------------------------------------------

G4VPhysicalVolume* DetectorConstruction::ConstructFromGDML() {
#ifdef HAS_GDML
    G4GDMLParser parser;
    parser.Read(fGDMLFile, /*validate=*/false);

    G4VPhysicalVolume* worldPhys = parser.GetWorldVolume();
    if (!worldPhys)
        throw std::runtime_error(
            "[DetectorConstruction] GDML: no world volume in " + fGDMLFile);

    return worldPhys;
#else
    throw std::runtime_error(
        "[DetectorConstruction] geometry_mode=\"gdml\" requested, but gasTPC "
        "was built against a Geant4 without GDML support.");
#endif
}

// ---- ConstructSDandField -------------------------------------------------

void DetectorConstruction::ConstructSDandField() {
    if (fFieldStrength <= 0.0) {
        G4cout << "[DetectorConstruction] field_strength_V_per_cm <= 0: "
                  "no electric field applied.\\n";
        return;
    }

    // Locate the drift logical volume (works for both modes).
    if (!fDriftLogical) {
        fDriftLogical =
            G4LogicalVolumeStore::GetInstance()->GetVolume("DriftRegion", false);
    }

    if (!fDriftLogical) {
        G4cerr << "[DetectorConstruction] Warning: 'DriftRegion' volume not found. "
               << "No electric field will be applied.\n";
        return;
    }

    // ---- Build the field stack -------------------------------------------
    auto* eField      = new ElectricField(fFieldStrength);
    auto* eqOfMotion  = new G4EqMagElectricField(eField);

    // 8 variables: (x, y, z, t, px, py, pz, E)
    const G4int nVar  = 8;
    auto* stepper     = new G4ClassicalRK4(eqOfMotion, nVar);

    const G4double minStep = 0.01 * mm;
#if G4VERSION_NUMBER >= 1140
    auto* intDriver   = new G4IntegrationDriver<G4ClassicalRK4>(
                            minStep, stepper, stepper->GetNumberOfVariables());
#else
    auto* intDriver   = new G4MagInt_Driver(minStep, stepper,
                                             stepper->GetNumberOfVariables());
#endif
    auto* chordFinder = new G4ChordFinder(intDriver);

    auto* fieldMgr    = new G4FieldManager(eField, chordFinder);
    fieldMgr->SetDetectorField(eField);

    // Propagate field into all daughter volumes.
    fDriftLogical->SetFieldManager(fieldMgr, /*forceToAllDaughters=*/true);

    G4cout << "[DetectorConstruction] Electric field: "
           << fFieldStrength << " V/cm applied to 'DriftRegion'.\n";
}
