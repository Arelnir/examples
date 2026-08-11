/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#ifndef RISQTutorialSensitivity_h
#define RISQTutorialSensitivity_h 1

#include "G4CMPElectrodeSensitivity.hh"
#include <fstream>
#include <mutex>

class RISQTutorialSensitivity : public G4CMPElectrodeSensitivity {
public:
  RISQTutorialSensitivity(G4String name);
  virtual ~RISQTutorialSensitivity();

  void SetHitOutputFile(const G4String& fn);
  void SetPrimaryOutputFile(const G4String& fn);

  // 静态接口：低能声子记录（供 SteppingAction 调用）
  static void RecordLowEnergyPhonon(G4int runID, G4int eventID,
                                    G4int trackID, G4double energy);
  static void EnableLowERecord(bool enable = true);

protected:
  virtual G4bool IsHit(const G4Step*, const G4TouchableHistory*) const;
  virtual void EndOfEvent(G4HCofThisEvent*);

private:
  std::ofstream hitOutput;
  G4String hitFileName;

  std::ofstream primaryOutput;
  G4String primaryFileName;

  // 低能声子静态输出（线程安全）
  static std::ofstream lowEOutput;
  static std::mutex lowEMutex;
  static bool lowEEnabled;
  static bool lowEFileInitialized;
};

#endif
