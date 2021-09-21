// This file is part of the Acts project.
//
// Copyright (C) 2016-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Material/MaterialSlab.hpp"

#include <memory>
#include <vector>

namespace Acts {

/// This enum describes the type of surface material mapping
enum MappingType { PreMapping = -1, Default = 0, PostMapping = 1, Sensor = 2 };

/// @class ISurfaceMaterial
///
/// Virtual base class of surface based material description
///
/// MaterialSlab that are associated to a surface,
/// extended by certain special representations (binned, homogenous)
///
class ISurfaceMaterial {
 public:
  /// Constructor
  ISurfaceMaterial() = default;

  /// Constructor
  ///
  /// @param splitFactor is the splitting ratio between pre/post update
  ISurfaceMaterial(double splitFactor) : m_splitFactor(splitFactor) {}

  /// Constructor
  ///
  /// @param splitFactor is the splitting ratio between pre/post update
  /// @param mappingType is the type of surface mapping associated to the surface
  ISurfaceMaterial(double splitFactor, Acts::MappingType mappingType)
      : m_splitFactor(splitFactor), m_mappingType(mappingType) {}

  /// Destructor
  virtual ~ISurfaceMaterial() = default;

  /// Scale operator
  ///
  /// @param scale is the scale factor applied
  virtual ISurfaceMaterial& operator*=(double scale) = 0;

  /// Return method for full material description of the Surface
  /// - from local coordinate on the surface
  ///
  /// @param lp is the local position used for the (eventual) lookup
  ///
  /// @return const MaterialSlab
  virtual const MaterialSlab& materialSlab(const Vector2& lp) const = 0;

  /// Return method for full material description of the Surface
  /// - from the global coordinates
  ///
  /// @param gp is the global position used for the (eventual) lookup
  ///
  /// @return const MaterialSlab
  virtual const MaterialSlab& materialSlab(const Vector3& gp) const = 0;

  /// Direct access via bins to the MaterialSlab
  ///
  /// @param bin0 is the material bin in dimension 0
  /// @param bin1 is the material bin in dimension 1
  virtual const MaterialSlab& materialSlab(size_t bin0, size_t bin1) const = 0;

  /// Update pre factor
  ///
  /// @param pDir is the navigation direction through the surface
  /// @param mStage is the material update directive (onapproach, full, onleave)
  double factor(NavigationDirection pDir, MaterialUpdateStage mStage) const;

  /// Return the type of surface material mapping
  ///
  MappingType mappingType() const { return m_mappingType; }

  /// Return method for fully scaled material description of the Surface
  /// - from local coordinate on the surface
  ///
  /// @param lp is the local position used for the (eventual) lookup
  /// @param pDir is the navigation direction through the surface
  /// @param mStage is the material update directive (onapproach, full, onleave)
  ///
  /// @return MaterialSlab
  MaterialSlab materialSlab(const Vector2& lp, NavigationDirection pDir,
                            MaterialUpdateStage mStage) const;

  /// Return method for full material description of the Surface
  /// - from the global coordinates
  ///
  /// @param gp is the global position used for the (eventual) lookup
  /// @param pDir is the navigation direction through the surface
  /// @param mStage is the material update directive (onapproach, full, onleave)
  ///
  /// @return MaterialSlab
  MaterialSlab materialSlab(const Vector3& gp, NavigationDirection pDir,
                            MaterialUpdateStage mStage) const;

  /// @brief output stream operator
  ///
  /// Prints information about this object to the output stream using the
  /// virtual ISurfaceMaterial::toStream method
  ///
  /// @return modified output stream object
  friend std::ostream& operator<<(std::ostream& out,
                                  const ISurfaceMaterial& sm) {
    sm.toStream(out);
    return out;
  }

  /// Output Method for std::ostream, to be overloaded by child classes
  virtual std::ostream& toStream(std::ostream& sl) const = 0;

 protected:
  double m_splitFactor{1.};  //!< the split factor in favour of oppositePre
  MappingType m_mappingType{
      Acts::MappingType::Default};  //!< Use the default mapping type by default
};

inline double ISurfaceMaterial::factor(NavigationDirection pDir,
                                       MaterialUpdateStage mStage) const {
  if (mStage == Acts::fullUpdate) {
    return 1.;
  }
  return (pDir * mStage > 0 ? m_splitFactor : 1. - m_splitFactor);
}

inline MaterialSlab ISurfaceMaterial::materialSlab(
    const Vector2& lp, NavigationDirection pDir,
    MaterialUpdateStage mStage) const {
  // The plain material properties associated to this bin
  MaterialSlab plainMatProp = materialSlab(lp);
  // Scale if you have material to scale
  if (plainMatProp) {
    double scaleFactor = factor(pDir, mStage);
    if (scaleFactor == 0.) {
      return MaterialSlab();
    }
    plainMatProp.scaleThickness(scaleFactor);
  }
  return plainMatProp;
}

inline MaterialSlab ISurfaceMaterial::materialSlab(
    const Vector3& gp, NavigationDirection pDir,
    MaterialUpdateStage mStage) const {
  // The plain material properties associated to this bin
  MaterialSlab plainMatProp = materialSlab(gp);
  // Scale if you have material to scale
  if (plainMatProp) {
    double scaleFactor = factor(pDir, mStage);
    if (scaleFactor == 0.) {
      return MaterialSlab();
    }
    plainMatProp.scaleThickness(scaleFactor);
  }
  return plainMatProp;
}

}  // namespace Acts
