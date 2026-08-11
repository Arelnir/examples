/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file RISQTutorial/RISQTutorial.cc
/// \brief Main program of the RISQTutorial example (based on G4CMP's phonon example)

#include "G4RunManager.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

#include "G4CMPPhysicsList.hh"              // 替换原来的 FTFP_BERT + G4CMPPhysics
#include "G4CMPConfigManager.hh"
#include "RISQTutorialActionInitialization.hh"
#include "RISQTutorialConfigManager.hh"
#include "RISQTutorialDetectorConstruction.hh"

int main(int argc,char** argv)
{
  // Construct the run manager
  G4RunManager * runManager = new G4RunManager;

  // Set mandatory initialization classes
  RISQTutorialDetectorConstruction* detector = new RISQTutorialDetectorConstruction();
  runManager->SetUserInitialization(detector);

  // Use G4CMPPhysicsList, which includes only G4CMP processes (phonons, charges)
  G4VUserPhysicsList* physics = new G4CMPPhysicsList();
  physics->SetCuts();
  runManager->SetUserInitialization(physics);

  // Set user action classes
  runManager->SetUserInitialization(new RISQTutorialActionInitialization);

  // Create configuration managers to ensure macro commands exist
  G4CMPConfigManager::Instance();
  RISQTutorialConfigManager::Instance();

  // Visualization manager
  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();

  // Get the pointer to the User Interface manager
  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  if (argc==1)   // Define UI session for interactive mode
  {
       G4UIExecutive * ui = new G4UIExecutive(argc,argv);
       ui->SessionStart();
       delete ui;
  }
  else           // Batch mode
  {
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command+fileName);
  }

  delete visManager;
  delete runManager;

  return 0;
}