/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#include "RISQTutorialSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4CMPUtils.hh"
#include "G4PhononLong.hh"
#include "G4PhononTransFast.hh"
#include "G4PhononTransSlow.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

RISQTutorialSteppingAction::RISQTutorialSteppingAction() {;}

RISQTutorialSteppingAction::~RISQTutorialSteppingAction() {;}

void RISQTutorialSteppingAction::UserSteppingAction(const G4Step* step) {
    // 只对声子进行处理
    G4Track* track = step->GetTrack();
    if (!G4CMP::IsPhonon(track->GetDefinition())) return;

    // 铝的两倍能隙
    constexpr G4double twoDelta = 2 * 173.715e-6 * CLHEP::eV;

    // 如果声子动能低于 2Δ，且仍处于活跃状态，则杀死它
    if (track->GetKineticEnergy() < twoDelta && track->GetTrackStatus() == fAlive) {
        track->SetTrackStatus(fStopAndKill);
    }
}