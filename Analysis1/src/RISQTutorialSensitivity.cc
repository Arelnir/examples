/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#include "RISQTutorialSensitivity.hh"
#include "G4CMPElectrodeHit.hh"
#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4PhononLong.hh"
#include "G4PhononTransFast.hh"
#include "G4PhononTransSlow.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "RISQTutorialConfigManager.hh"
#include <fstream>
#include <iostream>

RISQTutorialSensitivity::RISQTutorialSensitivity(G4String name) :
  G4CMPElectrodeSensitivity(name), primaryFileName(""), hitFileName("") {
  SetHitOutputFile(RISQTutorialConfigManager::GetHitOutput());
  SetPrimaryOutputFile(RISQTutorialConfigManager::GetPrimaryOutput());
}

RISQTutorialSensitivity::~RISQTutorialSensitivity() {
  if (primaryOutput.is_open()) primaryOutput.close();
  if (hitOutput.is_open()) hitOutput.close();
}

void RISQTutorialSensitivity::EndOfEvent(G4HCofThisEvent* HCE) {
  G4int HCID = G4SDManager::GetSDMpointer()->GetCollectionID(hitsCollection);
  auto* hitCol = static_cast<G4CMPElectrodeHitsCollection*>(HCE->GetHC(HCID));
  std::vector<G4CMPElectrodeHit*>* hitVec = hitCol->GetVector();

  G4RunManager* runMan = G4RunManager::GetRunManager();

  // --- primary output ---
  const G4Event* currentEvent = runMan->GetCurrentEvent();
  if (primaryOutput.good() && currentEvent->GetNumberOfPrimaryVertex() > 0) {
    G4PrimaryVertex* primaryVertex = currentEvent->GetPrimaryVertex(0);
    G4PrimaryParticle* primary = primaryVertex->GetPrimary(0);
    primaryOutput << runMan->GetCurrentRun()->GetRunID() << ' '
                  << runMan->GetCurrentEvent()->GetEventID() << ' '
                  << primary->GetParticleDefinition()->GetParticleName() << ' '
                  << primary->GetTotalEnergy()/CLHEP::eV << ' '
                  << primaryVertex->GetX0()/CLHEP::mm << ' '
                  << primaryVertex->GetY0()/CLHEP::mm << ' '
                  << primaryVertex->GetZ0()/CLHEP::mm << ' '
                  << primaryVertex->GetT0()/CLHEP::ns << '\n';
  }

  // --- hit output (simplified) ---
  const bool enableCoordFilter = true;
  const double xLeftMin  = -3.775, xLeftMax  = -3.325;
  const double xRightMin =  3.325, xRightMax =  3.775;
  const double yMin = -0.52, yMax = 0.077665;

  if (hitOutput.good()) {
    for (G4CMPElectrodeHit* hit : *hitVec) {
      double x = hit->GetFinalPosition().x();
      double y = hit->GetFinalPosition().y();

      bool inLeft  = (x >= xLeftMin  && x <= xLeftMax  && y >= yMin && y <= yMax);
      bool inRight = (x >= xRightMin && x <= xRightMax && y >= yMin && y <= yMax);
      if (enableCoordFilter && !inLeft && !inRight) continue;

      hitOutput << runMan->GetCurrentRun()->GetRunID() << ','
                << runMan->GetCurrentEvent()->GetEventID() << ','
                << hit->GetTrackID() << ','
                << hit->GetEnergyDeposit()/CLHEP::eV << '\n';
    }
  }
}

void RISQTutorialSensitivity::SetHitOutputFile(const G4String &fn) {
  if (hitFileName != fn) {
    if (hitOutput.is_open()) hitOutput.close();
    hitFileName = fn;
    hitOutput.open(hitFileName, std::ios_base::app);
    if (!hitOutput.good()) {
      G4ExceptionDescription msg;
      msg << "Error opening hit output file " << hitFileName;
      G4Exception("RISQTutorialSensitivity::SetHitOutputFile", "PhonSense003",
                  FatalException, msg);
      hitOutput.close();
    } else {
      hitOutput << "Run ID,Event ID,Track ID,Energy Deposited [eV]\n";
    }
  }
}

void RISQTutorialSensitivity::SetPrimaryOutputFile(const G4String &fn) {
  if (primaryFileName != fn) {
    if (primaryOutput.is_open()) primaryOutput.close();
    primaryFileName = fn;
    primaryOutput.open(primaryFileName, std::ios_base::app);
    if (!primaryOutput.good()) {
      G4ExceptionDescription msg;
      msg << "Error opening output file " << primaryFileName;
      G4Exception("RISQTutorialSensitivity::SetPrimaryOutputFile", "PhonSense003",
                  FatalException, msg);
      primaryOutput.close();
    } else {
      primaryOutput << "Run ID,Event ID,Particle Name,Start Energy [eV],"
                    << "Start X [mm],Start Y [mm],Start Z [mm],Start Time [ns]\n";
    }
  }
}

G4bool RISQTutorialSensitivity::IsHit(const G4Step* step,
                                      const G4TouchableHistory*) const {
  const G4Track* track = step->GetTrack();
  const G4StepPoint* postStepPoint = step->GetPostStepPoint();
  const G4ParticleDefinition* particle = track->GetDefinition();

  G4bool correctParticle = (particle == G4PhononLong::Definition() ||
                            particle == G4PhononTransFast::Definition() ||
                            particle == G4PhononTransSlow::Definition());

  G4bool correctStatus = (track->GetTrackStatus() == fStopAndKill &&
                          postStepPoint->GetStepStatus() == fGeomBoundary &&
                          step->GetNonIonizingEnergyDeposit() > 0.);

  return correctParticle && correctStatus;
}