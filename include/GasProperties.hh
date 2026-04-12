#pragma once
/// @file GasProperties.hh
/// @brief Gas property data and factory for the ionization model.
///
/// Supported gases: Xe (xenon), SF6, SeF6.

#include <stdexcept>
#include <string>

/// Plain data structure that carries all gas parameters needed by the simulation.
struct GasProperties {
    std::string name;
    double wValue;          ///< W-value in eV (mean energy per ion pair)
    double fanoFactor;      ///< Fano factor (dimensionless)
    double attachmentCoeff; ///< Electron attachment coefficient (1/cm); 0 for non-attaching
    double driftVelocity;   ///< Electron drift velocity (cm/µs) at nominal field
    double diffusionT;      ///< Transverse diffusion coefficient (cm/√cm)
    double diffusionL;      ///< Longitudinal diffusion coefficient (cm/√cm)
};

/// Factory that returns pre-set GasProperties for well-known gases.
class GasPropertiesFactory {
public:
    /// @throws std::runtime_error for unknown gas names.
    static GasProperties GetGas(const std::string& name);
};
