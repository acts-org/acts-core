// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/VolumeBounds.hpp"

namespace Acts {

std::ostream& operator<<(std::ostream& sl, const Acts::VolumeBounds& vb) {
  return vb.toStream(sl);
}

std::ostream& operator<<(std::ostream& sl, const VolumeBounds::BoundsType& bt) {
  switch (bt) {
    using enum VolumeBounds::BoundsType;
    case eCone:
      sl << "Cone";
      break;
    case eCuboid:
      sl << "Cuboid";
      break;
    case eCutoutCylinder:
      sl << "CutoutCylinder";
      break;
    case eCylinder:
      sl << "Cylinder";
      break;
    case eGenericCuboid:
      sl << "GenericCuboid";
      break;
    case eTrapezoid:
      sl << "Trapezoid";
      break;
    case eOther:
      sl << "Other";
      break;
  }
  return sl;
}

}  // namespace Acts
