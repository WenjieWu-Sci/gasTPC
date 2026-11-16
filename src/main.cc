/// @file main.cc
/// @brief Geant4 gas TPC simulation entry point.
///
/// Usage:
///   ./gasTPC                              # interactive
///   ./gasTPC macros/run_builtin.mac       # batch (positional)
///   ./gasTPC --macro macros/run_builtin.mac --config config/detector.json

#include <iostream>
#include <string>

// ---- Geant4 version-aware run-manager header ----------------------------
#include "G4Version.hh"
#if G4VERSION_NUMBER >= 1070
  #include "G4RunManagerFactory.hh"
#else
  #ifdef G4MULTITHREADED
    #include "G4MTRunManager.hh"
  #else
    #include "G4RunManager.hh"
  #endif
#endif

#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

#include "ConfigManager.hh"
#include "DetectorConstruction.hh"
#include "EventAction.hh"
#include "GasProperties.hh"
#include "IonizationModel.hh"
#include "PhysicsList.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

int main(int argc, char** argv) {
    // ---- Parse arguments ------------------------------------------------
    std::string configFile = "config/detector.json";
    std::string macroFile;

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--config" && i + 1 < argc) {
            configFile = argv[++i];
        } else if (arg == "--macro" && i + 1 < argc) {
            macroFile = argv[++i];
        } else if (arg[0] != '-') {
            macroFile = arg;  // positional → treat as macro file
        }
    }

    // ---- Load configuration ---------------------------------------------
    auto& cfg = ConfigManager::Instance();
    cfg.LoadFile("config/gas.json");
    cfg.LoadFile("config/field.json");
    cfg.LoadFile(configFile);   // explicit --config overrides bundled defaults

    // ---- Create run manager ---------------------------------------------
#if G4VERSION_NUMBER >= 1070
    auto* runManager =
        G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
#else
  #ifdef G4MULTITHREADED
    auto* runManager = new G4MTRunManager();
    runManager->SetNumberOfThreads(1);
  #else
    auto* runManager = new G4RunManager();
  #endif
#endif

    // ---- Build ionisation model (shared across user actions) ------------
    const std::string gasName = cfg.GetString("gas", "Xe");
    const GasProperties gasProps = GasPropertiesFactory::GetGas(gasName);
    const IonizationModel ionModel(gasProps);

    // ---- Mandatory initialisation ---------------------------------------
    runManager->SetUserInitialization(new DetectorConstruction());
    runManager->SetUserInitialization(new PhysicsList());

    // ---- User actions ---------------------------------------------------
    auto* runAction   = new RunAction();
    auto* eventAction = new EventAction(runAction);

    runManager->SetUserAction(new PrimaryGeneratorAction());
    runManager->SetUserAction(runAction);
    runManager->SetUserAction(eventAction);
    runManager->SetUserAction(new SteppingAction(eventAction, ionModel));

    // ---- Visualisation --------------------------------------------------
    auto* visManager = new G4VisExecutive("Quiet");
    visManager->Initialize();

    // ---- UI -------------------------------------------------------------
    G4UImanager* UI = G4UImanager::GetUIpointer();

    if (macroFile.empty()) {
        // Interactive mode
        G4UIExecutive* ui = new G4UIExecutive(argc, argv);
        UI->ApplyCommand("/run/initialize");
        ui->SessionStart();
        delete ui;
    } else {
        // Batch mode
        UI->ApplyCommand("/run/initialize");
        UI->ApplyCommand("/control/execute " + macroFile);
    }

    // ---- Cleanup --------------------------------------------------------
    delete visManager;
    delete runManager;
    return 0;
}
