/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file exoticphysics/phonon/include/RISQTutorialDetectorConstruction.hh
/// \brief Definition of the RISQTutorialDetectorConstruction class

#ifndef RISQTutorialDetectorConstruction_h
#define RISQTutorialDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"

class G4Material;
class G4VPhysicalVolume;
class G4CMPSurfaceProperty;
class RISQTutorialSensitivity;

class RISQTutorialDetectorConstruction : public G4VUserDetectorConstruction {
public:
  RISQTutorialDetectorConstruction();
  virtual ~RISQTutorialDetectorConstruction();

  virtual G4VPhysicalVolume* Construct();

private:
  void DefineMaterials();
  void SetupGeometry();
  void AttachPhononSensor(G4CMPSurfaceProperty* surfProp);

  G4Material* fLiquidHelium;
  G4Material* fSilicon;
  G4Material* fAluminum;

  G4VPhysicalVolume* fWorldPhys;
  G4CMPSurfaceProperty* fElectrodeSurfProp;
  G4CMPSurfaceProperty* wallSurfProp;

  RISQTutorialSensitivity* fSuperconductorSensitivity;

  G4bool fConstructed;
};

#endif