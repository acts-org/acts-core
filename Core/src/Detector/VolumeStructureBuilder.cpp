// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/VolumeStructureBuilder.hpp"

#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/ConeVolumeBounds.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GenericCuboidVolumeBounds.hpp"
#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

Acts::Experimental::VolumeStructureBuilder::VolumeStructureBuilder(
    const Acts::Experimental::VolumeStructureBuilder::Config& cfg,
    std::unique_ptr<const Acts::Logger> mlogger)
    : IExternalStructureBuilder(), m_cfg(cfg), m_logger(std::move(mlogger)) {
  // Sanity cross-checks
  if (m_cfg.boundValues.empty() && !m_cfg.extent.has_value()) {
    throw std::invalid_argument(
        "VolumeStructureBuilder: no extent nor boundary values given");
  }
  // Check for the bounds type
  if (m_cfg.boundsType == VolumeBounds::BoundsType::eOther) {
    throw std::invalid_argument(
        "VolumeStructureBuilder: no known volume bounds type provided.");
  }
}

Acts::Experimental::ExternalStructure
Acts::Experimental::VolumeStructureBuilder::construct(
    [[maybe_unused]] const Acts::GeometryContext& gctx) const {
  // Print out the auxiliary information
  if (!m_cfg.auxiliary.empty()) {
    ACTS_DEBUG(m_cfg.auxiliary);
  }

  // The volume bounds to be constructed
  std::unique_ptr<VolumeBounds> volumeBounds = nullptr;

  // The transform from the extent
  auto eTransform = Transform3::Identity();
  std::vector<ActsScalar> boundValues = m_cfg.boundValues;

  // This code dispatches into the dedicated volume types
  switch (m_cfg.boundsType) {
    case VolumeBounds::BoundsType::eCone: {
      ACTS_VERBOSE("Building conical volume bounds.");
      // Cone translation - only pre-defined values
      if (boundValues.size() < 5u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for cone volume bounds need to "
            "be fully provided, they can not be estimated from an Extent "
            "object. It needs at least 5 parameters, while " +
            std::to_string(boundValues.size()) + " where given");
      }
      auto bArray = toArray<ConeVolumeBounds::BoundValues::eSize, ActsScalar>(
          boundValues);
      volumeBounds = std::make_unique<ConeVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eCuboid: {
      ACTS_VERBOSE("Building cuboid volume bounds.");
      // Cuboid translation - either parameters / or extent
      if (boundValues.empty() && m_cfg.extent.has_value()) {
        ACTS_VERBOSE("Cuboid: estimate parameters from Extent.");
        const auto& vExtent = m_cfg.extent.value();
        if (vExtent.constrains(BinningValue::binX) &&
            vExtent.constrains(BinningValue::binY) &&
            vExtent.constrains(BinningValue::binZ)) {
          eTransform.pretranslate(Vector3(vExtent.medium(BinningValue::binX),
                                          vExtent.medium(BinningValue::binY),
                                          vExtent.medium(BinningValue::binZ)));
          boundValues = {0.5 * vExtent.interval(BinningValue::binX),
                         0.5 * vExtent.interval(BinningValue::binY),
                         0.5 * vExtent.interval(BinningValue::binZ)};

        } else {
          throw std::runtime_error(
              "VolumeStructureBuilder: translation to cuboid does not work as "
              "the extent does not constrain all necessary value.");
        }
      } else if (boundValues.size() < 3u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for cuboid volume bounds need "
            "to be fully provided, it needs exactly 3 parameters, while " +
            std::to_string(boundValues.size()) + " where given");
      }
      auto bArray =
          toArray<CuboidVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<CuboidVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eCutoutCylinder: {
      ACTS_VERBOSE("Building cutout cylindrical volume bounds.");
      // Cutout cylinder translation - only parameters
      if (boundValues.size() < 5u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for cutout cylinder volume "
            "bounds need to be fully provided, they can not be estimated from "
            "an Extent object. It needs exactly 3 parameters, while " +
            std::to_string(boundValues.size()) + " where given");
      }
      auto bArray =
          toArray<CutoutCylinderVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<CutoutCylinderVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eCylinder: {
      ACTS_VERBOSE("Building cylindrical volume bounds.");
      // Cylinder translation - either parameters / or extent
      if (boundValues.empty() && m_cfg.extent.has_value()) {
        ACTS_VERBOSE("Cylinder: estimate parameters from Extent.");
        const auto& vExtent = m_cfg.extent.value();
        if (vExtent.constrains(BinningValue::binR) &&
            vExtent.constrains(BinningValue::binZ)) {
          eTransform.pretranslate(
              Vector3(0., 0., vExtent.medium(BinningValue::binZ)));
          boundValues = {vExtent.min(BinningValue::binR),
                         vExtent.max(BinningValue::binR),
                         0.5 * vExtent.interval(BinningValue::binZ)};
          if (vExtent.constrains(BinningValue::binPhi)) {
            boundValues.push_back(0.5 * vExtent.interval(BinningValue::binPhi));
            boundValues.push_back(vExtent.medium(BinningValue::binPhi));
          }
        } else {
          throw std::runtime_error(
              "VolumeStructureBuilder: translation to cuboid does not work as "
              "the extent does not constrain all necessary values.");
        }
      } else if (boundValues.size() < 3u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for cylinder volume "
            "bounds need to be fully provided, it needs at least 3 parameters, "
            "while " +
            std::to_string(boundValues.size()) + " where given");
      }
      // Check if phi has been constraint, otherwise fill it with full coverage
      if (boundValues.size() == 3u) {
        boundValues.push_back(M_PI);
        boundValues.push_back(0.);
      }
      ACTS_VERBOSE(" - cylindrical shape with [iR, oR, hZ, sPhi, mPhi] = "
                   << boundValues[0] << ", " << boundValues[1] << ", "
                   << boundValues[2] << ", " << boundValues[3] << ", "
                   << boundValues[4]);
      auto bArray =
          toArray<CylinderVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<CylinderVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eGenericCuboid: {
      ACTS_VERBOSE("Building generic cuboid volume bounds.");
      // Generic cuboid translation - parameters only
      if (boundValues.size() < GenericCuboidVolumeBounds::BoundValues::eSize) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for generic cuboid volume "
            "bounds need to be provided, they can not be estimated from an "
            "Extent object. It needs exactly 24 parameters, while " +
            std::to_string(boundValues.size()) + " where given");
      }
      auto bArray =
          toArray<GenericCuboidVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<GenericCuboidVolumeBounds>(bArray);
    } break;
    case VolumeBounds::BoundsType::eTrapezoid: {
      ACTS_VERBOSE("Building trapezoid volume bounds.");
      // Trapezoid translation - parameters only
      if (boundValues.size() < 4u) {
        throw std::runtime_error(
            "VolumeStructureBuilder: parameters for trapezoid volume bounds "
            "need to be provided, they can not be estimated from an Extent "
            "object. It needs at least 4 parameters, while " +
            std::to_string(boundValues.size()) + " where given");
      }
      auto bArray =
          toArray<TrapezoidVolumeBounds::BoundValues::eSize>(boundValues);
      volumeBounds = std::make_unique<TrapezoidVolumeBounds>(bArray);
    } break;
    default:
      break;
  }

  Transform3 fTransform = m_cfg.transform * eTransform;
  ACTS_VERBOSE(" - translation: " << Acts::toString(fTransform.translation()));
  if (!fTransform.rotation().isApprox(
          Acts::Transform3::Identity().rotation())) {
    ACTS_VERBOSE(" - rotation: " << Acts::toString(fTransform.rotation()));
  }
  // Return the transform, the volume bounds, and some default portal
  // generators
  return {fTransform, std::move(volumeBounds), defaultPortalGenerator()};
}
