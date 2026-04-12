/// @file IonizationModel.cc
/// @brief Fano-factor fluctuations + exponential electron attachment.

#include "IonizationModel.hh"

#include <algorithm>
#include <cmath>
#include <random>

IonizationModel::IonizationModel(const GasProperties& gas)
    : fGas(gas),
      fRng(std::random_device{}()),
      fNormalDist(0.0, 1.0)
{}

double IonizationModel::MeanIonPairs(double energyDeposit_eV) const {
    if (fGas.wValue <= 0.0) return 0.0;
    return energyDeposit_eV / fGas.wValue;
}

int IonizationModel::ComputeElectrons(double energyDeposit_eV,
                                       double driftDistance_cm) const {
    const double mean = MeanIonPairs(energyDeposit_eV);
    if (mean <= 0.0) return 0;

    // ---- Fano-factor fluctuation (Gaussian approximation) ----------------
    const double sigma = std::sqrt(fGas.fanoFactor * mean);
    const double nRaw  = mean + sigma * fNormalDist(fRng);
    int nElectrons = static_cast<int>(std::max(0.0, std::round(nRaw)));

    // ---- Electron attachment (exponential survival probability) ----------
    if (fGas.attachmentCoeff > 0.0 && driftDistance_cm > 0.0 && nElectrons > 0) {
        const double survival = std::exp(-fGas.attachmentCoeff * driftDistance_cm);
        std::binomial_distribution<int> binom(nElectrons, survival);
        nElectrons = binom(fRng);
    }

    return nElectrons;
}
