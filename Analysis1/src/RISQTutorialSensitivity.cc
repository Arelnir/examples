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

RISQTutorialSensitivity::RISQTutorialSensitivity(G4String name) :
  G4CMPElectrodeSensitivity(name)
{
  // 初级声子 ROOT 文件
  primaryFile = new TFile("phonon_primary.root", "RECREATE");
  primaryTree = new TTree("primaryTree", "Primary Phonons");
  primaryTree->Branch("runID", &primRunID, "runID/I");
  primaryTree->Branch("eventID", &primEventID, "eventID/I");
  primaryTree->Branch("particleName", primParticleName, "particleName/C");
  primaryTree->Branch("energy", &primEnergy, "energy/D");
  primaryTree->Branch("x", &primX, "x/D");
  primaryTree->Branch("y", &primY, "y/D");
  primaryTree->Branch("z", &primZ, "z/D");
  primaryTree->Branch("t", &primT, "t/D");
  primaryTree->SetAutoFlush(-2000000);   // 内存优化

  // 有效电极击中 ROOT 文件
  hitsFile = new TFile("phonon_hits.root", "RECREATE");
  hitsTree = new TTree("hitsTree", "Active Electrode Hits");
  hitsTree->Branch("runID", &hitRunID, "runID/I");
  hitsTree->Branch("eventID", &hitEventID, "eventID/I");
  hitsTree->Branch("trackID", &hitTrackID, "trackID/I");
  hitsTree->Branch("particleName", hitParticleName, "particleName/C");
  hitsTree->Branch("startEnergy", &hitStartEnergy, "startEnergy/D");
  hitsTree->Branch("startX", &hitStartX, "startX/D");
  hitsTree->Branch("startY", &hitStartY, "startY/D");
  hitsTree->Branch("startZ", &hitStartZ, "startZ/D");
  hitsTree->Branch("startTime", &hitStartTime, "startTime/D");
  hitsTree->Branch("eDep", &hitEDep, "eDep/D");
  hitsTree->Branch("weight", &hitWeight, "weight/D");
  hitsTree->Branch("endX", &hitEndX, "endX/D");
  hitsTree->Branch("endY", &hitEndY, "endY/D");
  hitsTree->Branch("endZ", &hitEndZ, "endZ/D");
  hitsTree->Branch("endTime", &hitEndTime, "endTime/D");
  hitsTree->SetAutoFlush(-2000000);      // 内存优化
}

RISQTutorialSensitivity::~RISQTutorialSensitivity() {
  if (primaryFile) { primaryFile->Write(); primaryFile->Close(); delete primaryFile; }
  if (hitsFile)    { hitsFile->Write();    hitsFile->Close();    delete hitsFile; }
}

void RISQTutorialSensitivity::EndOfEvent(G4HCofThisEvent* HCE) {
  G4RunManager* runMan = G4RunManager::GetRunManager();
  const G4Event* currentEvent = runMan->GetCurrentEvent();

  // ========== 写入初级声子信息 ==========
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

  // ========== 写入有效电极击中（坐标过滤） ==========
  G4int HCID = G4SDManager::GetSDMpointer()->GetCollectionID(hitsCollection);
  auto* hitCol = static_cast<G4CMPElectrodeHitsCollection*>(HCE->GetHC(HCID));
  std::vector<G4CMPElectrodeHit*>* hitVec = hitCol->GetVector();

  for (G4CMPElectrodeHit* hit : *hitVec) {
    double x = hit->GetFinalPosition().x();   // mm
    double y = hit->GetFinalPosition().y();

    bool isLeft  = (x >= xLeftMin  && x <= xLeftMax  && y >= yMin && y <= yMax);
    bool isRight = (x >= xRightMin && x <= xRightMax && y >= yMin && y <= yMax);
    if (!isLeft && !isRight) continue;   // 只保留有效电极

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
    hitEndZ = hit->GetFinalPosition().getZ();
    hitEndTime = hit->GetFinalTime() / CLHEP::ns;

    hitsTree->Fill();
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