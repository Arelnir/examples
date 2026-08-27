/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#include "RISQTutorialDetectorConstruction.hh"
#include "RISQTutorialSensitivity.hh"
#include "G4CMPLogicalBorderSurface.hh"
#include "G4CMPPhononElectrode.hh"
#include "G4CMPSurfaceProperty.hh"
#include "G4CMPUtils.hh"
#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4GeometryManager.hh"
#include "G4LatticeLogical.hh"
#include "G4LatticeManager.hh"
#include "G4LatticePhysical.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4SolidStore.hh"
#include "G4SystemOfUnits.hh"
#include "G4TransportationManager.hh"
#include "G4Tubs.hh"
#include "G4ExtrudedSolid.hh"
#include "G4UserLimits.hh"
#include "G4VisAttributes.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

RISQTutorialDetectorConstruction::RISQTutorialDetectorConstruction()
  : fLiquidHelium(0), fSilicon(0), fAluminum(0),
    fWorldPhys(0), fElectrodeSurfProp(0), wallSurfProp(0),
    fSuperconductorSensitivity(0), fConstructed(false) {;}

RISQTutorialDetectorConstruction::~RISQTutorialDetectorConstruction() {
  delete fElectrodeSurfProp;
  delete wallSurfProp;
}

G4VPhysicalVolume* RISQTutorialDetectorConstruction::Construct() {
  if (fConstructed) {
    if (!G4RunManager::IfGeometryHasBeenDestroyed()) {
      G4GeometryManager::GetInstance()->OpenGeometry();
      G4PhysicalVolumeStore::GetInstance()->Clean();
      G4LogicalVolumeStore::GetInstance()->Clean();
      G4SolidStore::GetInstance()->Clean();
    }
    G4LatticeManager::GetLatticeManager()->Reset();
    G4CMPLogicalBorderSurface::CleanSurfaceTable();
  }
  DefineMaterials();
  SetupGeometry();
  fConstructed = true;
  return fWorldPhys;
}

void RISQTutorialDetectorConstruction::DefineMaterials() {
  G4NistManager* nistManager = G4NistManager::Instance();
  fLiquidHelium = nistManager->FindOrBuildMaterial("G4_AIR");
  fSilicon      = nistManager->FindOrBuildMaterial("G4_Si");
  fAluminum     = nistManager->FindOrBuildMaterial("G4_Al");
}

void RISQTutorialDetectorConstruction::SetupGeometry() {
  // ---------- 世界体积 ----------
  G4double worldXY = 2.5 * cm;
  G4double worldZ  = 1.0 * cm;
  G4VSolid* worldSolid = new G4Box("World", worldXY/2., worldXY/2., worldZ/2.);
  G4LogicalVolume* worldLV = new G4LogicalVolume(worldSolid, fLiquidHelium, "World");
  worldLV->SetUserLimits(new G4UserLimits(10*mm, DBL_MAX, DBL_MAX, 0, 0));
  fWorldPhys = new G4PVPlacement(0, G4ThreeVector(), worldLV, "World", 0, false, 0);

  // ---------- 硅晶体 ----------
  G4double siX = 10000. * um;
  G4double siY = 3333.334 * um;
  G4double siZ = 675.0 * um;
  G4VSolid* siSolid = new G4Box("SiCrystal", siX/2., siY/2., siZ/2.);
  G4LogicalVolume* siLV = new G4LogicalVolume(siSolid, fSilicon, "SiCrystal");
  G4VPhysicalVolume* siPhys = new G4PVPlacement(0, G4ThreeVector(0,0,0),
                                                siLV, "SiCrystal", worldLV, false, 0);

  // ---------- 晶格 ----------
  G4LatticeManager* LM = G4LatticeManager::GetLatticeManager();
  G4LatticeLogical* SiLog = LM->LoadLattice(fSilicon, "Si");
  G4LatticePhysical* SiPhys = new G4LatticePhysical(SiLog);
  SiPhys->SetMillerOrientation(1,0,0);
  LM->RegisterLattice(siPhys, SiPhys);

  // ---------- 可视化 ----------
  worldLV->SetVisAttributes(G4VisAttributes::GetInvisible());
  G4VisAttributes* siVisAtt = new G4VisAttributes(G4Colour(0.50, 0.80, 0.60));
  siVisAtt->SetVisibility(true);
  siLV->SetVisAttributes(siVisAtt);

  // ---------- 表面属性 ----------
  if (!fConstructed) {
    wallSurfProp = new G4CMPSurfaceProperty("SiWall",
        0.0, 1.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 1.0);
    new G4CMPLogicalBorderSurface("SiSide", siPhys, fWorldPhys, wallSurfProp);

    fElectrodeSurfProp = new G4CMPSurfaceProperty("ElectrodeSurf",
        1.0, 0.0, 0.0, 0.0,
        0.94495, 0.05505, 0.0, 0.0,
        1.0, 0.0);

    const G4double GHz = 1e9 * hertz;
    const std::vector<G4double> anhCoeffs  = {};
    const std::vector<G4double> diffCoeffs = {};
    const std::vector<G4double> specCoeffs = {};
    const G4double anhCutoff  = 0.;
    const G4double reflCutoff = 0.;
    fElectrodeSurfProp->AddScatteringProperties(anhCutoff, reflCutoff,
                                                anhCoeffs, diffCoeffs, specCoeffs,
                                                GHz, GHz, GHz);

    AttachPhononSensor(fElectrodeSurfProp);
  }

  G4double elecThick = 0.1 * um;
  G4double siHalfZ = siZ / 2.;

  // ================= CPW =================
  G4double cpwHalfX = 8380./2. * um;
  G4double cpwHalfY = 36./2. * um;
  G4double cpwHalfZ = elecThick / 2.;
  G4VSolid* cpwSolid = new G4Box("CPW", cpwHalfX, cpwHalfY, cpwHalfZ);
  G4LogicalVolume* cpwLV = new G4LogicalVolume(cpwSolid, fAluminum, "CPW_LV");

  G4double cpwCenterY = 465. * um;
  G4double cpwZ = siHalfZ + cpwHalfZ;
  G4VPhysicalVolume* cpwPhys = new G4PVPlacement(0,
      G4ThreeVector(0., cpwCenterY, cpwZ), cpwLV, "CPW", worldLV, false, 0);
  new G4CMPLogicalBorderSurface("Si_CPW", siPhys, cpwPhys, fElectrodeSurfProp);
  new G4CMPLogicalBorderSurface("CPW_Si", cpwPhys, siPhys, fElectrodeSurfProp);
  G4VisAttributes* copperVisAtt = new G4VisAttributes(G4Colour(1.0, 0.6, 0.2));
  cpwLV->SetVisAttributes(copperVisAtt);

// ================= CPW 端口（矩形 + 两个细梯形） =================
G4double portHalfZ = elecThick / 2.;
G4double portZcenter = siHalfZ + portHalfZ;

// 矩形尺寸
G4double rectHalfX = 200./2. * um;   // 宽 200
G4double rectHalfY = 1080./2. * um;  // 长 1200
G4VSolid* rectSolid = new G4Box("CPW_PortRect", rectHalfX, rectHalfY, portHalfZ);
G4LogicalVolume* rectLeftLV = new G4LogicalVolume(rectSolid, fAluminum, "CPW_PortRectLeftLV");
G4LogicalVolume* rectRightLV = new G4LogicalVolume(rectSolid, fAluminum, "CPW_PortRectRightLV");

// 矩形中心 X = ±4650 um，Y=465 um
G4double rectCenterX = cpwHalfX + 360.*um + rectHalfX;   // 4650 um
G4double rectYcenter = cpwCenterY;

// 放置左矩形
G4VPhysicalVolume* pvPort;
pvPort = new G4PVPlacement(0, G4ThreeVector(-rectCenterX, rectYcenter, portZcenter),
                           rectLeftLV, "CPW_PortRectLeft", worldLV, false, 602);
new G4CMPLogicalBorderSurface("Si_RectLeft", siPhys, pvPort, fElectrodeSurfProp);
new G4CMPLogicalBorderSurface("RectLeft_Si", pvPort, siPhys, fElectrodeSurfProp);

// 放置右矩形
pvPort = new G4PVPlacement(0, G4ThreeVector(rectCenterX, rectYcenter, portZcenter),
                           rectRightLV, "CPW_PortRectRight", worldLV, false, 603);
new G4CMPLogicalBorderSurface("Si_RectRight", siPhys, pvPort, fElectrodeSurfProp);
new G4CMPLogicalBorderSurface("RectRight_Si", pvPort, siPhys, fElectrodeSurfProp);

// ---------- 右上梯形（四个顶点，单位 um） ----------
std::vector<G4TwoVector> upperTrap = {
    G4TwoVector(4190.*um, 465.*um),
    G4TwoVector(4190.*um, 483.*um),
    G4TwoVector(4550.*um, 1005.*um),
    G4TwoVector(4550.*um, 905.0375*um)
};

// ---------- 右下梯形（关于 Y=465 对称） ----------
std::vector<G4TwoVector> lowerTrap = {
    G4TwoVector(4190.*um, 465.*um),
    G4TwoVector(4190.*um, 447.*um),
    G4TwoVector(4550.*um, -75.*um),
    G4TwoVector(4550.*um, 24.9625*um)
};

// 定义 Z 段
G4ExtrudedSolid::ZSection zsecArr[2] = {
    G4ExtrudedSolid::ZSection(-portHalfZ, G4TwoVector(0,0), 1.0),
    G4ExtrudedSolid::ZSection( portHalfZ, G4TwoVector(0,0), 1.0)
};
std::vector<G4ExtrudedSolid::ZSection> zsec(zsecArr, zsecArr+2);

// 右侧两个梯形实体
G4VSolid* trapRightUpperSolid = new G4ExtrudedSolid("TrapRightUpper", upperTrap, zsec);
G4VSolid* trapRightLowerSolid = new G4ExtrudedSolid("TrapRightLower", lowerTrap, zsec);

// 左侧梯形：X 取负，Y 不变
std::vector<G4TwoVector> upperTrapLeft, lowerTrapLeft;
for (auto& p : upperTrap) upperTrapLeft.emplace_back(-p.x(), p.y());
for (auto& p : lowerTrap) lowerTrapLeft.emplace_back(-p.x(), p.y());

G4VSolid* trapLeftUpperSolid = new G4ExtrudedSolid("TrapLeftUpper", upperTrapLeft, zsec);
G4VSolid* trapLeftLowerSolid = new G4ExtrudedSolid("TrapLeftLower", lowerTrapLeft, zsec);

// 逻辑体
G4LogicalVolume* trapRightUpperLV = new G4LogicalVolume(trapRightUpperSolid, fAluminum, "TrapRightUpperLV");
G4LogicalVolume* trapRightLowerLV = new G4LogicalVolume(trapRightLowerSolid, fAluminum, "TrapRightLowerLV");
G4LogicalVolume* trapLeftUpperLV  = new G4LogicalVolume(trapLeftUpperSolid, fAluminum, "TrapLeftUpperLV");
G4LogicalVolume* trapLeftLowerLV  = new G4LogicalVolume(trapLeftLowerSolid, fAluminum, "TrapLeftLowerLV");

// 放置右侧梯形
pvPort = new G4PVPlacement(0, G4ThreeVector(0,0,portZcenter), trapRightUpperLV,
                           "CPW_PortTrapRightUpper", worldLV, false, 604);
new G4CMPLogicalBorderSurface("Si_TrapRightUpper", siPhys, pvPort, fElectrodeSurfProp);
new G4CMPLogicalBorderSurface("TrapRightUpper_Si", pvPort, siPhys, fElectrodeSurfProp);

pvPort = new G4PVPlacement(0, G4ThreeVector(0,0,portZcenter), trapRightLowerLV,
                           "CPW_PortTrapRightLower", worldLV, false, 605);
new G4CMPLogicalBorderSurface("Si_TrapRightLower", siPhys, pvPort, fElectrodeSurfProp);
new G4CMPLogicalBorderSurface("TrapRightLower_Si", pvPort, siPhys, fElectrodeSurfProp);

// 放置左侧梯形
pvPort = new G4PVPlacement(0, G4ThreeVector(0,0,portZcenter), trapLeftUpperLV,
                           "CPW_PortTrapLeftUpper", worldLV, false, 606);
new G4CMPLogicalBorderSurface("Si_TrapLeftUpper", siPhys, pvPort, fElectrodeSurfProp);
new G4CMPLogicalBorderSurface("TrapLeftUpper_Si", pvPort, siPhys, fElectrodeSurfProp);

pvPort = new G4PVPlacement(0, G4ThreeVector(0,0,portZcenter), trapLeftLowerLV,
                           "CPW_PortTrapLeftLower", worldLV, false, 607);
new G4CMPLogicalBorderSurface("Si_TrapLeftLower", siPhys, pvPort, fElectrodeSurfProp);
new G4CMPLogicalBorderSurface("TrapLeftLower_Si", pvPort, siPhys, fElectrodeSurfProp);

// 颜色设置
trapRightUpperLV->SetVisAttributes(copperVisAtt);
trapRightLowerLV->SetVisAttributes(copperVisAtt);
trapLeftUpperLV->SetVisAttributes(copperVisAtt);
trapLeftLowerLV->SetVisAttributes(copperVisAtt);
rectLeftLV->SetVisAttributes(copperVisAtt);
rectRightLV->SetVisAttributes(copperVisAtt);

  // ================= IDC =================
  G4double idcMetalHalfX = 60.16/2. * um;
  G4double idcMetalHalfY = 366./2. * um;
  G4double idcHalfZ = elecThick / 2.;
  G4VSolid* idcMetalSolid = new G4Box("IDCmetal", idcMetalHalfX, idcMetalHalfY, idcHalfZ);
  G4LogicalVolume* idcMetalLV = new G4LogicalVolume(idcMetalSolid, fAluminum, "IDCmetal_LV");

  G4double idcYcenter = 256. * um;
  G4double idcZcenter = siHalfZ + idcHalfZ;

  G4double idcLocalX[3] = {-164.92*um, 0., 164.92*um};
  G4double idcCenterX[2] = {-3000. * um, 3000. * um};

  G4VPhysicalVolume* pv;
  for (int side = 0; side < 2; ++side) {
      for (int i = 0; i < 3; ++i) {
          G4double x = idcCenterX[side] + idcLocalX[i];
          G4String name = G4String("IDC_") + (side==0?"L":"R") + G4String(std::to_string(i));
          pv = new G4PVPlacement(0, G4ThreeVector(x, idcYcenter, idcZcenter),
                                 idcMetalLV, name, worldLV, false, 10+side*3+i);
          new G4CMPLogicalBorderSurface("Si_"+name, siPhys, pv, fElectrodeSurfProp);
          new G4CMPLogicalBorderSurface(name+"_Si", pv, siPhys, fElectrodeSurfProp);
      }
  }
  idcMetalLV->SetVisAttributes(copperVisAtt);

  // ================= 灵敏探测器 =================
  G4SDManager* SDman = G4SDManager::GetSDMpointer();
  if (!fSuperconductorSensitivity) {
    fSuperconductorSensitivity = new RISQTutorialSensitivity("PhononElectrode");
    SDman->AddNewDetector(fSuperconductorSensitivity);
    siLV->SetSensitiveDetector(fSuperconductorSensitivity);
  }

  // ================= Resonator =================
  G4double resHalfZ = elecThick / 2.;
  G4double resZcenter = siHalfZ + resHalfZ;
  G4double resMetalHalfX = 16.0375/2. * um;
  G4double resMetalHalfY = 1200./2. * um;
  G4VSolid* resMetalSolid = new G4Box("ResMetal", resMetalHalfX, resMetalHalfY, resHalfZ);
  G4LogicalVolume* resMetalLV = new G4LogicalVolume(resMetalSolid, fAluminum, "ResMetal_LV");
  resMetalLV->SetSensitiveDetector(fSuperconductorSensitivity);

  G4double resYcenter = (idcYcenter - 183.*um) - 600.*um;  // 73 - 600 = -527 um
  G4double resCenterX[2] = {-3000. * um, 3000. * um};
  G4double resLocalX[2] = {-30.48125*um, 30.48125*um};

  for (int side = 0; side < 2; ++side) {
      for (int i = 0; i < 2; ++i) {
          G4double x = resCenterX[side] + resLocalX[i];
          G4String name = G4String("Res_") + (side==0?"L":"R") + G4String(std::to_string(i));
          pv = new G4PVPlacement(0, G4ThreeVector(x, resYcenter, resZcenter),
                                 resMetalLV, name, worldLV, false, 100+side*2+i);
          new G4CMPLogicalBorderSurface("Si_"+name, siPhys, pv, fElectrodeSurfProp);
          new G4CMPLogicalBorderSurface(name+"_Si", pv, siPhys, fElectrodeSurfProp);
      }
  }
  G4VisAttributes* cyanVisAtt = new G4VisAttributes(G4Colour(0.0, 1.0, 1.0));
  resMetalLV->SetVisAttributes(cyanVisAtt);

  // ================= 边缘固定电极 =================
  G4double edgePadHalfX = 400./2. * um, edgePadHalfY = 100./2. * um;
  G4double edgePadHalfZ = elecThick / 2.;
  G4VSolid* edgePadSolid = new G4Box("EdgePad", edgePadHalfX, edgePadHalfY, edgePadHalfZ);
  G4LogicalVolume* edgePadLV = new G4LogicalVolume(edgePadSolid, fAluminum, "EdgePad_LV");

  G4double edgeX[2] = {-2050. * um, 2050. * um};
  G4double edgeY[2] = {-1375. * um, 1375. * um};
  G4double edgeZ = siHalfZ + edgePadHalfZ;

  int edgeCopy = 700;
  for (int iy = 0; iy < 2; ++iy) {
      for (int ix = 0; ix < 2; ++ix) {
          G4String name = G4String("EdgePad_") + (iy==0?"Bot":"Top") + "_" + (ix==0?"L":"R");
          pv = new G4PVPlacement(0, G4ThreeVector(edgeX[ix], edgeY[iy], edgeZ),
                                 edgePadLV, name, worldLV, false, edgeCopy++);
          new G4CMPLogicalBorderSurface("Si_"+name, siPhys, pv, fElectrodeSurfProp);
          new G4CMPLogicalBorderSurface(name+"_Si", pv, siPhys, fElectrodeSurfProp);
      }
  }
  G4VisAttributes* purpleVisAtt = new G4VisAttributes(G4Colour(0.7, 0.4, 0.9));
  edgePadLV->SetVisAttributes(purpleVisAtt);
}

void RISQTutorialDetectorConstruction::AttachPhononSensor(G4CMPSurfaceProperty *surfProp) {
  if (!surfProp) return;
  auto sensorProp = surfProp->GetPhononMaterialPropertiesTablePointer();
  G4CMP::UpdateMPT(sensorProp, "filmAbsorption", 1.0);
  G4CMP::UpdateMPT(sensorProp, "filmThickness", 100.*nm);
  G4CMP::UpdateMPT(sensorProp, "gapEnergy", 173.715e-6*eV);
  G4CMP::UpdateMPT(sensorProp, "lowQPLimit", 3.);
  G4CMP::UpdateMPT(sensorProp, "phononLifetime", 242.*ps);
  G4CMP::UpdateMPT(sensorProp, "phononLifetimeSlope", 0.29);
  G4CMP::UpdateMPT(sensorProp, "vSound", 3.26*km/s);
  G4CMP::UpdateMPT(sensorProp, "subgapAbsorption", 0.0);
  surfProp->SetPhononElectrode(new G4CMPPhononElectrode);
}