// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Geant4/Geant4Converters.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "G4Transform3D.hh"
#include "G4Trap.hh"
#include "G4Trd.hh"
#include "G4Tubs.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VSolid.hh"

Acts::Transform3 Acts::Geant4AlgebraConverter::transform(
    const G4ThreeVector& g4Trans) {
  Transform3 gTransform = Transform3::Identity();
  Vector3 scaledTrans =
      Vector3(scale * g4Trans[0], scale * g4Trans[1], scale * g4Trans[2]);
  gTransform.pretranslate(scaledTrans);
  return gTransform;
}

Acts::Transform3 Acts::Geant4AlgebraConverter::transform(
    const G4RotationMatrix& g4Rot, const G4ThreeVector& g4Trans) {
  // Create the translation
  Vector3 translation(scale * g4Trans[0], scale * g4Trans[1],
                      scale * g4Trans[2]);
  // And the rotation to it
  RotationMatrix3 rotation;
  rotation << g4Rot.xx(), g4Rot.xy(), g4Rot.xz(), g4Rot.yx(), g4Rot.yy(),
      g4Rot.yz(), g4Rot.zx(), g4Rot.zy(), g4Rot.zz();
  Transform3 transform = Transform3::Identity();
  transform.rotate(rotation);
  transform.pretranslate(translation);
  return transform;
}

Acts::Transform3 Acts::Geant4AlgebraConverter::transform(
    const G4Transform3D& g4Trf) {
  auto g4Rot = g4Trf.getRotation();
  auto g4Trans = g4Trf.getTranslation();
  return transform(g4Rot, g4Trans);
}

Acts::Transform3 Acts::Geant4AlgebraConverter::transform(
    const G4VPhysicalVolume& g4PhysVol) {
  // Get Rotation and translation
  auto g4Translation = g4PhysVol.GetTranslation();
  auto g4Rotation = g4PhysVol.GetRotation();

  G4Transform3D g4Transform =
      (g4Rotation == nullptr)
          ? G4Transform3D(CLHEP::HepRotation(), g4Translation)
          : G4Transform3D(*g4Rotation, g4Translation);

  return transform(g4Transform);
}

std::tuple<std::shared_ptr<Acts::CylinderBounds>, double>
Acts::Geant4ShapeConverter::cylinderBounds(const G4Tubs& g4Tubs) {
  using B = Acts::CylinderBounds;

  std::array<double, B::eSize> tArray = {};
  tArray[B::eR] =
      static_cast<double>(g4Tubs.GetInnerRadius() + g4Tubs.GetOuterRadius()) *
      0.5;
  tArray[B::eHalfLengthZ] = static_cast<double>(g4Tubs.GetZHalfLength());
  tArray[B::eHalfPhiSector] =
      0.5 * static_cast<double>(g4Tubs.GetDeltaPhiAngle());
  // Geant fiddles around with user given values, i.e. it would not allow [-PI,
  // +PI) as a full segment (has to be [0, 2PI)])
  if (std::abs(tArray[B::eHalfPhiSector] - std::numbers::pi) <
      std::numeric_limits<double>::epsilon()) {
    tArray[B::eAveragePhi] = 0.;
  } else {
    tArray[B::eAveragePhi] = static_cast<double>(g4Tubs.GetStartPhiAngle()) +
                             tArray[B::eHalfPhiSector];
  }
  double thickness = g4Tubs.GetOuterRadius() - g4Tubs.GetInnerRadius();
  auto cBounds = std::make_shared<CylinderBounds>(tArray);
  return {std::move(cBounds), thickness};
}

std::tuple<std::shared_ptr<Acts::RadialBounds>, double>
Acts::Geant4ShapeConverter::radialBounds(const G4Tubs& g4Tubs) {
  using B = Acts::RadialBounds;

  std::array<double, B::eSize> tArray = {};
  tArray[B::eMinR] = static_cast<double>(g4Tubs.GetInnerRadius());
  tArray[B::eMaxR] = static_cast<double>(g4Tubs.GetOuterRadius());
  tArray[B::eHalfPhiSector] =
      0.5 * static_cast<double>(g4Tubs.GetDeltaPhiAngle());
  // Geant fiddles around with user given values, i.e. it would not allow [-PI,
  // +PI) as a full segment (has to be [0, 2PI)])
  if (std::abs(tArray[B::eHalfPhiSector] - std::numbers::pi) <
      std::numeric_limits<double>::epsilon()) {
    tArray[B::eAveragePhi] = 0.;
  } else {
    tArray[B::eAveragePhi] = static_cast<double>(g4Tubs.GetStartPhiAngle()) +
                             tArray[B::eHalfPhiSector];
  }
  double thickness = g4Tubs.GetZHalfLength() * 2;
  auto rBounds = std::make_shared<RadialBounds>(tArray);
  return {std::move(rBounds), thickness};
}

std::shared_ptr<Acts::LineBounds> Acts::Geant4ShapeConverter::lineBounds(
    const G4Tubs& g4Tubs) {
  auto r = static_cast<double>(g4Tubs.GetOuterRadius());
  auto hlZ = static_cast<double>(g4Tubs.GetZHalfLength());
  return std::make_shared<LineBounds>(r, hlZ);
}

std::tuple<std::shared_ptr<Acts::RectangleBounds>, std::array<int, 2u>, double>
Acts::Geant4ShapeConverter::rectangleBounds(const G4Box& g4Box) {
  std::vector<double> hG4XYZ = {static_cast<double>(g4Box.GetXHalfLength()),
                                static_cast<double>(g4Box.GetYHalfLength()),
                                static_cast<double>(g4Box.GetZHalfLength())};

  auto minAt = std::min_element(hG4XYZ.begin(), hG4XYZ.end());
  std::size_t minPos = std::distance(hG4XYZ.begin(), minAt);
  double thickness = 2. * hG4XYZ[minPos];

  std::array<int, 2u> rAxes = {};
  switch (minPos) {
    case 0: {
      rAxes = {1, 2};
    } break;
    case 1: {
      if (keepAxisOrder) {
        rAxes = {0, -2};  // flip for right-handed
      } else {
        rAxes = {2, 0};  // cyclic positive
      }
    } break;
    case 2: {
      rAxes = {0, 1};
    } break;
    default:  // do nothing
      break;
  }
  auto rBounds = std::make_shared<RectangleBounds>(hG4XYZ[std::abs(rAxes[0u])],
                                                   hG4XYZ[std::abs(rAxes[1u])]);
  return {std::move(rBounds), rAxes, thickness};
}

std::tuple<std::shared_ptr<Acts::TrapezoidBounds>, std::array<int, 2u>, double>
Acts::Geant4ShapeConverter::trapezoidBounds(const G4Trd& g4Trd) {
  // primary parameters
  double hlX0 = static_cast<double>(g4Trd.GetXHalfLength1());
  double hlX1 = static_cast<double>(g4Trd.GetXHalfLength2());
  double hlY0 = static_cast<double>(g4Trd.GetYHalfLength1());
  double hlY1 = static_cast<double>(g4Trd.GetYHalfLength2());
  double hlZ = static_cast<double>(g4Trd.GetZHalfLength());

  std::vector<double> dXYZ = {(hlX0 + hlX1) * 0.5, (hlY0 + hlY1) * 0.5, hlZ};

  auto minAt = std::min_element(dXYZ.begin(), dXYZ.end());
  std::size_t minPos = std::distance(dXYZ.begin(), minAt);
  double thickness = 2. * dXYZ[minPos];

  double halfLengthXminY = 0.;
  double halfLengthXmaxY = 0.;
  double halfLengthY = 0.;

  std::array<int, 2u> rAxes = {};
  switch (minPos) {
    case 0: {
      halfLengthXminY = hlY0;
      halfLengthXmaxY = hlY1;
      halfLengthY = hlZ;
      rAxes = {1, 2};
    } break;
    case 1: {
      halfLengthXminY = hlX0;
      halfLengthXmaxY = hlX1;
      halfLengthY = hlZ;
      rAxes = {0, -2};
    } break;
    case 2: {
      if (std::abs(hlY0 - hlY1) < std::abs(hlX0 - hlX1)) {
        halfLengthXminY = hlX0;
        halfLengthXmaxY = hlX1;
        halfLengthY = (hlY0 + hlY1) * 0.5;
        rAxes = {0, 1};
      } else {
        halfLengthXminY = hlY0;
        halfLengthXmaxY = hlY1;
        halfLengthY = (hlX0 + hlX1) * 0.5;
        rAxes = {-1, 0};
      }
    } break;
  }

  auto tBounds = std::make_shared<TrapezoidBounds>(
      halfLengthXminY, halfLengthXmaxY, halfLengthY);
  return {std::move(tBounds), rAxes, thickness};
}

std::tuple<std::shared_ptr<Acts::TrapezoidBounds>, std::array<int, 2u>, double>
Acts::Geant4ShapeConverter::trapezoidBounds(const G4Trap& g4Trap) {
  // primary parameters
  auto y1 = static_cast<double>(g4Trap.GetYHalfLength1());
  auto y2 = static_cast<double>(g4Trap.GetYHalfLength2());
  auto x1 = static_cast<double>(g4Trap.GetXHalfLength1());
  auto x2 = static_cast<double>(g4Trap.GetXHalfLength2());
  auto x3 = static_cast<double>(g4Trap.GetXHalfLength3());
  auto x4 = static_cast<double>(g4Trap.GetXHalfLength4());
  auto phi = static_cast<double>(g4Trap.GetPhi());
  auto theta = static_cast<double>(g4Trap.GetTheta());
  auto z = static_cast<double>(g4Trap.GetZHalfLength());

  double hlX0 = (x1 + x2) * 0.5;
  double hlX1 = 2 * z * tan(theta) * cos(phi) + (x3 + x4) * 0.5;
  double hlY0 = y1;
  double hlY1 = y2 + 2 * z * tan(theta) * sin(phi);
  double hlZ = z;

  std::vector<double> dXYZ = {(hlX0 + hlX1) * 0.5, (hlY0 + hlY1) * 0.5, hlZ};

  auto minAt = std::ranges::min_element(dXYZ);
  std::size_t minPos = std::distance(dXYZ.begin(), minAt);
  double thickness = 2. * dXYZ[minPos];

  double halfLengthXminY = 0.;
  double halfLengthXmaxY = 0.;
  double halfLengthY = 0.;

  std::array<int, 2u> rAxes = {};
  switch (minPos) {
    case 0: {
      halfLengthXminY = std::min(hlY0, hlY1);
      halfLengthXmaxY = std::max(hlY0, hlY1);
      halfLengthY = hlZ;
      rAxes = {1, 2};
    } break;
    case 1: {
      halfLengthXminY = std::min(hlX0, hlX1);
      halfLengthXmaxY = std::max(hlX0, hlX1);
      halfLengthY = hlZ;
      rAxes = {0, -2};
    } break;
    case 2: {
      if (std::abs(hlY0 - hlY1) < std::abs(hlX0 - hlX1)) {
        halfLengthXminY = std::min(hlX0, hlX1);
        halfLengthXmaxY = std::max(hlX0, hlX1);
        halfLengthY = (hlY0 + hlY1) * 0.5;
        rAxes = {0, 1};
      } else {
        halfLengthXminY = std::min(hlY0, hlY1);
        halfLengthXmaxY = std::max(hlY0, hlY1);
        halfLengthY = (hlX0 + hlX1) * 0.5;
        rAxes = {-1, 0};
      }
    } break;
    default: {
      throw std::runtime_error("Geant4Converters: could not convert G4Trap.");
    }
  }

  auto tBounds = std::make_shared<TrapezoidBounds>(
      halfLengthXminY, halfLengthXmaxY, halfLengthY);
  return std::make_tuple(std::move(tBounds), rAxes, thickness);
}

std::tuple<std::shared_ptr<Acts::PlanarBounds>, std::array<int, 2u>, double>
Acts::Geant4ShapeConverter::planarBounds(const G4VSolid& g4Solid) {
  const G4Box* box = dynamic_cast<const G4Box*>(&g4Solid);
  if (box != nullptr) {
    auto [rBounds, axes, thickness] = rectangleBounds(*box);
    return {std::move(rBounds), axes, thickness};
  }

  const G4Trd* trd = dynamic_cast<const G4Trd*>(&g4Solid);
  if (trd != nullptr) {
    auto [tBounds, axes, thickness] = trapezoidBounds(*trd);
    return {std::move(tBounds), axes, thickness};
  }

  std::shared_ptr<Acts::PlanarBounds> pBounds = nullptr;
  std::array<int, 2u> rAxes = {};
  double rThickness = 0.;
  return {std::move(pBounds), rAxes, rThickness};
}

namespace {
Acts::Transform3 axesOriented(const Acts::Transform3& toGlobalOriginal,
                              const std::array<int, 2u>& axes) {
  auto originalRotation = toGlobalOriginal.rotation();
  auto colX = originalRotation.col(std::abs(axes[0u]));
  auto colY = originalRotation.col(std::abs(axes[1u]));
  colX *= std::copysign(1, axes[0u]);
  colY *= std::copysign(1, axes[1u]);
  Acts::Vector3 colZ = colX.cross(colY);

  Acts::Transform3 orientedTransform = Acts::Transform3::Identity();
  orientedTransform.matrix().block(0, 0, 3, 1) = colX;
  orientedTransform.matrix().block(0, 1, 3, 1) = colY;
  orientedTransform.matrix().block(0, 2, 3, 1) = colZ;
  orientedTransform.matrix().block(0, 3, 3, 1) = toGlobalOriginal.translation();

  return orientedTransform;
}
}  // namespace

std::shared_ptr<Acts::Surface> Acts::Geant4PhysicalVolumeConverter::surface(
    const G4VPhysicalVolume& g4PhysVol, const Transform3& toGlobal,
    bool convertMaterial, double compressed) {
  // Get the logical volume
  auto g4LogVol = g4PhysVol.GetLogicalVolume();
  auto g4Solid = g4LogVol->GetSolid();

  auto assignMaterial = [&](Acts::Surface& sf, double moriginal,
                            double mcompressed) -> void {
    auto g4Material = g4LogVol->GetMaterial();
    if (convertMaterial && g4Material != nullptr) {
      if (compressed < 0.) {
        mcompressed = moriginal;
      }
      auto surfaceMaterial = Geant4MaterialConverter{}.surfaceMaterial(
          *g4Material, moriginal, mcompressed);
      sf.assignSurfaceMaterial(std::move(surfaceMaterial));
    }
  };

  // Dynamic cast chain & conversion
  std::shared_ptr<Surface> surface = nullptr;

  // Into a rectangle
  auto g4Box = dynamic_cast<const G4Box*>(g4Solid);
  if (g4Box != nullptr) {
    if (forcedType == Surface::SurfaceType::Other ||
        forcedType == Surface::SurfaceType::Plane) {
      auto [bounds, axes, original] =
          Geant4ShapeConverter{}.rectangleBounds(*g4Box);
      auto orientedToGlobal = axesOriented(toGlobal, axes);
      surface = Acts::Surface::makeShared<PlaneSurface>(orientedToGlobal,
                                                        std::move(bounds));
      assignMaterial(*surface.get(), original, compressed);
      return surface;
    } else {
      throw std::runtime_error("Can not convert 'G4Box' into forced shape.");
    }
  }

  // Into a Trapezoid - from Trd
  auto g4Trd = dynamic_cast<const G4Trd*>(g4Solid);
  if (g4Trd != nullptr) {
    if (forcedType == Surface::SurfaceType::Other ||
        forcedType == Surface::SurfaceType::Plane) {
      auto [bounds, axes, original] =
          Geant4ShapeConverter{}.trapezoidBounds(*g4Trd);
      auto orientedToGlobal = axesOriented(toGlobal, axes);
      surface = Acts::Surface::makeShared<PlaneSurface>(orientedToGlobal,
                                                        std::move(bounds));
      assignMaterial(*surface.get(), original, compressed);
      return surface;
    } else {
      throw std::runtime_error("Can not convert 'G4Trd' into forced shape.");
    }
  }

  // Into a Trapezoid - from Trap
  auto g4Trap = dynamic_cast<const G4Trap*>(g4Solid);
  if (g4Trap != nullptr) {
    if (forcedType == Surface::SurfaceType::Other ||
        forcedType == Surface::SurfaceType::Plane) {
      auto [bounds, axes, original] =
          Geant4ShapeConverter{}.trapezoidBounds(*g4Trap);
      auto orientedToGlobal = axesOriented(toGlobal, axes);
      surface = Acts::Surface::makeShared<PlaneSurface>(orientedToGlobal,
                                                        std::move(bounds));
      assignMaterial(*surface.get(), original, compressed);
      return surface;
    } else {
      throw std::runtime_error("Can not convert 'G4Trap' into forced shape.");
    }
  }

  // Into a Cylinder, disc or line
  auto g4Tubs = dynamic_cast<const G4Tubs*>(g4Solid);
  if (g4Tubs != nullptr) {
    double diffR = g4Tubs->GetOuterRadius() - g4Tubs->GetInnerRadius();
    double diffZ = 2 * g4Tubs->GetZHalfLength();
    // Detect if cylinder or disc case
    double original = 0.;
    if (forcedType == Surface::SurfaceType::Cylinder ||
        (diffR < diffZ && forcedType == Surface::SurfaceType::Other)) {
      auto [bounds, originalT] = Geant4ShapeConverter{}.cylinderBounds(*g4Tubs);
      original = originalT;
      surface = Acts::Surface::makeShared<CylinderSurface>(toGlobal,
                                                           std::move(bounds));
    } else if (forcedType == Surface::SurfaceType::Disc ||
               forcedType == Surface::SurfaceType::Other) {
      auto [bounds, originalT] = Geant4ShapeConverter{}.radialBounds(*g4Tubs);
      original = originalT;
      surface =
          Acts::Surface::makeShared<DiscSurface>(toGlobal, std::move(bounds));
    } else if (forcedType == Surface::SurfaceType::Straw) {
      auto bounds = Geant4ShapeConverter{}.lineBounds(*g4Tubs);
      surface =
          Acts::Surface::makeShared<StrawSurface>(toGlobal, std::move(bounds));

    } else {
      throw std::runtime_error("Can not convert 'G4Tubs' into forced shape.");
    }
    assignMaterial(*surface.get(), original, compressed);
    return surface;
  }

  return nullptr;
}

Acts::Material Acts::Geant4MaterialConverter::material(
    const G4Material& g4Material, double compression) {
  auto X0 = g4Material.GetRadlen();
  auto L0 = g4Material.GetNuclearInterLength();
  auto Rho = g4Material.GetDensity();

  // Get{A,Z} is only meaningful for single-element materials (according to
  // the Geant4 docs). Need to compute average manually.
  auto g4Elements = g4Material.GetElementVector();
  auto g4Fraction = g4Material.GetFractionVector();
  auto g4NElements = g4Material.GetNumberOfElements();
  double Ar = 0;
  double Z = 0;
  if (g4NElements == 1) {
    Ar = g4Elements->at(0)->GetN();
    Z = g4Material.GetZ();
  } else {
    for (std::size_t i = 0; i < g4NElements; i++) {
      Ar += g4Elements->at(i)->GetN() * g4Fraction[i];
      Z += g4Elements->at(i)->GetZ() * g4Fraction[i];
    }
  }

  return Material::fromMassDensity(X0 / compression, L0 / compression, Ar, Z,
                                   compression * Rho);
}

std::shared_ptr<Acts::HomogeneousSurfaceMaterial>
Acts::Geant4MaterialConverter::surfaceMaterial(const G4Material& g4Material,
                                               double original,
                                               double compressed) {
  double compression = original / compressed;
  return std::make_shared<HomogeneousSurfaceMaterial>(
      MaterialSlab(material(g4Material, compression), compressed));
}

std::shared_ptr<Acts::CylinderVolumeBounds>
Acts::Geant4VolumeConverter::cylinderBounds(const G4Tubs& g4Tubs) {
  using C = Acts::CylinderVolumeBounds;

  std::array<double, C::eSize> tArray = {};
  tArray[C::eMinR] = static_cast<double>(g4Tubs.GetInnerRadius());
  tArray[C::eMaxR] = static_cast<double>(g4Tubs.GetOuterRadius());
  tArray[C::eHalfLengthZ] = static_cast<double>(g4Tubs.GetZHalfLength());
  tArray[C::eHalfPhiSector] =
      0.5 * static_cast<double>(g4Tubs.GetDeltaPhiAngle());
  tArray[C::eAveragePhi] = static_cast<double>(g4Tubs.GetStartPhiAngle());

  return std::make_shared<CylinderVolumeBounds>(tArray);
}
