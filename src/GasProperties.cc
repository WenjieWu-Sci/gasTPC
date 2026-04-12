/// @file GasProperties.cc
/// @brief Pre-set gas parameters for Xe, SF6, and SeF6.

#include "GasProperties.hh"

GasProperties GasPropertiesFactory::GetGas(const std::string& name) {
    // -----------------------------------------------------------------------
    // Xenon (non-attaching noble gas, excellent for TPCs)
    // W = 21.9 eV  [ICRU Report 31]
    // Fano = 0.17  [Alkhazov et al.]
    // -----------------------------------------------------------------------
    if (name == "Xe") {
        return {
            "Xe",
            21.9,   // wValue        [eV]
            0.17,   // fanoFactor
            0.0,    // attachmentCoeff [1/cm]  – noble gas, no attachment
            2.0,    // driftVelocity  [cm/µs]  at ~500 V/cm
            0.013,  // diffusionT     [cm/√cm]
            0.012   // diffusionL     [cm/√cm]
        };
    }

    // -----------------------------------------------------------------------
    // SF6 – strong electron-attaching gas (negative-ion TPC candidate)
    // W = 34.0 eV, Fano = 0.57  [Christophorou et al.]
    // High attachment coeff → electrons captured quickly
    // -----------------------------------------------------------------------
    if (name == "SF6") {
        return {
            "SF6",
            34.0,   // wValue        [eV]
            0.57,   // fanoFactor
            0.05,   // attachmentCoeff [1/cm]
            0.1,    // driftVelocity  [cm/µs]
            0.030,  // diffusionT
            0.025   // diffusionL
        };
    }

    // -----------------------------------------------------------------------
    // SeF6 – selenium hexafluoride (experimental negative-ion TPC gas)
    // Parameters estimated from analogy with SF6 and literature
    // -----------------------------------------------------------------------
    if (name == "SeF6") {
        return {
            "SeF6",
            33.0,   // wValue        [eV]
            0.55,   // fanoFactor
            0.03,   // attachmentCoeff [1/cm]
            0.2,    // driftVelocity  [cm/µs]
            0.028,  // diffusionT
            0.022   // diffusionL
        };
    }

    throw std::runtime_error("GasPropertiesFactory: unknown gas '" + name + "'");
}
