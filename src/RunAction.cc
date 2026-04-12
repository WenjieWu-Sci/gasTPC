/// @file RunAction.cc
/// @brief Opens/closes output file; fills TTree (ROOT) or CSV (fallback).

#include "RunAction.hh"
#include "ConfigManager.hh"
#include "TPCHit.hh"

#include "G4Run.hh"
#include "G4SystemOfUnits.hh"

#include <fstream>
#include <iostream>

#ifdef HAS_ROOT
#include "TFile.h"
#include "TTree.h"
#endif

// ---- constructor / destructor --------------------------------------------

RunAction::RunAction() {
    fOutputFile = ConfigManager::Instance().GetString("output_file", "output");
}

RunAction::~RunAction() = default;

// ---- BeginOfRunAction ----------------------------------------------------

void RunAction::BeginOfRunAction(const G4Run* run) {
    G4cout << "[RunAction] Starting run #" << run->GetRunID() << G4endl;

#ifdef HAS_ROOT
    const std::string fname = fOutputFile + ".root";
    fRootFile = new TFile(fname.c_str(), "RECREATE");
    if (!fRootFile || fRootFile->IsZombie()) {
        G4cerr << "[RunAction] ERROR: cannot open ROOT file '" << fname << "'\n";
        fRootFile = nullptr;
        return;
    }

    fTree = new TTree("tpc", "Gas TPC Simulation");

    fTree->Branch("eventID",    &fEventID);
    fTree->Branch("nHits",      &fNHits);
    fTree->Branch("posX",       &fPosX);
    fTree->Branch("posY",       &fPosY);
    fTree->Branch("posZ",       &fPosZ);
    fTree->Branch("edep",       &fEdep);
    fTree->Branch("time",       &fTime);
    fTree->Branch("trackID",    &fTrackID);
    fTree->Branch("pdg",        &fPDG);
    fTree->Branch("nElectrons", &fNElectrons);

    G4cout << "[RunAction] ROOT output → " << fname << G4endl;

#else
    fTextFile      = fOutputFile + "_summary.csv";
    fTotalElectrons = 0;
    fTotalHits      = 0;
    fNEvents        = 0;
    G4cout << "[RunAction] Text summary → " << fTextFile << G4endl;
#endif
}

// ---- EndOfRunAction ------------------------------------------------------

void RunAction::EndOfRunAction(const G4Run* run) {
    const int nev = run->GetNumberOfEvent();

#ifdef HAS_ROOT
    if (fRootFile && fTree) {
        fRootFile->Write();
        fRootFile->Close();
        delete fRootFile;
        fRootFile = nullptr;
        fTree     = nullptr;
    }
    G4cout << "[RunAction] Run finished. Events processed: " << nev << G4endl;

#else
    // Write CSV summary
    std::ofstream csv(fTextFile);
    if (csv.is_open()) {
        csv << "# Gas TPC simulation summary\n";
        csv << "# Events," << fNEvents << "\n";
        csv << "# TotalHits," << fTotalHits << "\n";
        csv << "# TotalElectrons," << fTotalElectrons << "\n";
        if (fNEvents > 0) {
            csv << "# AvgHitsPerEvent,"
                << static_cast<double>(fTotalHits) / fNEvents << "\n";
            csv << "# AvgElectronsPerEvent,"
                << static_cast<double>(fTotalElectrons) / fNEvents << "\n";
        }
    }
    G4cout << "[RunAction] Events: "    << nev
           << " | Total hits: "         << fTotalHits
           << " | Total electrons: "    << fTotalElectrons << G4endl;
    (void)nev;
#endif
}

// ---- FillEvent -----------------------------------------------------------

void RunAction::FillEvent(int eventID, const std::vector<TPCHit>& hits) {
#ifdef HAS_ROOT
    if (!fTree) return;

    fEventID = eventID;
    fNHits   = static_cast<int>(hits.size());

    fPosX.clear(); fPosY.clear(); fPosZ.clear();
    fEdep.clear(); fTime.clear();
    fTrackID.clear(); fPDG.clear(); fNElectrons.clear();

    for (const auto& h : hits) {
        fPosX.push_back(h.position.x() / mm);
        fPosY.push_back(h.position.y() / mm);
        fPosZ.push_back(h.position.z() / mm);
        fEdep.push_back(h.energyDeposit / keV);
        fTime.push_back(h.time / ns);
        fTrackID.push_back(h.trackID);
        fPDG.push_back(h.pdgCode);
        fNElectrons.push_back(h.nElectrons);
    }

    fTree->Fill();

#else
    fNEvents++;
    fTotalHits += static_cast<long long>(hits.size());
    for (const auto& h : hits)
        fTotalElectrons += h.nElectrons;
    (void)eventID;
#endif
}
