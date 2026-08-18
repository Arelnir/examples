/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#ifndef RISQTutorialSensitivity_h
#define RISQTutorialSensitivity_h 1

#include "G4CMPElectrodeSensitivity.hh"
#include <string>

class TFile;
class TTree;

class RISQTutorialSensitivity : public G4CMPElectrodeSensitivity {
public:
  RISQTutorialSensitivity(G4String name);
  virtual ~RISQTutorialSensitivity();

protected:
  virtual G4bool IsHit(const G4Step*, const G4TouchableHistory*) const;
  virtual void EndOfEvent(G4HCofThisEvent*);

private:
  // 初级声子 ROOT 输出
  TFile* primaryFile;
  TTree* primaryTree;
  int primRunID, primEventID;
  double primEnergy, primX, primY, primZ, primT;
  char primParticleName[20];

  // 有效电极击中 ROOT 输出
  TFile* hitsFile;
  TTree* hitsTree;
  int hitRunID, hitEventID, hitTrackID;
  char hitParticleName[20];
  double hitStartEnergy, hitStartX, hitStartY, hitStartZ, hitStartTime;
  double hitEDep, hitWeight;
  double hitEndX, hitEndY, hitEndZ, hitEndTime;

  // 坐标过滤（有效电极区域，单位：mm）
  static constexpr double xLeftMin  = -3.775, xLeftMax  = -3.325;
  static constexpr double xRightMin =  3.325, xRightMax =  3.775;
  static constexpr double yMin = -0.52, yMax = 0.077665;
};

#endif