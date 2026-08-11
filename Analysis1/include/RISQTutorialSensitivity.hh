/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#ifndef RISQTutorialSensitivity_h
#define RISQTutorialSensitivity_h 1

#include "G4CMPElectrodeSensitivity.hh"
#include <fstream>

class RISQTutorialSensitivity : public G4CMPElectrodeSensitivity {
public:
  RISQTutorialSensitivity(G4String name);
  virtual ~RISQTutorialSensitivity();

  void SetHitOutputFile(const G4String& fn);
  void SetPrimaryOutputFile(const G4String& fn);

protected:
  virtual G4bool IsHit(const G4Step*, const G4TouchableHistory*) const;
  virtual void EndOfEvent(G4HCofThisEvent*);

private:
  std::ofstream hitOutput;
  G4String hitFileName;

  std::ofstream primaryOutput;
  G4String primaryFileName;
};

#endif
