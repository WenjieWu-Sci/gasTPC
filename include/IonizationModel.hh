#pragma once
/// @file IonizationModel.hh
/// @brief Models primary ionization in a gas TPC.
///
/// Given an energy deposit, computes the number of electron-ion pairs using
/// the W-value and Fano-factor fluctuation (Gaussian smearing).
/// Optionally applies exponential electron attachment attenuation.

#include "GasProperties.hh"

#include <random>

class IonizationModel {
public:
    explicit IonizationModel(const GasProperties& gas);

    /// Returns the mean number of ion pairs without fluctuations.
    double MeanIonPairs(double energyDeposit_eV) const;

    /// Returns the number of surviving electrons after ionization fluctuations
    /// and electron attachment over @p driftDistance_cm.
    int ComputeElectrons(double energyDeposit_eV, double driftDistance_cm) const;

private:
    GasProperties fGas;
    mutable std::mt19937 fRng;
    mutable std::normal_distribution<double> fNormalDist;
};
