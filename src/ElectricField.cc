/// @file ElectricField.cc
/// @brief Uniform E-field along –Z (cathode → anode direction for electron drift).

#include "ElectricField.hh"
#include "G4SystemOfUnits.hh"

ElectricField::ElectricField(double fieldStrength_Vcm)
    : fFieldStrength(fieldStrength_Vcm)
{}

void ElectricField::SetFieldStrength(double fieldStrength_Vcm) {
    fFieldStrength = fieldStrength_Vcm;
}

void ElectricField::GetFieldValue(const G4double /*point*/[4],
                                   G4double* field) const {
    // field[0..2] = (Bx, By, Bz) – no magnetic component
    field[0] = 0.0;
    field[1] = 0.0;
    field[2] = 0.0;

    // field[3..5] = (Ex, Ey, Ez) in Geant4 internal units (volt/mm)
    // Physical setup: cathode at –Z, anode at +Z.
    // E-field points from anode (0 V) to cathode (–HV), i.e. in –Z direction.
    // Electrons (q = –e) drift in +Z direction.
    field[3] = 0.0;
    field[4] = 0.0;
    field[5] = -(fFieldStrength * (CLHEP::volt / CLHEP::cm));
}
