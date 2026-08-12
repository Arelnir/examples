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
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "TFile.h"
#include "TTree.h"

// 静态成员初始化
TFile* RISQTutorialSensitivity::lowEFile = nullptr;
TTree* RISQTutorialSensitivity::lowETree = nullptr;
std::mutex RISQTutorialSensitivity::lowEMutex;
bool RISQTutorialSensitivity::lowEInitialized = false;
int RISQTutorialSensitivity::lowERunID, RISQTutorialSensitivity::lowEEventID,
    RISQTutorialSensitivity::lowETrackID;
double RISQTutorialSensitivity::lowEEnergy;

RISQTutorialSensitivity::RISQTutorialSensitivity(G4String name) :
  G4CMPElectrodeSensitivity(name)
{
  // 初级声子 ROOT 文件
  primaryFile = new TFile("/mnt/sim/g4simbytsc/temp/phonon_primary.root", "RECREATE");
  primaryTree = new TTree("primaryTree", "Primary Phonons");
  primaryTree->Branch("runID", &primRunID, "runID/I");
  primaryTree->Branch("eventID", &primEventID, "eventID/I");
  primaryTree->Branch("particleName", primParticleName, "particleName/C");
  primaryTree->Branch("energy", &primEnergy, "energy/D");
  primaryTree->Branch("x", &primX, "x/D");
  primaryTree->Branch("y", &primY, "y/D");
  primaryTree->Branch("z", &primZ, "z/D");
  primaryTree->Branch("t", &primT, "t/D");

  // 有效电极击中文件
  hitsActiveFile = new TFile("/mnt/sim/g4simbytsc/temp/phonon_hits_active.root", "RECREATE");
  hitsActiveTree = new TTree("hitsTree", "Active Electrode Hits");
  hitsActiveTree->Branch("runID", &hitRunID, "runID/I");
  hitsActiveTree->Branch("eventID", &hitEventID, "eventID/I");
  hitsActiveTree->Branch("trackID", &hitTrackID, "trackID/I");
  hitsActiveTree->Branch("particleName", hitParticleName, "particleName/C");
  hitsActiveTree->Branch("startEnergy", &hitStartEnergy, "startEnergy/D");
  hitsActiveTree->Branch("startX", &hitStartX, "startX/D");
  hitsActiveTree->Branch("startY", &hitStartY, "startY/D");
  hitsActiveTree->Branch("startZ", &hitStartZ, "startZ/D");
  hitsActiveTree->Branch("startTime", &hitStartTime, "startTime/D");
  hitsActiveTree->Branch("eDep", &hitEDep, "eDep/D");
  hitsActiveTree->Branch("weight", &hitWeight, "weight/D");
  hitsActiveTree->Branch("endX", &hitEndX, "endX/D");
  hitsActiveTree->Branch("endY", &hitEndY, "endY/D");
  hitsActiveTree->Branch("endZ", &hitEndZ, "endZ/D");
  hitsActiveTree->Branch("endTime", &hitEndTime, "endTime/D");

  // 无效电极击中文件
  hitsPassiveFile = new TFile("/mnt/sim/g4simbytsc/temp/phonon_hits_passive.root", "RECREATE");
  hitsPassiveTree = new TTree("hitsTree", "Passive Electrode Hits");
  hitsPassiveTree->Branch("runID", &hitRunID, "runID/I");
  hitsPassiveTree->Branch("eventID", &hitEventID, "eventID/I");
  hitsPassiveTree->Branch("trackID", &hitTrackID, "trackID/I");
  hitsPassiveTree->Branch("particleName", hitParticleName, "particleName/C");
  hitsPassiveTree->Branch("startEnergy", &hitStartEnergy, "startEnergy/D");
  hitsPassiveTree->Branch("startX", &hitStartX, "startX/D");
  hitsPassiveTree->Branch("startY", &hitStartY, "startY/D");
  hitsPassiveTree->Branch("startZ", &hitStartZ, "startZ/D");
  hitsPassiveTree->Branch("startTime", &hitStartTime, "startTime/D");
  hitsPassiveTree->Branch("eDep", &hitEDep, "eDep/D");
  hitsPassiveTree->Branch("weight", &hitWeight, "weight/D");
  hitsPassiveTree->Branch("endX", &hitEndX, "endX/D");
  hitsPassiveTree->Branch("endY", &hitEndY, "endY/D");
  hitsPassiveTree->Branch("endZ", &hitEndZ, "endZ/D");
  hitsPassiveTree->Branch("endTime", &hitEndTime, "endTime/D");
}

RISQTutorialSensitivity::~RISQTutorialSensitivity() {
  if (primaryFile) { primaryFile->Write(); primaryFile->Close(); delete primaryFile; }
  if (hitsActiveFile) { hitsActiveFile->Write(); hitsActiveFile->Close(); delete hitsActiveFile; }
  if (hitsPassiveFile) { hitsPassiveFile->Write(); hitsPassiveFile->Close(); delete hitsPassiveFile; }

  std::lock_guard<std::mutex> lock(lowEMutex);
  if (lowEFile) { lowEFile->Write(); lowEFile->Close(); delete lowEFile; lowEFile = nullptr; lowETree = nullptr; }
}

void RISQTutorialSensitivity::RecordLowEnergyPhonon(G4int runID, G4int eventID,
                                                    G4int trackID, G4double energy) {
  std::lock_guard<std::mutex> lock(lowEMutex);
  if (!lowEInitialized) {
    lowEFile = new TFile("/mnt/sim/g4simbytsc/temp/phonon_lowenergy.root", "RECREATE");
    lowETree = new TTree("lowETree", "Low-Energy Phonons");
    lowETree->Branch("runID", &lowERunID, "runID/I");
    lowETree->Branch("eventID", &lowEEventID, "eventID/I");
    lowETree->Branch("trackID", &lowETrackID, "trackID/I");
    lowETree->Branch("energy", &lowEEnergy, "energy/D");
    lowEInitialized = true;
  }
  lowERunID = runID;
  lowEEventID = eventID;
  lowETrackID = trackID;
  lowEEnergy = energy / CLHEP::eV;
  lowETree->Fill();
}

void RISQTutorialSensitivity::EndOfEvent(G4HCofThisEvent* HCE) {
  G4RunManager* runMan = G4RunManager::GetRunManager();
  const G4Event* currentEvent = runMan->GetCurrentEvent();

  // ========== 初级声子写入 ==========
  if (currentEvent->GetNumberOfPrimaryVertex() > 0) {
    G4PrimaryVertex* pv = currentEvent->GetPrimaryVertex(0);
    G4PrimaryParticle* pp = pv->GetPrimary(0);
    primRunID = runMan->GetCurrentRun()->GetRunID();
    primEventID = currentEvent->GetEventID();
    strncpy(primParticleName, pp->GetParticleDefinition()->GetParticleName().c_str(), 19);
    primParticleName[19] = '\0';
    primEnergy = pp->GetTotalEnergy() / CLHEP::eV;
    primX = pv->GetX0() / CLHEP::mm;
    primY = pv->GetY0() / CLHEP::mm;
    primZ = pv->GetZ0() / CLHEP::mm;
    primT = pv->GetT0() / CLHEP::ns;
    primaryTree->Fill();
  }

  // ========== 电极击中（区分有效/无效） ==========
  G4int HCID = G4SDManager::GetSDMpointer()->GetCollectionID(hitsCollection);
  auto* hitCol = static_cast<G4CMPElectrodeHitsCollection*>(HCE->GetHC(HCID));
  std::vector<G4CMPElectrodeHit*>* hitVec = hitCol->GetVector();

  for (G4CMPElectrodeHit* hit : *hitVec) {
    // 击中坐标，单位 mm
    double x = hit->GetFinalPosition().x();
    double y = hit->GetFinalPosition().y();

    bool isLeft  = (x >= xLeftMin  && x <= xLeftMax  && y >= yMin && y <= yMax);
    bool isRight = (x >= xRightMin && x <= xRightMax && y >= yMin && y <= yMax);
    bool isActive = (isLeft || isRight);

    // 填充公共变量
    hitRunID   = runMan->GetCurrentRun()->GetRunID();
    hitEventID = currentEvent->GetEventID();
    hitTrackID = hit->GetTrackID();
    strncpy(hitParticleName, hit->GetParticleName().c_str(), 19);
    hitParticleName[19] = '\0';
    hitStartEnergy = hit->GetStartEnergy() / CLHEP::eV;
    hitStartX = hit->GetStartPosition().getX() / CLHEP::mm;
    hitStartY = hit->GetStartPosition().getY() / CLHEP::mm;
    hitStartZ = hit->GetStartPosition().getZ() / CLHEP::mm;
    hitStartTime = hit->GetStartTime() / CLHEP::ns;
    hitEDep = hit->GetEnergyDeposit() / CLHEP::eV;
    hitWeight = hit->GetWeight();
    hitEndX = x;
    hitEndY = y;
    hitEndZ = hit->GetFinalPosition().getZ();          // mm
    hitEndTime = hit->GetFinalTime() / CLHEP::ns;

    if (isActive)
      hitsActiveTree->Fill();
    else
      hitsPassiveTree->Fill();
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