/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file exoticphysics/phonon/src/RISQTutorialPrimaryGeneratorAction.cc
/// \brief Implementation of the RISQTutorialPrimaryGeneratorAction class
//
// $Id: e75f788b103aef810361fad30f75077829192c13 $
//
// 20140519  Allow the user to specify phonon type by name in macro; if
//	     "geantino" is set, use random generator to select.

#include "RISQTutorialPrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4Geantino.hh"
#include "G4GeneralParticleSource.hh"
#include "G4RandomDirection.hh"
#include "G4PhononTransFast.hh"
#include "G4PhononTransSlow.hh"
#include "G4PhononLong.hh"
#include "G4SystemOfUnits.hh"

using namespace std;

RISQTutorialPrimaryGeneratorAction::RISQTutorialPrimaryGeneratorAction() { 
  fGPS = new G4GeneralParticleSource();

  // 以下默认设置已被注释，粒子属性完全由宏命令控制（/gps/...）
  // fGPS->SetParticleDefinition(G4Geantino::Definition());
  // fGPS->SetParticleMomentumDirection(G4RandomDirection());
  // fGPS->SetParticlePosition(G4ThreeVector(0.0,0.0,0.4998095*CLHEP::cm));
  // fGPS->SetParticleEnergy(0.0075*eV);  
}

RISQTutorialPrimaryGeneratorAction::~RISQTutorialPrimaryGeneratorAction() {
  delete fGPS;
}

void RISQTutorialPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent) {
  // 如果将来想支持 geantino 触发随机极化选择，可取消下面注释
  /*
  if (fGPS->GetParticleDefinition() == G4Geantino::Definition()) {
    G4double selector = G4UniformRand();
    if (selector<0.53539) {
      fGPS->SetParticleDefinition(G4PhononTransSlow::Definition()); 
    } else if (selector<0.90217) {
      fGPS->SetParticleDefinition(G4PhononTransFast::Definition());
    } else {
      fGPS->SetParticleDefinition(G4PhononLong::Definition());
    }
  }
  */

  // 直接使用 GPS 生成初级顶点，所有属性由宏命令设定
  fGPS->GeneratePrimaryVertex(anEvent);
}