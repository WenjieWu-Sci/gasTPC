#pragma once
/// @file RunAction.hh
/// @brief Opens/closes the output file and owns the output tree / text stream.
///
/// When compiled with ROOT (HAS_ROOT defined), output is written to a TTree in
/// a ROOT file.  Without ROOT, a plain CSV summary is written instead.

#include "TPCHit.hh"
#include "G4UserRunAction.hh"

#include <string>
#include <vector>

class G4Run;

#ifdef HAS_ROOT
#include "TFile.h"
#include "TTree.h"
#endif

class RunAction : public G4UserRunAction {
public:
    RunAction();
    ~RunAction() override;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction  (const G4Run* run) override;

    /// Called by EventAction at the end of every event.
    void FillEvent(int eventID, const std::vector<TPCHit>& hits);

private:
    std::string fOutputFile;

#ifdef HAS_ROOT
    TFile* fRootFile = nullptr;
    TTree* fTree     = nullptr;

    // Branch variables (filled per event)
    int                 fEventID   = 0;
    int                 fNHits     = 0;
    std::vector<double> fPosX, fPosY, fPosZ;
    std::vector<double> fEdep;
    std::vector<double> fTime;
    std::vector<int>    fTrackID;
    std::vector<int>    fPDG;
    std::vector<int>    fNElectrons;
#else
    // Text-file fallback
    std::string fTextFile;
    long long   fTotalElectrons = 0;
    long long   fTotalHits      = 0;
    int         fNEvents        = 0;
#endif
};
