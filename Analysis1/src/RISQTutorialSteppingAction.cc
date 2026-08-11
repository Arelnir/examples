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
#include <fstream>
#include <iostream>
#include <mutex>

namespace {
    constexpr double twoDelta = 2 * 173.715e-6 * CLHEP::eV; // Al 2Δ
    std::ofstream lowEOutput;
    std::mutex lowEMutex;
    bool lowEFileInitialized = false;
}

RISQTutorialSteppingAction::RISQTutorialSteppingAction() {;}

RISQTutorialSteppingAction::~RISQTutorialSteppingAction() {;}

void RISQTutorialSteppingAction::UserSteppingAction(const G4Step* step) {
    G4Track* track = step->GetTrack();
    if (!G4CMP::IsPhonon(track->GetDefinition())) return;

    G4double energy = track->GetKineticEnergy();
    if (energy < twoDelta && track->GetTrackStatus() == fAlive) {
        // ---------- 记录低能声子 ----------
        std::lock_guard<std::mutex> lock(lowEMutex);
        if (!lowEFileInitialized) {
            lowEOutput.open("phonon_lowenergy.txt", std::ios_base::app);
            lowEFileInitialized = true;
            if (lowEOutput.good()) {
                lowEOutput << "Run ID,Event ID,Track ID,Energy [eV]" << std::endl;
            }
        }
        if (lowEOutput.good()) {
            lowEOutput << G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID() << ','
                       << G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID() << ','
                       << track->GetTrackID() << ','
                       << energy / CLHEP::eV << '\n';
        }

        // ---------- 杀死声子 ----------
        track->SetTrackStatus(fStopAndKill);
    }
}