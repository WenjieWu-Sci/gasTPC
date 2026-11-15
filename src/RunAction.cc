/// @file RunAction.cc
/// @brief Opens/closes output file; fills TTree (ROOT) or CSV (fallback).

#include "RunAction.hh"
#include "ConfigManager.hh"
#include "TPCHit.hh"

#include "G4Run.hh"
#include "G4SystemOfUnits.hh"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

#ifdef HAS_ROOT
#include "TFile.h"
#include "TTree.h"
#endif

// ---- constructor / destructor --------------------------------------------

RunAction::RunAction() {
    auto& cfg = ConfigManager::Instance();
    fOutputFile = cfg.GetString("output_file", "output");
#ifdef HAS_ROOT
    fRecordSteps = cfg.GetBool("record_steps", false);
#else
    fRecordSteps = cfg.GetBool("record_steps", true);
#endif
    fStepsFile  = fOutputFile + "_steps.csv";
    fEventsFile = fOutputFile + "_events.csv";
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

    if (fRecordSteps) {
        fSteps.open(fStepsFile);
        if (!fSteps.is_open()) {
            G4cerr << "[RunAction] ERROR: cannot open steps file '"
                   << fStepsFile << "'\n";
            fRecordSteps = false;
        } else {
            fSteps << "eventID,trackID,pdg,x_mm,y_mm,z_mm,edep_keV,n_e,creator\n";
            G4cout << "[RunAction] Step records → " << fStepsFile << G4endl;
        }
    }

    fEvents.open(fEventsFile);
    if (!fEvents.is_open()) {
        G4cerr << "[RunAction] ERROR: cannot open events file '"
               << fEventsFile << "'\n";
    } else {
        fEvents << "eventID,n_hits,edep_total_keV,zmin_mm,zmax_mm,rmax_mm,"
                   "x0_mm,y0_mm,z0_mm,theta_deg,phi_deg\n";
        G4cout << "[RunAction] Event records → " << fEventsFile << G4endl;
    }
}

// ---- EndOfRunAction ------------------------------------------------------

void RunAction::EndOfRunAction(const G4Run* run) {
    const int nev = run->GetNumberOfEvent();

    if (fSteps.is_open())  fSteps.close();
    if (fEvents.is_open()) fEvents.close();

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
    if (!csv.is_open()) {
        G4cerr << "[RunAction] ERROR: cannot open text summary file '"
               << fTextFile << "'\n";
    } else {
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
#endif
}

// ---- FillEvent -----------------------------------------------------------

void RunAction::FillEvent(int eventID, const std::vector<TPCHit>& hits,
                          const G4ThreeVector& primPos,
                          const G4ThreeVector& primDir) {
    if (fRecordSteps) {
        for (const auto& h : hits) {
            fSteps << eventID                    << ','
                   << h.trackID                  << ','
                   << h.pdgCode                  << ','
                   << h.position.x() / mm        << ','
                   << h.position.y() / mm        << ','
                   << h.position.z() / mm        << ','
                   << h.energyDeposit / keV      << ','
                   << h.nElectrons               << ','
                   << h.creatorProc              << '\n';
        }
    }

    if (fEvents.is_open()) {
        double edep = 0.0, zmin = 0.0, zmax = 0.0, rmax = 0.0;
        bool first = true;
        for (const auto& h : hits) {
            edep += h.energyDeposit / keV;
            const double z = h.position.z() / mm;
            const double r = std::hypot(h.position.x() / mm,
                                        h.position.y() / mm);
            if (first) { zmin = zmax = z; rmax = r; first = false; }
            else {
                zmin = std::min(zmin, z); zmax = std::max(zmax, z);
                rmax = std::max(rmax, r);
            }
        }
        const double theta = primDir.theta() / deg;
        const double phi   = primDir.phi()   / deg;
        fEvents << eventID                 << ','
                << hits.size()             << ','
                << edep                    << ','
                << (first ? -999.0 : zmin) << ','
                << (first ? -999.0 : zmax) << ','
                << (first ? -999.0 : rmax) << ','
                << primPos.x() / mm        << ','
                << primPos.y() / mm        << ','
                << primPos.z() / mm        << ','
                << theta                   << ','
                << phi                     << '\n';
    }

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
