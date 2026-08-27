/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#include "RISQTutorialSteppingAction.hh"
#include "RISQTutorialSensitivity.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4CMPUtils.hh"

RISQTutorialSteppingAction::RISQTutorialSteppingAction() {;}

RISQTutorialSteppingAction::~RISQTutorialSteppingAction() {;}

void RISQTutorialSteppingAction::UserSteppingAction(const G4Step* step) {
  G4Track* track = step->GetTrack();
  if (!G4CMP::IsPhonon(track->GetDefinition())) return;

  constexpr double twoDelta = 2 * 173.715e-6 * CLHEP::eV;
  G4double energy = track->GetKineticEnergy();

  if (energy < twoDelta && track->GetTrackStatus() == fAlive) {
    // 先记录，再杀死
    RISQTutorialSensitivity::RecordLowEnergyPhonon(
        G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID(),
        G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID(),
        track->GetTrackID(),
        energy);

    track->SetTrackStatus(fStopAndKill);
  }
}