// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <array>
#include <memory>
#include <tuple>

namespace CLHEP {
class Hep3Vector;
class HepRotation;
}  // namespace CLHEP

namespace HepGeom {
class Transform3D;
}

class G4Box;
class G4Material;
class G4Trd;
class G4Trap;
class G4Tubs;
class G4VSolid;
class G4VPhysicalVolume;
using G4ThreeVector = CLHEP::Hep3Vector;
using G4RotationMatrix = CLHEP::HepRotation;
using G4Transform3D = HepGeom::Transform3D;

namespace Acts {
struct Geant4AlgebraConverter {
  // A potential scalar between Geant4 and ACTS
  double scale = 1.;

  /// @brief Translate a geometry transform: translation only
  ///
  /// @param g4Trans the translation of the Geant4 object
  ///
  /// @return a Acts transform
  Transform3 transform(const G4ThreeVector& g4Trans);

  /// @brief Translate a geometry transform
  ///
  /// @param g4Rot the rotation of the Geant4 object
  /// @param g4Trans the translation of the Geant4 object
  ///
  /// @return a Acts transform
  Transform3 transform(const G4RotationMatrix& g4Rot,
                       const G4ThreeVector& g4Trans);

  /// @brief Translate a geometry transform
  ///
  /// @param g4Trf the Geant4 transform object
  ///
  /// @return a Acts transform
  Transform3 transform(const G4Transform3D& g4Trf);

  /// @brief Translate a geometry transform from a G4 physical volume
  ///
  /// @param g4PhysVol the Geant4 physical volume
  ///
  /// @return a Acts transform
  Transform3 transform(const G4VPhysicalVolume& g4PhysVol);
};

class AnnulusBounds;
class CylinderBounds;
class RadialBounds;
class RectangleBounds;
class TrapezoidBounds;
class PlanarBounds;
class LineBounds;

// The following set of converters convert a Geant4 volume shape
// to an ACTS surface bounds object, this is for converting the volume
// based geometry into a surfaced based one.
//
// The obviously smallest expansion/extrusion is condensed to the epsilon
// thin surface.
struct Geant4ShapeConverter {
  /// A scale between Geant4 and ACTS
  double scale = 1.;
  /// A description to keep the axis order, if it is set to false
  /// cyclic re-ordering will happen, otherwise axis flip if needed in
  /// order to keep the system right-handed
  bool keepAxisOrder = false;

  /// @brief Convert to cylinder bounds
  ///
  /// @param g4Tubs a Geant4 tube shape
  ///
  /// @return an Acts Cylinder bounds object, and thickness
  std::tuple<std::shared_ptr<CylinderBounds>, double> cylinderBounds(
      const G4Tubs& g4Tubs);

  /// @brief Convert to radial bounds
  ///
  /// @param g4Tubs a Geant4 tube shape
  ///
  /// @return an Acts Radial bounds object and thickness
  std::tuple<std::shared_ptr<RadialBounds>, double> radialBounds(
      const G4Tubs& g4Tubs);

  /// @brief Convert to line/straw bounds
  ///
  /// @param g4Tubs a Geant4 tube shape
  ///
  /// @return an Acts line bounds object and thickness
  std::shared_ptr<LineBounds> lineBounds(const G4Tubs& g4Tubs);

  /// @brief Convert to rectangle bounds
  ///
  /// @param g4Box a Geant4 box shape
  ///
  /// @return an ACTS Rectangle bounds shape,  axis orientation, and thickness
  std::tuple<std::shared_ptr<RectangleBounds>, std::array<int, 2u>, double>
  rectangleBounds(const G4Box& g4Box);

  /// @brief Convert to trapezoid bounds - from Trd
  ///
  /// @param g4Trd a Geant4 trapezoid shape
  ///
  /// @return an ACTS Trapezoid bounds object, axis orientation, and thickness
  std::tuple<std::shared_ptr<TrapezoidBounds>, std::array<int, 2u>, double>
  trapezoidBounds(const G4Trd& g4Trd);

  /// @brief Convert to trapezoid bounds - from Trap
  ///
  /// @param g4Trap a Geant4 trapezoid shape
  ///
  /// @return an ACTS Trapezoid bounds object, axis orientation, and thickness
  std::tuple<std::shared_ptr<TrapezoidBounds>, std::array<int, 2u>, double>
  trapezoidBounds(const G4Trap& g4Trap);

  /// @brief Convert to general solid into a planar shape
  ///
  /// @param g4Solid a Geant4 solid shape
  ///
  /// @return an ACTS Planar bounds object,
  /// the axes, and the thickness of the compressed dimension
  std::tuple<std::shared_ptr<PlanarBounds>, std::array<int, 2u>, double>
  planarBounds(const G4VSolid& g4Solid);
};

struct Geant4PhysicalVolumeConverter {
  /// Optionally allow to foce a type, throws exception if not possible
  Surface::SurfaceType forcedType = Surface::SurfaceType::Other;

  /// @brief Convert a Geant4 phsyical volume to a surface
  ///
  /// @param g4PhysVol the physical volume to be constructed
  /// @param toGlobal the global transformation before the volume
  /// @param convertMaterial a material conversion flag
  /// @param compressed the compressed thickness of the converted material
  ///
  /// @return a shared surface object
  std::shared_ptr<Surface> surface(const G4VPhysicalVolume& g4PhysVol,
                                   const Transform3& toGlobal,
                                   bool convertMaterial = false,
                                   double compressed = 0.);
};

class Material;
class HomogeneousSurfaceMaterial;
class HomogeneousVolumeMaterial;

struct Geant4MaterialConverter {
  Material material(const G4Material& g4Material, double compression = 1);

  /// @brief Convert a Geant4 material to a surface material description
  ///
  /// @param g4Material the geant4 material description
  /// @param original the original thickness
  /// @param compressed the compressed thickness
  ///
  std::shared_ptr<HomogeneousSurfaceMaterial> surfaceMaterial(
      const G4Material& g4Material, double original, double compressed);
};

class CylinderVolumeBounds;

struct Geant4VolumeConverter {
  /// @brief Convert to cylinder bounds
  ///
  /// @param g4Tubs a Geant4 tube shape
  ///
  /// @return an Acts Cylinder bounds object
  std::shared_ptr<CylinderVolumeBounds> cylinderBounds(const G4Tubs& g4Tubs);
};

}  // namespace Acts
