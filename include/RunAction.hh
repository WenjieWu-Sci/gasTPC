#pragma once
/// @file RunAction.hh
/// @brief Opens/closes the output file and owns the output tree / text stream.
///
/// When compiled with ROOT (HAS_ROOT defined), output is written to a TTree in
/// a ROOT file.  Without ROOT, a plain CSV summary is written instead.
/// Both modes can additionally write per-hit steps and per-event aggregates.

#include "TPCHit.hh"
#include "G4ThreeVector.hh"
#include "G4UserRunAction.hh"

#include <fstream>
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

    /// Called by EventAction at the end of every event (also zero-hit ones).
    void FillEvent(int eventID, const std::vector<TPCHit>& hits,
                   const G4ThreeVector& primPos, const G4ThreeVector& primDir);

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

    // Per-hit step records (written in both build modes when enabled).
    // Text mode: "<output>_steps.csv".  Controlled by config key
    // "record_steps" (default: on in text mode, off with ROOT).
    bool        fRecordSteps = false;
    std::string fStepsFile;
    std::ofstream fSteps;

    // Per-event aggregates: "<output>_events.csv" (both build modes).
    std::string fEventsFile;
    std::ofstream fEvents;
};
