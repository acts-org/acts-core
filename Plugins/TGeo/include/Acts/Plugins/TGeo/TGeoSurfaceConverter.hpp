// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <cmath>
#include <memory>
#include <numbers>
#include <string>
#include <tuple>

#include "RtypesCore.h"

class TGeoShape;
class TGeoMatrix;

namespace Acts {

class CylinderBounds;
class DiscBounds;
class PlanarBounds;
class Surface;
class TGeoDetectorElement;

/// Helper struct to convert TGeoShapes into Surface or Volume Bounds
struct TGeoSurfaceConverter {
  /// Convert a TGeoShape into cylinder surface components
  ///
  /// @param tgShape The TGeoShape
  /// @param rotation The rotation matrix as Double_t* from root
  /// @param translation The translation vector as Double_t* from root
  /// @param axes The axes definition
  /// @param scalor The unit scalor between TGeo and Acts
  ///
  /// @return tuple of DiscBounds, Transform, thickness
  static std::tuple<std::shared_ptr<const CylinderBounds>, const Transform3,
                    double>
  cylinderComponents(const TGeoShape& tgShape, const Double_t* rotation,
                     const Double_t* translation, const std::string& axes,
                     double scalor = 10.) noexcept(false);

  /// Convert a TGeoShape into disk surface components
  ///
  /// @param tgShape The TGeoShape
  /// @param rotation The rotation matrix as Double_t* from root
  /// @param translation The translation vector as Double_t* from root
  /// @param axes The axes definition
  /// @param scalor The unit scalor between TGeo and Acts
  ///
  /// @return tuple of DiscBounds, Transform, thickness
  static std::tuple<std::shared_ptr<const DiscBounds>, const Transform3, double>
  discComponents(const TGeoShape& tgShape, const Double_t* rotation,
                 const Double_t* translation, const std::string& axes,
                 double scalor = 10.) noexcept(false);

  /// Convert a TGeoShape into plane surface components
  ///
  /// @param tgShape The TGeoShape
  /// @param rotation The rotation matrix as Double_t* from root
  /// @param translation The translation as a Double_t*
  /// @param axes The axes definition
  /// @param scalor The unit scalor between TGeo and Acts
  ///
  /// @return tuple of PlanarBounds, Transform, thickness
  static std::tuple<std::shared_ptr<const PlanarBounds>, const Transform3,
                    double>
  planeComponents(const TGeoShape& tgShape, const Double_t* rotation,
                  const Double_t* translation, const std::string& axes,
                  double scalor = 10.) noexcept(false);

  /// Convert a TGeoShape to a Surface
  ///
  /// @param tgShape The TGeoShape
  /// @param tgMatrix The matrix representing the transform
  /// @param axes The axes definition
  /// @param scalor The unit scalor between TGeo and Acts
  ///
  /// @return shared pointer to a surface and the original thickness that
  /// has been condensed to the surface
  static std::tuple<std::shared_ptr<Surface>, double> toSurface(
      const TGeoShape& tgShape, const TGeoMatrix& tgMatrix,
      const std::string& axes, double scalor = 10.) noexcept(false);

  /// Translate TGeo degree [0, 360) to radian
  /// * will correct to [-pi,pi)
  /// @param degree The input in degree
  /// @return angle in radians
  static double toRadian(double degree) {
    if (degree > 180. && degree < 360.) {
      degree -= 360.;
    }
    return degree / 180. * std::numbers::pi;
  }
};

}  // namespace Acts
