// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Detector/detail/SupportSurfacesHelper.hpp"

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>

Acts::Experimental::detail::SupportSurfacesHelper::SupportSurfaceComponents
Acts::Experimental::detail::SupportSurfacesHelper::CylindricalSupport::
operator()(const Extent& lExtent) const {
  // Bail out if you have no measure of R, Z
  if (!lExtent.constrains(AxisDirection::AxisZ) ||
      !lExtent.constrains(AxisDirection::AxisR)) {
    throw std::invalid_argument(
        "SupportSurfacesHelper::CylindricalSupport::operator() - z or "
        "r are not constrained.");
  }

  // Min / Max z  with clearances adapted
  double minZ = lExtent.min(AxisDirection::AxisZ) + std::abs(zClearance[0u]);
  double maxZ = lExtent.max(AxisDirection::AxisZ) - std::abs(zClearance[1u]);

  // Phi sector
  double hPhiSector = std::numbers::pi;
  double avgPhi = 0.;
  if (lExtent.constrains(AxisDirection::AxisPhi)) {
    // Min / Max phi  with clearances adapted
    double minPhi =
        lExtent.min(AxisDirection::AxisPhi) + std::abs(phiClearance[0u]);
    double maxPhi =
        lExtent.max(AxisDirection::AxisPhi) - std::abs(phiClearance[1u]);
    hPhiSector = 0.5 * (maxPhi - minPhi);
    avgPhi = 0.5 * (minPhi + maxPhi);
  }

  Transform3 transform = Transform3::Identity();
  if (std::abs(minZ + maxZ) > s_onSurfaceTolerance) {
    transform.pretranslate(Vector3(0., 0., 0.5 * (minZ + maxZ)));
  }

  // The Radius estimation
  double r = rOffset < 0 ? lExtent.min(AxisDirection::AxisR) + rOffset
                         : lExtent.max(AxisDirection::AxisR) + rOffset;
  if (rOffset == 0.) {
    r = lExtent.medium(AxisDirection::AxisR);
  }
  // Components are resolved and returned as a tuple
  return {
      type, {r, 0.5 * (maxZ - minZ), hPhiSector, avgPhi, 0., 0.}, transform};
}

Acts::Experimental::detail::SupportSurfacesHelper::SupportSurfaceComponents
Acts::Experimental::detail::SupportSurfacesHelper::DiscSupport::operator()(
    const Extent& lExtent) const {
  // Bail out if you have no measure of R, Z
  if (!lExtent.constrains(AxisDirection::AxisZ) ||
      !lExtent.constrains(AxisDirection::AxisR)) {
    throw std::invalid_argument(
        "SupportSurfacesHelper::DiscSupport::operator() - z or "
        "r are not constrained.");
  }

  // Min / Max r  with clearances adapted
  double minR = lExtent.min(AxisDirection::AxisR) + std::abs(rClearance[0u]);
  double maxR = lExtent.max(AxisDirection::AxisR) - std::abs(rClearance[1u]);

  // Phi sector
  double hPhiSector = std::numbers::pi;
  double avgPhi = 0.;
  if (lExtent.constrains(AxisDirection::AxisPhi)) {
    // Min / Max phi  with clearances adapted
    double minPhi =
        lExtent.min(AxisDirection::AxisPhi) + std::abs(phiClearance[0u]);
    double maxPhi =
        lExtent.max(AxisDirection::AxisPhi) - std::abs(phiClearance[1u]);
    hPhiSector = 0.5 * (maxPhi - minPhi);
    avgPhi = 0.5 * (minPhi + maxPhi);
  }

  // The z position estimate
  double z = zOffset < 0 ? lExtent.min(AxisDirection::AxisZ) + zOffset
                         : lExtent.max(AxisDirection::AxisZ) + zOffset;
  if (zOffset == 0.) {
    z = lExtent.medium(AxisDirection::AxisZ);
  }

  Transform3 transform = Transform3::Identity();
  transform.pretranslate(Vector3(0., 0., z));

  // Components are resolved and returned as a tuple
  return {type, {minR, maxR, hPhiSector, avgPhi}, transform};
}

Acts::Experimental::detail::SupportSurfacesHelper::SupportSurfaceComponents
Acts::Experimental::detail::SupportSurfacesHelper::RectangularSupport::
operator()(const Extent& lExtent) const {
  // Bail out if you have no measure of X, Y, Z
  if (!(lExtent.constrains(AxisDirection::AxisX) &&
        lExtent.constrains(AxisDirection::AxisY) &&
        lExtent.constrains(AxisDirection::AxisZ))) {
    throw std::invalid_argument(
        "SupportSurfacesHelper::RectangularSupport::operator() - x, y or "
        "z are not constrained.");
  }

  // Set the local coordinates - cyclic permutation
  std::array<AxisDirection, 2> locals = {AxisDirection::AxisX,
                                         AxisDirection::AxisY};
  if (pPlacement == AxisDirection::AxisX) {
    locals = {AxisDirection::AxisY, AxisDirection::AxisZ};
  } else if (pPlacement == AxisDirection::AxisY) {
    locals = {AxisDirection::AxisZ, AxisDirection::AxisX};
  }

  // Make the rectangular shape
  double minX = lExtent.min(locals[0]) + std::abs(loc0Clearance[0u]);
  double maxX = lExtent.max(locals[0]) - std::abs(loc0Clearance[1u]);
  double minY = lExtent.min(locals[1]) + std::abs(loc1Clearance[0u]);
  double maxY = lExtent.max(locals[1]) - std::abs(loc1Clearance[1u]);

  double gPlacement = lExtent.medium(pPlacement) + pOffset;
  Vector3 placement = Vector3::Zero();
  placement[toUnderlying(pPlacement)] = gPlacement;

  Transform3 transform = Transform3::Identity();
  transform.pretranslate(placement);

  return {type, {minX, minY, maxX, maxY}, transform};
}

std::vector<std::shared_ptr<Acts::Surface>>
Acts::Experimental::detail::SupportSurfacesHelper::cylindricalSupport(
    const SupportSurfaceComponents& components, unsigned int splits) {
  // Resolve the components
  auto [type, values, transform] = components;

  // Parameter size check
  if (values.size() != 6u) {
    throw std::invalid_argument(
        "SupportSurfacesHelper::cylindricalSupport(...) - "
        "values vector has wrong size, requires 6 parameters.");
  }

  // Surface type check
  if (type != Surface::SurfaceType::Cylinder) {
    throw std::invalid_argument(
        "SupportSurfacesHelper::cylindricalSupport(...) - "
        "surface type is not a cylinder.");
  }

  std::array<double, 6u> bounds = {};
  std::copy_n(values.begin(), 6u, bounds.begin());

  // Return vector for generated surfaces
  std::vector<std::shared_ptr<Acts::Surface>> cSupport;
  if (splits == 1u) {
    // No splitting is done in this case
    cSupport.push_back(Surface::makeShared<CylinderSurface>(
        transform, std::make_shared<CylinderBounds>(bounds)));
  } else {
    // Split into n(splits) planar surfaces, prep work:
    double r = bounds[0u];
    double halfZ = bounds[1u];
    double minPhi = bounds[3u] - bounds[2u];
    double maxPhi = bounds[3u] + bounds[2u];
    double dHalfPhi = (maxPhi - minPhi) / (2 * splits);
    double cosPhiHalf = std::cos(dHalfPhi);
    double sinPhiHalf = std::sin(dHalfPhi);
    double planeR = r * cosPhiHalf;
    double planeHalfX = r * sinPhiHalf;
    double planeZ = transform.translation().z();

    auto sRectangle =
        std::make_shared<Acts::RectangleBounds>(planeHalfX, halfZ);
    // Now create the Trapezoids
    for (unsigned int iphi = 0; iphi < splits; ++iphi) {
      // Get the moduleTransform
      double phi = -std::numbers::pi + (2 * iphi + 1) * dHalfPhi;
      double cosPhi = std::cos(phi);
      double sinPhi = std::sin(phi);
      double planeX = planeR * cosPhi;
      double planeY = planeR * sinPhi;

      Acts::Vector3 planeCenter(planeX, planeY, planeZ);
      Acts::Vector3 planeAxisZ(cosPhi, sinPhi, 0.);
      Acts::Vector3 planeAxisY(0., 0., 1.);
      Acts::Vector3 planeAxisX = planeAxisY.cross(planeAxisZ);

      RotationMatrix3 planeRotation;
      planeRotation.col(0) = planeAxisX;
      planeRotation.col(1) = planeAxisY;
      planeRotation.col(2) = planeAxisZ;

      Transform3 sTransform{planeRotation};
      sTransform.pretranslate(planeCenter);
      // Place it
      cSupport.push_back(
          Surface::makeShared<PlaneSurface>(sTransform, sRectangle));
    }
  }

  return cSupport;
}

std::vector<std::shared_ptr<Acts::Surface>>
Acts::Experimental::detail::SupportSurfacesHelper::discSupport(
    const SupportSurfaceComponents& components, unsigned int splits) {
  // Resolve the components
  auto [type, values, transform] = components;

  // Parameter size check
  if (values.size() != 4u) {
    throw std::invalid_argument(
        "SupportSurfacesHelper::discSupport(...) - "
        "values vector has wrong size, requires 4 parameters.");
  }

  // Surface type check
  if (type != Surface::SurfaceType::Disc) {
    throw std::invalid_argument(
        "SupportSurfacesHelper::discSupport(...) - "
        "surface type is not a disc.");
  }

  std::array<double, 4u> bounds = {};
  std::copy_n(values.begin(), 4u, bounds.begin());

  // Return vector for generated surfaces
  std::vector<std::shared_ptr<Acts::Surface>> dSupport;
  if (splits == 1u) {
    // No splitting is done in this case
    dSupport.push_back(Surface::makeShared<DiscSurface>(
        transform, std::make_shared<RadialBounds>(bounds)));
  } else {
    // Split into n(splits) planar surfaces in phi, prep work:
    double minR = bounds[0u];
    double maxR = bounds[1u];
    double minPhi = bounds[3u] - bounds[2u];
    double maxPhi = bounds[3u] + bounds[2u];
    double dHalfPhi = (maxPhi - minPhi) / (2 * splits);
    double cosPhiHalf = std::cos(dHalfPhi);
    double sinPhiHalf = std::sin(dHalfPhi);
    double maxLocY = maxR * cosPhiHalf;
    double minLocY = minR * cosPhiHalf;
    double hR = 0.5 * (maxLocY + minLocY);
    double hY = 0.5 * (maxLocY - minLocY);
    double hXminY = minR * sinPhiHalf;
    double hXmaxY = maxR * sinPhiHalf;
    // Split trapezoid
    auto sTrapezoid =
        std::make_shared<Acts::TrapezoidBounds>(hXminY, hXmaxY, hY);
    Vector3 zAxis = transform.rotation().col(2);
    double zPosition = transform.translation().z();
    // Now create the Trapezoids
    for (unsigned int iphi = 0; iphi < splits; ++iphi) {
      // Create the split module transform
      double phi = -std::numbers::pi + (2 * iphi + 1) * dHalfPhi;
      auto sTransform = Transform3(
          Translation3(hR * std::cos(phi), hR * std::sin(phi), zPosition) *
          AngleAxis3(phi - std::numbers::pi / 2., zAxis));
      // Place it
      dSupport.push_back(
          Surface::makeShared<PlaneSurface>(sTransform, sTrapezoid));
    }
  }
  return dSupport;
}

std::vector<std::shared_ptr<Acts::Surface>>
Acts::Experimental::detail::SupportSurfacesHelper::rectangularSupport(
    const SupportSurfaceComponents& components) {
  // Resolve the components
  auto [type, values, transform] = components;

  // Parameter size check
  if (values.size() != 4u) {
    throw std::invalid_argument(
        "SupportSurfacesHelper::rectangularSupport(...) - "
        "values vector has wrong size, requires 4 parameters.");
  }

  // Surface type check
  if (type != Surface::SurfaceType::Plane) {
    throw std::invalid_argument(
        "SupportSurfacesHelper::rectangularSupport(...) - "
        "surface type is not a plane.");
  }

  std::array<double, 4u> bounds = {};
  std::copy_n(values.begin(), 4u, bounds.begin());

  return {Surface::makeShared<PlaneSurface>(
      transform, std::make_shared<RectangleBounds>(bounds))};
}

void Acts::Experimental::detail::SupportSurfacesHelper::addSupport(
    std::vector<std::shared_ptr<Surface>>& layerSurfaces,
    std::vector<std::size_t>& assignToAll, const Extent& layerExtent,
    const SurfaceComponentsCreator& componentCreator,
    unsigned int supportSplits) {
  // Get the main support surface components
  auto supportComponents = componentCreator(layerExtent);
  const auto& sType = std::get<0>(supportComponents);

  std::vector<std::shared_ptr<Acts::Surface>> supportSurfaces = {};

  if (sType == Surface::SurfaceType::Cylinder) {
    supportSurfaces = cylindricalSupport(supportComponents, supportSplits);
  } else if (sType == Surface::SurfaceType::Disc) {
    supportSurfaces = discSupport(supportComponents, supportSplits);
  } else if (sType == Surface::SurfaceType::Plane) {
    supportSurfaces = rectangularSupport(supportComponents);
  } else {
    throw std::invalid_argument(
        "SupportSurfacesHelper: currently only cylindrical/disc/rectangle "
        "support building is possible.");
  }

  // Remember the surfaces to be assigned to all bins, once the
  // support surfaces are split they enter the standard bin assignment
  if (supportSplits == 1u && supportSurfaces.size() == 1u) {
    assignToAll.push_back(layerSurfaces.size());
  }
  // Add those to the layer surfaces
  layerSurfaces.insert(layerSurfaces.end(), supportSurfaces.begin(),
                       supportSurfaces.end());
}
