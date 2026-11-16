#pragma once
/// @file TPCHit.hh
/// @brief Lightweight hit record for energy deposits in the TPC drift volume.

#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4VHit.hh"

class TPCHit : public G4VHit {
public:
    TPCHit()          = default;
    ~TPCHit() override = default;

    /// High-throughput memory management via G4Allocator.
    void* operator new(size_t);
    void  operator delete(void*);

    void Print() override;

    // ---- hit data -------------------------------------------------------
    G4ThreeVector position;
    double energyDeposit = 0.0; ///< MeV
    double time          = 0.0; ///< ns
    int    trackID       = 0;
    int    pdgCode       = 0;
    int    nElectrons    = 0;   ///< ionisation electrons after attachment
    /// Creator process of this track: 0 primary/other, 1 photoelectric,
    /// 2 Compton, 3 Rayleigh.  Lets gamma studies tag the first interaction.
    int    creatorProc   = 0;
};

/// Thread-local allocator instance (defined in EventAction.cc).
extern G4ThreadLocal G4Allocator<TPCHit>* TPCHitAllocator;

// ---------- inline allocator wrappers ------------------------------------
inline void* TPCHit::operator new(size_t) {
    if (!TPCHitAllocator)
        TPCHitAllocator = new G4Allocator<TPCHit>;
    return TPCHitAllocator->MallocSingle();
}

inline void TPCHit::operator delete(void* hit) {
    TPCHitAllocator->FreeSingle(static_cast<TPCHit*>(hit));
}
