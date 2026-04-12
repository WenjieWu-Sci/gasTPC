#pragma once
/// @file ElectricField.hh
/// @brief Uniform electric field along the Z-axis for the TPC drift region.
///
/// Implements the G4ElectroMagneticField interface so Geant4's transport
/// machinery applies the force on charged primaries inside the drift volume.

#include "G4ElectroMagneticField.hh"
#include "G4ThreeVector.hh"

class ElectricField : public G4ElectroMagneticField {
public:
    /// @param fieldStrength_Vcm  Field magnitude in V/cm (positive = +Z direction).
    explicit ElectricField(double fieldStrength_Vcm);

    /// Fills @p field[0..2] = (Bx,By,Bz) and @p field[3..5] = (Ex,Ey,Ez)
    /// in Geant4 internal units.
    void GetFieldValue(const G4double point[4], G4double* field) const override;

    /// The electric field changes particle energy → return true.
    G4bool DoesFieldChangeEnergy() const override { return true; }

    void   SetFieldStrength(double fieldStrength_Vcm);
    double GetFieldStrength() const { return fFieldStrength; }

private:
    double fFieldStrength; ///< Stored in V/cm; converted on the fly in GetFieldValue.
};
