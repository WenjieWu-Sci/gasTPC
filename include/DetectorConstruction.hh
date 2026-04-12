#pragma once
/// @file DetectorConstruction.hh
/// @brief Builds the TPC geometry either from C++ ("builtin") or from a GDML file.
///
/// Builtin mode creates a cylindrical TPC with:
///   - gas-filled drift region ("DriftRegion" logical volume)
///   - thin copper cathode at the –Z face
///   - thin copper anode  at the +Z face
///
/// GDML mode reads a GDML file and locates the "DriftRegion" logical volume
/// inside it.  The uniform electric field is applied to that volume in both modes.

#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VUserDetectorConstruction.hh"

#include <memory>
#include <string>

class ElectricField;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
    DetectorConstruction();
    ~DetectorConstruction() override = default;

    G4VPhysicalVolume* Construct()       override;
    void ConstructSDandField()           override;

    G4LogicalVolume* GetDriftLogical() const { return fDriftLogical; }

private:
    G4VPhysicalVolume* ConstructBuiltin();
    G4VPhysicalVolume* ConstructFromGDML();

    /// Build (or retrieve) the gas G4Material for @p gasName.
    void DefineGasMaterial(const std::string& gasName);

    // ---- config parameters -----------------------------------------------
    std::string fMode;              ///< "builtin" or "gdml"
    std::string fGDMLFile;
    std::string fGasName;

    double fTPCRadius;          ///< mm (Geant4 units)
    double fTPCLength;          ///< mm (Geant4 units) – full length
    double fAnodeThickness;     ///< mm
    double fCathodeThickness;   ///< mm
    double fFieldStrength;      ///< V/cm

    G4LogicalVolume* fDriftLogical = nullptr;
};
