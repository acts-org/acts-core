// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/BoundingBox.hpp"

#include <array>
#include <iomanip>
#include <memory>
#include <ostream>
#include <vector>

namespace Acts {

class CylinderBounds;
class ConeBounds;
class RadialBounds;
class PlanarBounds;

/// @class ConeVolumeBounds
///
/// Volume bound class for describing conical volumes
/// either with cylindrical inlay or outer boundary, it also allows
/// for a sectoral description
class ConeVolumeBounds : public VolumeBounds {
 public:
  /// @enum BoundValues for readability
  enum BoundValues : unsigned int {
    eInnerAlpha = 0,
    eInnerOffsetZ = 1,
    eOuterAlpha = 2,
    eOuterOffsetZ = 3,
    eHalfLengthZ = 4,
    eAveragePhi = 5,
    eHalfPhiSector = 6,
    eSize
  };

  ConeVolumeBounds() = delete;

  /// Constructor - for general cone-cone setups
  ///
  /// @param innerAlpha The opening angle of the inner cone (0 if no cone)
  /// @param innerOffsetZ The tip z position in of the inner cone, w.r.t center
  /// @param outerAlpha  The opening angle of the outer cone (0 if no cone)
  /// @param outerOffsetZ The tip z position in of the outer cone, w.r.t center
  /// @param halflengthZ The minimum z value of the inner and outer cones
  /// @param averagePhi The phi orientation of the sector
  /// @param halfPhiSector The opening angle phi sector
  ConeVolumeBounds(double innerAlpha, double innerOffsetZ, double outerAlpha,
                   double outerOffsetZ, double halflengthZ, double averagePhi,
                   double halfPhiSector) noexcept(false);

  /// Constructor - for general cylidner-cone setups
  ///
  /// @param cylinderR The inner radius of the cylinder
  /// @param alpha  The opening angle of the cone (0 if no cone)
  /// @param offsetZ The tip  z position in of the cone, w.r.t center
  /// @param halflengthZ The minimum z value of the inner and outer cones
  /// @param averagePhi The phi orientation of the sector (defaulted to 0)
  /// @param halfPhiSector The opening angle phi sector
  ///
  /// @note depending on cylinderR > coneR it is constructing a cone with
  /// cylindrical cutout or a cylinder with conical cutout
  ConeVolumeBounds(double cylinderR, double alpha, double offsetZ,
                   double halflengthZ, double averagePhi,
                   double halfPhiSector) noexcept(false);

  /// Constructor - from a fixed size array
  ///
  /// @param values The bound values
  ConeVolumeBounds(const std::array<double, eSize>& values) noexcept(false)
      : m_values(values) {
    checkConsistency();
    buildSurfaceBounds();
  }

  ConeVolumeBounds(const ConeVolumeBounds& cobo) = default;
  ~ConeVolumeBounds() override = default;
  ConeVolumeBounds& operator=(const ConeVolumeBounds& cobo) = default;

  VolumeBounds::BoundsType type() const final { return VolumeBounds::eCone; }

  /// Return the bound values as dynamically sized vector
  ///
  /// @return this returns a copy of the internal values
  std::vector<double> values() const final;

  /// This method checks if position in the 3D volume
  /// frame is inside the cylinder
  ///
  /// @param pos is the position in volume frame to be checked
  /// @param tol is the absolute tolerance to be applied
  bool inside(const Vector3& pos, double tol = 0.) const final;

  /// Oriented surfaces, i.e. the decomposed boundary surfaces and the
  /// according navigation direction into the volume given the normal
  /// vector on the surface
  ///
  /// @param transform is the 3D transform to be applied to the boundary
  /// surfaces to position them in 3D space
  ///
  /// It will throw an exception if the orientation prescription is not adequate
  ///
  /// @return a vector of surfaces bounding this volume
  std::vector<OrientedSurface> orientedSurfaces(
      const Transform3& transform = Transform3::Identity()) const final;

  /// Construct bounding box for this shape
  /// @param trf Optional transform
  /// @param envelope Optional envelope to add / subtract from min/max
  /// @param entity Entity to associate this bounding box with
  /// @return Constructed bounding box
  Volume::BoundingBox boundingBox(const Transform3* trf = nullptr,
                                  const Vector3& envelope = {0, 0, 0},
                                  const Volume* entity = nullptr) const final;

  /// Access to the bound values
  /// @param bValue the class nested enum for the array access
  double get(BoundValues bValue) const { return m_values[bValue]; }

  // Return the derived innerRmin
  double innerRmin() const;

  // Return the derived innerRmin
  double innerRmax() const;

  // Return the derived inner tan(alpha)
  double innerTanAlpha() const;

  // Return the derived outerRmin
  double outerRmin() const;

  // Return the derived outerRmax
  double outerRmax() const;

  // Return the derived outer tan(alpha)
  double outerTanAlpha() const;

  /// Output Method for std::ostream
  ///
  /// @param os is ostream operator to be dumped into
  std::ostream& toStream(std::ostream& os) const final;

 private:
  /// Check the input values for consistency,
  /// will throw a logic_exception if consistency is not given
  void checkConsistency() noexcept(false);

  /// Create the surface bounds
  void buildSurfaceBounds();

  /// The bound values
  std::array<double, eSize> m_values;
  std::shared_ptr<CylinderBounds> m_innerCylinderBounds{nullptr};
  std::shared_ptr<ConeBounds> m_innerConeBounds{nullptr};
  std::shared_ptr<ConeBounds> m_outerConeBounds{nullptr};
  std::shared_ptr<CylinderBounds> m_outerCylinderBounds{nullptr};
  std::shared_ptr<RadialBounds> m_negativeDiscBounds{nullptr};
  std::shared_ptr<RadialBounds> m_positiveDiscBounds{nullptr};
  std::shared_ptr<PlanarBounds> m_sectorBounds{nullptr};

  /// Derived values
  double m_innerRmin = 0.;
  double m_innerRmax = 0.;
  double m_innerTanAlpha = 0.;
  double m_outerRmin = 0.;
  double m_outerRmax = 0.;
  double m_outerTanAlpha = 0.;
};

}  // namespace Acts
