// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CylinderVolumeStack.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <memory>
#include <numbers>
#include <sstream>

namespace Acts {

struct CylinderVolumeStack::VolumeTuple {
  Volume* volume{};
  const CylinderVolumeBounds* bounds{};
  std::shared_ptr<CylinderVolumeBounds> updatedBounds{};
  Transform3 localTransform = Transform3::Identity();
  Transform3 globalTransform = Transform3::Identity();

  bool transformDirty = false;

  VolumeTuple(Volume& volume_, const Transform3& groupTransform)
      : volume{&volume_},
        localTransform{groupTransform.inverse() * volume_.transform()},
        globalTransform{volume_.transform()} {
    bounds = dynamic_cast<const CylinderVolumeBounds*>(&volume_.volumeBounds());
    assert(bounds != nullptr);
    updatedBounds = std::make_shared<CylinderVolumeBounds>(*bounds);
  }

  double midZ() const { return localTransform.translation()[eZ]; }
  double halfLengthZ() const {
    return updatedBounds->get(CylinderVolumeBounds::eHalfLengthZ);
  }
  double minZ() const { return midZ() - halfLengthZ(); }
  double maxZ() const { return midZ() + halfLengthZ(); }

  double minR() const {
    return updatedBounds->get(CylinderVolumeBounds::eMinR);
  }
  double maxR() const {
    return updatedBounds->get(CylinderVolumeBounds::eMaxR);
  }
  double midR() const { return (minR() + maxR()) / 2.0; }

  void set(std::initializer_list<
           std::pair<CylinderVolumeBounds::BoundValues, double>>
               keyValues) {
    updatedBounds->set(keyValues);
  }

  void setLocalTransform(const Transform3& transform,
                         const Transform3& groupTransform) {
    localTransform = transform;
    globalTransform = groupTransform * localTransform;
    transformDirty = true;
  }

  void commit(const Logger& logger) {
    // make a copy so we can't accidentally modify in-place
    auto copy = std::make_shared<CylinderVolumeBounds>(*updatedBounds);

    std::optional<Transform3> transform = std::nullopt;
    if (transformDirty) {
      transform = globalTransform;
    }

    volume->update(std::move(updatedBounds), transform, logger);
    bounds = copy.get();
    updatedBounds = std::move(copy);
    transformDirty = false;
  }
};

CylinderVolumeStack::CylinderVolumeStack(std::vector<Volume*>& volumes,
                                         AxisDirection direction,
                                         VolumeAttachmentStrategy strategy,
                                         VolumeResizeStrategy resizeStrategy,
                                         const Logger& logger)
    : Volume(initialVolume(volumes)),
      m_direction(direction),
      m_resizeStrategy(resizeStrategy),
      m_volumes(volumes) {
  initializeOuterVolume(direction, strategy, logger);
}

Volume& CylinderVolumeStack::initialVolume(
    const std::vector<Volume*>& volumes) {
  if (volumes.empty()) {
    throw std::invalid_argument(
        "CylinderVolumeStack requires at least one volume");
  }
  return *volumes.front();
}

void CylinderVolumeStack::initializeOuterVolume(
    AxisDirection direction, VolumeAttachmentStrategy strategy,
    const Logger& logger) {
  ACTS_DEBUG("Creating CylinderVolumeStack from "
             << m_volumes.size() << " volumes in direction "
             << axisDirectionName(direction));
  if (m_volumes.empty()) {
    throw std::invalid_argument(
        "CylinderVolumeStack requires at least one volume");
  }

  if (direction != Acts::AxisDirection::AxisZ &&
      direction != Acts::AxisDirection::AxisR) {
    throw std::invalid_argument(axisDirectionName(direction) +
                                " is not supported ");
  }

  // For alignment check, we have to pick one of the volumes as the base
  m_groupTransform = m_volumes.front()->transform();
  ACTS_VERBOSE("Initial group transform is:\n" << m_groupTransform.matrix());

  std::vector<VolumeTuple> volumeTuples;
  volumeTuples.reserve(m_volumes.size());

  for (const auto& volume : m_volumes) {
    const auto* cylinderBounds =
        dynamic_cast<const CylinderVolumeBounds*>(&volume->volumeBounds());
    if (cylinderBounds == nullptr) {
      throw std::invalid_argument{
          "CylinderVolumeStack requires all volumes to "
          "have CylinderVolumeBounds"};
    }

    checkNoPhiOrBevel(*cylinderBounds, logger);

    volumeTuples.emplace_back(*volume, m_groupTransform);
  }

  ACTS_DEBUG("*** Initial volume configuration:");
  printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);

  if (m_volumes.size() == 1) {
    ACTS_VERBOSE("Only one volume, returning");
    setTransform(m_volumes.front()->transform());
    const auto* cylBounds = dynamic_cast<const CylinderVolumeBounds*>(
        &m_volumes.front()->volumeBounds());
    assert(cylBounds != nullptr && "Volume bounds are not cylinder bounds");
    Volume::update(std::make_shared<CylinderVolumeBounds>(*cylBounds),
                   std::nullopt, logger);
    ACTS_VERBOSE("Transform is now: " << m_transform.matrix());
    return;
  }

  ACTS_VERBOSE("Checking volume alignment");
  checkVolumeAlignment(volumeTuples, logger);

  if (direction == Acts::AxisDirection::AxisZ) {
    ACTS_VERBOSE("Sorting by volume z position");
    std::ranges::sort(volumeTuples, {}, [](const auto& v) {
      return v.localTransform.translation()[eZ];
    });

    ACTS_VERBOSE("Checking for overlaps and attaching volumes in z");
    std::vector<VolumeTuple> gapVolumes =
        checkOverlapAndAttachInZ(volumeTuples, strategy, logger);

    ACTS_VERBOSE("Appending "
                 << gapVolumes.size()
                 << " gap volumes to the end of the volume vector");
    std::copy(gapVolumes.begin(), gapVolumes.end(),
              std::back_inserter(volumeTuples));

    ACTS_VERBOSE("*** Volume configuration after z attachment:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::VERBOSE);

    ACTS_VERBOSE("Synchronizing bounds in r");
    const auto [minR, maxR] = synchronizeRBounds(volumeTuples, logger);

    for (auto& vt : volumeTuples) {
      ACTS_VERBOSE("Updated bounds for volume at z: "
                   << vt.localTransform.translation()[eZ]);
      ACTS_VERBOSE(*vt.updatedBounds);

      vt.commit(logger);
    }

    ACTS_VERBOSE("*** Volume configuration after r synchronization:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::VERBOSE);

    std::ranges::sort(volumeTuples, {}, [](const auto& v) { return v.midZ(); });

    m_volumes.clear();
    for (const auto& vt : volumeTuples) {
      m_volumes.push_back(vt.volume);
    }

    ACTS_DEBUG("*** Volume configuration after final z sorting:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);

    double minZ = volumeTuples.front().minZ();
    double maxZ = volumeTuples.back().maxZ();

    double midZ = (minZ + maxZ) / 2.0;
    double hlZ = (maxZ - minZ) / 2.0;

    m_transform = m_groupTransform * Translation3{0, 0, midZ};

    Volume::update(std::make_shared<CylinderVolumeBounds>(minR, maxR, hlZ),
                   std::nullopt, logger);
    ACTS_DEBUG("Outer bounds are:\n" << volumeBounds());
    ACTS_DEBUG("Outer transform / new group transform is:\n"
               << m_transform.matrix());

    // Update group transform to the new center
    // @TODO: We probably can reuse m_transform
    m_groupTransform = m_transform;

  } else if (direction == Acts::AxisDirection::AxisR) {
    ACTS_VERBOSE("Sorting by volume r middle point");
    std::ranges::sort(volumeTuples, {}, [](const auto& v) { return v.midR(); });

    ACTS_VERBOSE("Checking for overlaps and attaching volumes in r");
    std::vector<VolumeTuple> gapVolumes =
        checkOverlapAndAttachInR(volumeTuples, strategy, logger);

    ACTS_VERBOSE("Appending "
                 << gapVolumes.size()
                 << " gap volumes to the end of the volume vector");
    std::copy(gapVolumes.begin(), gapVolumes.end(),
              std::back_inserter(volumeTuples));

    ACTS_VERBOSE("*** Volume configuration after r attachment:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::VERBOSE);

    ACTS_VERBOSE("Synchronizing bounds in z");
    const auto [minZ, maxZ] = synchronizeZBounds(volumeTuples, logger);

    for (auto& vt : volumeTuples) {
      ACTS_VERBOSE("Updated bounds for volume at r: " << vt.midR());
      ACTS_VERBOSE(*vt.updatedBounds);
      vt.commit(logger);
    }

    ACTS_VERBOSE("*** Volume configuration after z synchronization:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::VERBOSE);

    std::ranges::sort(volumeTuples, {}, [](const auto& v) { return v.midR(); });

    m_volumes.clear();
    for (const auto& vt : volumeTuples) {
      m_volumes.push_back(vt.volume);
    }

    ACTS_DEBUG("*** Volume configuration after final r sorting:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);

    double minR = volumeTuples.front().minR();
    double maxR = volumeTuples.back().maxR();

    double midZ = (minZ + maxZ) / 2.0;
    double hlZ = (maxZ - minZ) / 2.0;

    m_transform = m_groupTransform * Translation3{0, 0, midZ};

    Volume::update(std::make_shared<CylinderVolumeBounds>(minR, maxR, hlZ),
                   std::nullopt, logger);

    ACTS_DEBUG("Outer bounds are:\n" << volumeBounds());
    ACTS_DEBUG("Outer transform is:\n" << m_transform.matrix());

    // Update group transform to the new center
    // @TODO: We probably can reuse m_transform
    m_groupTransform = m_transform;

  } else {
    ACTS_ERROR("Binning in " << axisDirectionName(direction)
                             << " is not supported");
    throw std::invalid_argument(axisDirectionName(direction) +
                                " is not supported ");
  }
}

void CylinderVolumeStack::overlapPrint(
    AxisDirection direction, const CylinderVolumeStack::VolumeTuple& a,
    const CylinderVolumeStack::VolumeTuple& b, const Logger& logger) {
  if (logger().doPrint(Acts::Logging::DEBUG)) {
    std::stringstream ss;
    ss << std::fixed;
    ss << std::setprecision(3);
    ss << std::setfill(' ');

    int w = 9;

    ACTS_VERBOSE("Checking overlap between");
    if (direction == AxisDirection::AxisZ) {
      ss << " - " << " z: [ " << std::setw(w) << a.minZ() << " <- "
         << std::setw(w) << a.midZ() << " -> " << std::setw(w) << a.maxZ()
         << " ]";
      ACTS_VERBOSE(ss.str());

      ss.str("");
      ss << " - " << " z: [ " << std::setw(w) << b.minZ() << " <- "
         << std::setw(w) << b.midZ() << " -> " << std::setw(w) << b.maxZ()
         << " ]";
      ACTS_VERBOSE(ss.str());
    } else {
      ss << " - " << " r: [ " << std::setw(w) << a.minR() << " <-> "
         << std::setw(w) << a.maxR() << " ]";
      ACTS_VERBOSE(ss.str());

      ss.str("");
      ss << " - " << " r: [ " << std::setw(w) << b.minR() << " <-> "
         << std::setw(w) << b.maxR() << " ]";
      ACTS_VERBOSE(ss.str());
    }
  }
}

std::vector<CylinderVolumeStack::VolumeTuple>
CylinderVolumeStack::checkOverlapAndAttachInZ(std::vector<VolumeTuple>& volumes,
                                              VolumeAttachmentStrategy strategy,
                                              const Logger& logger) {
  // Preconditions: volumes are sorted by z
  std::vector<VolumeTuple> gapVolumes;
  for (std::size_t i = 0; i < volumes.size() - 1; i++) {
    std::size_t j = i + 1;
    auto& a = volumes.at(i);
    auto& b = volumes.at(j);

    overlapPrint(AxisDirection::AxisZ, a, b, logger);

    if (a.maxZ() > b.minZ()) {
      ACTS_ERROR(" -> Overlap in z");
      throw std::invalid_argument("Volumes overlap in z");
    } else {
      ACTS_VERBOSE(" -> No overlap");
    }

    constexpr auto tolerance = s_onSurfaceTolerance;
    if (std::abs(a.maxZ() - b.minZ()) < tolerance) {
      ACTS_VERBOSE("No gap between volumes, no attachment needed");
    } else {
      double gapWidth = b.minZ() - a.maxZ();
      ACTS_VERBOSE("Gap width: " << gapWidth);

      ACTS_VERBOSE("Synchronizing bounds in z with strategy: " << strategy);
      switch (strategy) {
        case VolumeAttachmentStrategy::Midpoint: {
          ACTS_VERBOSE(" -> Strategy: Expand both volumes to midpoint");

          double aZMidNew = (a.minZ() + a.maxZ()) / 2.0 + gapWidth / 4.0;
          double aHlZNew = a.halfLengthZ() + gapWidth / 4.0;
          ACTS_VERBOSE("  - New halflength for first volume: " << aHlZNew);
          ACTS_VERBOSE("  - New bounds for first volume: ["
                       << (aZMidNew - aHlZNew) << " <- " << aZMidNew << " -> "
                       << (aZMidNew + aHlZNew) << "]");

          assert(std::abs(a.minZ() - (aZMidNew - aHlZNew)) < 1e-9 &&
                 "Volume shrunk");
          assert(aHlZNew >= a.halfLengthZ() && "Volume shrunk");

          double bZMidNew = (b.minZ() + b.maxZ()) / 2.0 - gapWidth / 4.0;
          double bHlZNew = b.halfLengthZ() + gapWidth / 4.0;
          ACTS_VERBOSE("  - New halflength for second volume: " << bHlZNew);
          ACTS_VERBOSE("  - New bounds for second volume: ["
                       << (bZMidNew - bHlZNew) << " <- " << bZMidNew << " -> "
                       << (bZMidNew + bHlZNew) << "]");

          assert(bHlZNew >= b.halfLengthZ() && "Volume shrunk");
          assert(std::abs(b.maxZ() - (bZMidNew + bHlZNew)) < 1e-9 &&
                 "Volume shrunk");

          a.setLocalTransform(Transform3{Translation3{0, 0, aZMidNew}},
                              m_groupTransform);
          a.updatedBounds->set(CylinderVolumeBounds::eHalfLengthZ, aHlZNew);

          b.setLocalTransform(Transform3{Translation3{0, 0, bZMidNew}},
                              m_groupTransform);
          b.updatedBounds->set(CylinderVolumeBounds::eHalfLengthZ, bHlZNew);

          break;
        }
        case VolumeAttachmentStrategy::First: {
          ACTS_VERBOSE(" -> Strategy: Expand first volume");
          double aZMidNew = (a.minZ() + b.minZ()) / 2.0;
          double aHlZNew = (b.minZ() - a.minZ()) / 2.0;
          ACTS_VERBOSE("  - Gap width: " << gapWidth);
          ACTS_VERBOSE("  - New bounds for first volume: ["
                       << (aZMidNew - aHlZNew) << " <- " << aZMidNew << " -> "
                       << (aZMidNew + aHlZNew) << "]");

          assert(std::abs(a.minZ() - (aZMidNew - aHlZNew)) < 1e-9 &&
                 "Volume shrunk");
          assert(aHlZNew >= a.halfLengthZ() && "Volume shrunk");

          a.setLocalTransform(Transform3{Translation3{0, 0, aZMidNew}},
                              m_groupTransform);
          a.updatedBounds->set(CylinderVolumeBounds::eHalfLengthZ, aHlZNew);

          break;
        }
        case VolumeAttachmentStrategy::Second: {
          ACTS_VERBOSE(" -> Strategy: Expand second volume");
          double bZMidNew = (a.maxZ() + b.maxZ()) / 2.0;
          double bHlZNew = (b.maxZ() - a.maxZ()) / 2.0;
          ACTS_VERBOSE("  - New halflength for second volume: " << bHlZNew);
          ACTS_VERBOSE("  - New bounds for second volume: ["
                       << (bZMidNew - bHlZNew) << " <- " << bZMidNew << " -> "
                       << (bZMidNew + bHlZNew) << "]");

          assert(bHlZNew >= b.halfLengthZ() && "Volume shrunk");
          assert(std::abs(b.maxZ() - (bZMidNew + bHlZNew)) < 1e-9 &&
                 "Volume shrunk");

          b.setLocalTransform(Transform3{Translation3{0, 0, bZMidNew}},
                              m_groupTransform);
          b.updatedBounds->set(CylinderVolumeBounds::eHalfLengthZ, bHlZNew);
          break;
        }
        case VolumeAttachmentStrategy::Gap: {
          ACTS_VERBOSE(" -> Strategy: Create a gap volume");
          double gapHlZ = (b.minZ() - a.maxZ()) / 2.0;
          double gapMidZ = (b.minZ() + a.maxZ()) / 2.0;

          ACTS_VERBOSE("  - Gap half length: " << gapHlZ
                                               << " at z: " << gapMidZ);

          double minR = std::min(a.minR(), b.minR());
          double maxR = std::max(a.maxR(), b.maxR());

          Transform3 gapLocalTransform{Translation3{0, 0, gapMidZ}};
          Transform3 gapGlobalTransform = m_groupTransform * gapLocalTransform;
          auto gapBounds =
              std::make_shared<CylinderVolumeBounds>(minR, maxR, gapHlZ);

          auto gap = addGapVolume(gapGlobalTransform, gapBounds);
          gapVolumes.emplace_back(*gap, m_groupTransform);

          break;
        }
        default:
          ACTS_ERROR("Attachment strategy " << strategy << " not implemented");
          std::stringstream ss;
          ss << strategy;
          throw std::invalid_argument("Attachment strategy " + ss.str() +
                                      " not implemented");
      }
    }
  }

  return gapVolumes;
}

std::vector<CylinderVolumeStack::VolumeTuple>
CylinderVolumeStack::checkOverlapAndAttachInR(std::vector<VolumeTuple>& volumes,
                                              VolumeAttachmentStrategy strategy,
                                              const Logger& logger) {
  std::vector<VolumeTuple> gapVolumes;
  for (std::size_t i = 0; i < volumes.size() - 1; i++) {
    std::size_t j = i + 1;
    auto& a = volumes.at(i);
    auto& b = volumes.at(j);

    overlapPrint(AxisDirection::AxisR, a, b, logger);

    if (a.maxR() > b.minR()) {
      ACTS_ERROR(" -> Overlap in r");
      throw std::invalid_argument("Volumes overlap in r");
    } else {
      ACTS_VERBOSE(" -> No overlap");
    }

    constexpr auto tolerance = s_onSurfaceTolerance;
    if (std::abs(a.maxR() - b.minR()) < tolerance) {
      ACTS_VERBOSE("No gap between volumes, no attachment needed");
    } else {
      double gapWidth = b.minR() - a.maxR();
      ACTS_VERBOSE("Gap width: " << gapWidth);

      ACTS_VERBOSE("Synchronizing bounds in r with strategy: " << strategy);
      switch (strategy) {
        case VolumeAttachmentStrategy::Midpoint: {
          ACTS_VERBOSE(" -> Strategy: Expand both volumes to midpoint");

          a.set({{CylinderVolumeBounds::eMaxR, a.maxR() + gapWidth / 2.0}});
          b.set({{CylinderVolumeBounds::eMinR, b.minR() - gapWidth / 2.0}});

          break;
        }
        case VolumeAttachmentStrategy::First: {
          ACTS_VERBOSE(" -> Strategy: Expand first volume");

          a.set({{CylinderVolumeBounds::eMaxR, b.minR()}});

          break;
        }
        case VolumeAttachmentStrategy::Second: {
          ACTS_VERBOSE(" -> Strategy: Expand second volume");

          b.set({{CylinderVolumeBounds::eMinR, a.maxR()}});

          break;
        }
        case VolumeAttachmentStrategy::Gap: {
          ACTS_VERBOSE(" -> Strategy: Create a gap volume");

          auto gapBounds = std::make_shared<CylinderVolumeBounds>(
              a.maxR(), b.minR(), a.halfLengthZ());
          auto gap = addGapVolume(m_groupTransform, gapBounds);

          gapVolumes.emplace_back(*gap, m_groupTransform);
          break;
        }
        default:
          ACTS_ERROR("Attachment strategy " << strategy << " not implemented");
          std::stringstream ss;
          ss << strategy;
          throw std::invalid_argument("Attachment strategy " + ss.str() +
                                      " not implemented");
      }
    }
  }

  return gapVolumes;
}

void CylinderVolumeStack::printVolumeSequence(
    const std::vector<VolumeTuple>& volumes, const Logger& logger,
    Acts::Logging::Level lvl) {
  if (!logger().doPrint(lvl)) {
    return;
  }
  for (const auto& vt : volumes) {
    std::stringstream ss;
    ss << std::fixed;
    ss << std::setprecision(3);
    ss << std::setfill(' ');

    int w = 9;
    ss << "z: [ " << std::setw(w) << vt.minZ() << " <- " << std::setw(w)
       << vt.midZ() << " -> " << std::setw(w) << vt.maxZ() << " ], r: [ "
       << std::setw(w) << vt.minR() << " <-> " << std::setw(w) << vt.maxR()
       << " ]";

    logger().log(lvl, ss.str());
  }
}

void CylinderVolumeStack::checkVolumeAlignment(
    const std::vector<VolumeTuple>& volumes, const Logger& logger) {
  std::size_t n = 0;
  for (auto& vt : volumes) {
    ACTS_VERBOSE("Checking volume #"
                 << n << " at z: " << vt.localTransform.translation()[eZ]);
    ACTS_VERBOSE("- Local transform is:\n" << vt.localTransform.matrix());

    // @TODO: What's a good tolerance here?
    constexpr auto tolerance = s_onSurfaceTolerance;

    // In the group coordinate system:

    // a) the volumes cannot rotate around x or y
    if (std::abs(vt.localTransform.rotation().col(eX)[eZ]) >= tolerance ||
        std::abs(vt.localTransform.rotation().col(eY)[eZ]) >= tolerance) {
      ACTS_ERROR("Volumes are not aligned: rotation is different");
      throw std::invalid_argument(
          "Volumes are not aligned: rotation is different");
    }

    ACTS_VERBOSE(" -> Rotation is ok!");

    // b) the volumes cannot have translation in x or y
    Vector2 translation = vt.localTransform.translation().head<2>();
    if (std::abs(translation[0]) > tolerance ||  //
        std::abs(translation[1]) > tolerance) {
      ACTS_ERROR("Volumes are not aligned: translation in x or y");
      throw std::invalid_argument(
          "Volumes are not aligned: translation in x or y");
    }
    ACTS_VERBOSE(" -> Translation in x/y is ok!");

    n++;
  }
}

std::pair<double, double> CylinderVolumeStack::synchronizeRBounds(
    std::vector<VolumeTuple>& volumes, const Logger& logger) {
  const double minR =
      std::min_element(volumes.begin(), volumes.end(),
                       [](const auto& a, const auto& b) {
                         return a.bounds->get(CylinderVolumeBounds::eMinR) <
                                b.bounds->get(CylinderVolumeBounds::eMinR);
                       })
          ->bounds->get(CylinderVolumeBounds::eMinR);

  const double maxR =
      std::max_element(volumes.begin(), volumes.end(),
                       [](const auto& a, const auto& b) {
                         return a.bounds->get(CylinderVolumeBounds::eMaxR) <
                                b.bounds->get(CylinderVolumeBounds::eMaxR);
                       })
          ->bounds->get(CylinderVolumeBounds::eMaxR);
  ACTS_VERBOSE("Found: minR: " << minR << " maxR: " << maxR);

  for (auto& vt : volumes) {
    vt.set({
        {CylinderVolumeBounds::eMinR, minR},
        {CylinderVolumeBounds::eMaxR, maxR},
    });
  }

  return {minR, maxR};
}

std::pair<double, double> CylinderVolumeStack::synchronizeZBounds(
    std::vector<VolumeTuple>& volumes, const Logger& logger) {
  const double minZ = std::min_element(volumes.begin(), volumes.end(),
                                       [](const auto& a, const auto& b) {
                                         return a.minZ() < b.minZ();
                                       })
                          ->minZ();

  const double maxZ = std::max_element(volumes.begin(), volumes.end(),
                                       [](const auto& a, const auto& b) {
                                         return a.maxZ() < b.maxZ();
                                       })
                          ->maxZ();
  const double midZ = (minZ + maxZ) / 2.0;
  const double hlZ = (maxZ - minZ) / 2.0;
  ACTS_DEBUG("Found overall z bounds: [ " << minZ << " <- " << midZ << " -> "
                                          << maxZ << " ]");
  const Transform3 transform{Translation3{0, 0, midZ}};

  for (auto& vt : volumes) {
    vt.set({{CylinderVolumeBounds::eHalfLengthZ, hlZ}});
    vt.setLocalTransform(transform, m_groupTransform);
  }

  return {minZ, maxZ};
}

void CylinderVolumeStack::update(std::shared_ptr<VolumeBounds> volbounds,
                                 std::optional<Transform3> transform,
                                 const Logger& logger) {
  ACTS_DEBUG(
      "Resizing CylinderVolumeStack with strategy: " << m_resizeStrategy);
  ACTS_DEBUG("Currently have " << m_volumes.size() << " children");
  ACTS_DEBUG(m_gaps.size() << " gaps");
  for (const auto& v : m_volumes) {
    ACTS_DEBUG(" - volume bounds: \n" << v->volumeBounds());
    ACTS_DEBUG("          transform: \n" << v->transform().matrix());
  }

  ACTS_DEBUG("New bounds are: \n" << *volbounds);

  auto cylBounds = std::dynamic_pointer_cast<CylinderVolumeBounds>(volbounds);
  if (cylBounds == nullptr) {
    throw std::invalid_argument(
        "CylinderVolumeStack requires CylinderVolumeBounds");
  }

  if (cylBounds == nullptr) {
    throw std::invalid_argument("New bounds are nullptr");
  }

  if (*cylBounds == volumeBounds()) {
    ACTS_VERBOSE("Bounds are the same, no resize needed");
    return;
  }

  ACTS_VERBOSE("Group transform is:\n" << m_groupTransform.matrix());
  ACTS_VERBOSE("Current transform is:\n" << m_transform.matrix());
  if (transform.has_value()) {
    ACTS_VERBOSE("Input transform:\n" << transform.value().matrix());
  }

  VolumeTuple oldVolume{*this, m_transform};
  VolumeTuple newVolume{*this, m_transform};
  newVolume.updatedBounds = std::make_shared<CylinderVolumeBounds>(*cylBounds);
  newVolume.globalTransform = transform.value_or(m_transform);
  newVolume.localTransform = m_transform.inverse() * newVolume.globalTransform;

  if (!transform.has_value()) {
    ACTS_VERBOSE("Local transform does not change");
  } else {
    ACTS_VERBOSE("Local transform changes from\n"
                 << m_groupTransform.matrix() << "\nto\n"
                 << newVolume.localTransform.matrix());
    ACTS_VERBOSE("Checking transform consistency");

    std::vector<VolumeTuple> volTemp{newVolume};
    checkVolumeAlignment(volTemp, logger);
  }

  checkNoPhiOrBevel(*cylBounds, logger);

  const double newMinR = newVolume.minR();
  const double newMaxR = newVolume.maxR();
  const double newMinZ = newVolume.minZ();
  const double newMaxZ = newVolume.maxZ();
  const double newMidZ = newVolume.midZ();
  const double newHlZ = newVolume.halfLengthZ();

  const double oldMinR = oldVolume.minR();
  const double oldMaxR = oldVolume.maxR();
  const double oldMinZ = oldVolume.minZ();
  const double oldMaxZ = oldVolume.maxZ();
  const double oldMidZ = oldVolume.midZ();
  const double oldHlZ = oldVolume.halfLengthZ();

  ACTS_VERBOSE("Previous bounds are: z: [ "
               << oldMinZ << " <- " << oldMidZ << " -> " << oldMaxZ << " ] ("
               << oldHlZ << "), r: [ " << oldMinR << " <-> " << oldMaxR
               << " ]");
  ACTS_VERBOSE("New bounds are: z:      [ "
               << newMinZ << " <- " << newMidZ << " -> " << newMaxZ << " ] ("
               << newHlZ << "), r: [ " << newMinR << " <-> " << newMaxR
               << " ]");

  constexpr auto tolerance = s_onSurfaceTolerance;
  auto same = [](double a, double b) { return std::abs(a - b) < tolerance; };

  if (!same(newMinZ, oldMinZ) && newMinZ > oldMinZ) {
    ACTS_ERROR("Shrinking the stack size in z is not supported: "
               << newMinZ << " -> " << oldMinZ);
    throw std::invalid_argument("Shrinking the stack in z is not supported");
  }

  if (!same(newMaxZ, oldMaxZ) && newMaxZ < oldMaxZ) {
    ACTS_ERROR("Shrinking the stack size in z is not supported: "
               << newMaxZ << " -> " << oldMaxZ);
    throw std::invalid_argument("Shrinking the stack in z is not supported");
  }

  if (!same(newMinR, oldMinR) && newMinR > oldMinR) {
    ACTS_ERROR("Shrinking the stack size in r is not supported: "
               << newMinR << " -> " << oldMinR);
    throw std::invalid_argument("Shrinking the stack in r is not supported");
  }

  if (!same(newMaxR, oldMaxR) && newMaxR < oldMaxR) {
    ACTS_ERROR("Shrinking the stack size in r is not supported: "
               << newMaxR << " -> " << oldMaxR);
    throw std::invalid_argument("Shrinking the stack is r in not supported");
  }

  auto isGap = [this](const Volume* vol) {
    return std::ranges::any_of(
        m_gaps, [&](const auto& gap) { return vol == gap.get(); });
  };

  if (m_direction == AxisDirection::AxisZ) {
    ACTS_VERBOSE("Stack direction is z");

    std::vector<VolumeTuple> volumeTuples;
    volumeTuples.reserve(m_volumes.size());
    std::transform(m_volumes.begin(), m_volumes.end(),
                   std::back_inserter(volumeTuples),
                   [this](const auto& volume) {
                     return VolumeTuple{*volume, m_groupTransform};
                   });

    ACTS_VERBOSE("*** Initial volume configuration:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);

    if (!same(newMinR, oldMinR) || !same(newMaxR, oldMaxR)) {
      ACTS_VERBOSE("Resize all volumes to new r bounds");
      for (auto& volume : volumeTuples) {
        volume.set({
            {CylinderVolumeBounds::eMinR, newMinR},
            {CylinderVolumeBounds::eMaxR, newMaxR},
        });
      }
      ACTS_VERBOSE("*** Volume configuration after r resizing:");
      printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);
    } else {
      ACTS_VERBOSE("R bounds are the same, no r resize needed");
    }

    if (same(newHlZ, oldHlZ)) {
      ACTS_VERBOSE("Halflength z is the same, no z resize needed");
    } else {
      if (m_resizeStrategy == VolumeResizeStrategy::Expand) {
        if (newMinZ < oldMinZ) {
          ACTS_VERBOSE("Expanding first volume to new z bounds");

          auto& first = volumeTuples.front();
          double newMinZFirst = newVolume.minZ();
          double newMidZFirst = (newMinZFirst + first.maxZ()) / 2.0;
          double newHlZFirst = (first.maxZ() - newMinZFirst) / 2.0;

          ACTS_VERBOSE(" -> first z: [ "
                       << newMinZFirst << " <- " << newMidZFirst << " -> "
                       << first.maxZ() << " ] (hl: " << newHlZFirst << ")");

          first.set({{CylinderVolumeBounds::eHalfLengthZ, newHlZFirst}});
          first.setLocalTransform(Transform3{Translation3{0, 0, newMidZFirst}},
                                  m_groupTransform);
        }

        if (newMaxZ > oldMaxZ) {
          ACTS_VERBOSE("Expanding last volume to new z bounds");

          auto& last = volumeTuples.back();
          double newMaxZLast = newVolume.maxZ();
          double newMidZLast = (last.minZ() + newMaxZLast) / 2.0;
          double newHlZLast = (newMaxZLast - last.minZ()) / 2.0;

          ACTS_VERBOSE(" -> last z: [ " << last.minZ() << " <- " << newMidZLast
                                        << " -> " << newMaxZLast
                                        << " ] (hl: " << newHlZLast << ")");

          last.set({{CylinderVolumeBounds::eHalfLengthZ, newHlZLast}});
          last.setLocalTransform(Transform3{Translation3{0, 0, newMidZLast}},
                                 m_groupTransform);
        }
      } else if (m_resizeStrategy == VolumeResizeStrategy::Gap) {
        ACTS_VERBOSE("Creating gap volumes to fill the new z bounds");

        auto printGapDimensions = [&](const VolumeTuple& gap,
                                      const std::string& prefix = "") {
          ACTS_VERBOSE(" -> gap" << prefix << ": [ " << gap.minZ() << " <- "
                                 << gap.midZ() << " -> " << gap.maxZ()
                                 << " ], r: [ " << gap.minR() << " <-> "
                                 << gap.maxR() << " ]");
        };

        if (!same(newMinZ, oldMinZ) && newMinZ < oldMinZ) {
          double gap1MinZ = newVolume.minZ();
          double gap1MaxZ = oldVolume.minZ();
          double gap1HlZ = (gap1MaxZ - gap1MinZ) / 2.0;
          double gap1PZ = (gap1MaxZ + gap1MinZ) / 2.0;

          // // check if we need a new gap volume or reuse an existing one
          auto& candidate = volumeTuples.front();
          if (isGap(candidate.volume)) {
            ACTS_VERBOSE("~> Reusing existing gap volume at negative z");

            gap1HlZ =
                candidate.bounds->get(CylinderVolumeBounds::eHalfLengthZ) +
                gap1HlZ;
            gap1MaxZ = gap1MinZ + gap1HlZ * 2;
            gap1PZ = (gap1MaxZ + gap1MinZ) / 2.0;

            printGapDimensions(candidate, " before");
            auto gap1Bounds = std::make_shared<CylinderVolumeBounds>(
                newMinR, newMaxR, gap1HlZ);
            auto gap1Transform = m_groupTransform * Translation3{0, 0, gap1PZ};
            candidate.volume->update(std::move(gap1Bounds), gap1Transform);
            candidate = VolumeTuple{*candidate.volume, m_groupTransform};
            ACTS_VERBOSE("After:");
            printGapDimensions(candidate, " after ");

          } else {
            ACTS_VERBOSE("~> Creating new gap volume at negative z");
            auto gap1Bounds = std::make_shared<CylinderVolumeBounds>(
                newMinR, newMaxR, gap1HlZ);
            auto gap1Transform = m_groupTransform * Translation3{0, 0, gap1PZ};
            auto gap1 = addGapVolume(gap1Transform, std::move(gap1Bounds));
            volumeTuples.insert(volumeTuples.begin(),
                                VolumeTuple{*gap1, m_groupTransform});
            printGapDimensions(volumeTuples.front());
          }
        }

        if (!same(newMaxZ, oldMaxZ) && newMaxZ > oldMaxZ) {
          double gap2MinZ = oldVolume.maxZ();
          double gap2MaxZ = newVolume.maxZ();
          double gap2HlZ = (gap2MaxZ - gap2MinZ) / 2.0;
          double gap2PZ = (gap2MaxZ + gap2MinZ) / 2.0;

          // check if we need a new gap volume or reuse an existing one
          auto& candidate = volumeTuples.back();
          if (isGap(candidate.volume)) {
            ACTS_VERBOSE("~> Reusing existing gap volume at positive z");

            gap2HlZ =
                candidate.bounds->get(CylinderVolumeBounds::eHalfLengthZ) +
                gap2HlZ;
            gap2MinZ = newVolume.maxZ() - gap2HlZ * 2;
            gap2PZ = (gap2MaxZ + gap2MinZ) / 2.0;

            printGapDimensions(candidate, " before");
            auto gap2Bounds = std::make_shared<CylinderVolumeBounds>(
                newMinR, newMaxR, gap2HlZ);
            auto gap2Transform = m_groupTransform * Translation3{0, 0, gap2PZ};

            candidate.volume->update(std::move(gap2Bounds), gap2Transform);
            candidate = VolumeTuple{*candidate.volume, m_groupTransform};
            printGapDimensions(candidate, " after ");
          } else {
            ACTS_VERBOSE("~> Creating new gap volume at positive z");
            auto gap2Bounds = std::make_shared<CylinderVolumeBounds>(
                newMinR, newMaxR, gap2HlZ);
            auto gap2Transform = m_groupTransform * Translation3{0, 0, gap2PZ};
            auto gap2 = addGapVolume(gap2Transform, std::move(gap2Bounds));
            volumeTuples.emplace_back(*gap2, m_groupTransform);
            printGapDimensions(volumeTuples.back());
          }
        }
      }

      ACTS_VERBOSE("*** Volume configuration after z resizing:");
      printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);
    }

    ACTS_VERBOSE("Commit and update outer vector of volumes");
    m_volumes.clear();
    for (auto& vt : volumeTuples) {
      vt.commit(logger);
      m_volumes.push_back(vt.volume);
    }

  } else if (m_direction == AxisDirection::AxisR) {
    ACTS_VERBOSE("Stack direction is r");

    std::vector<VolumeTuple> volumeTuples;
    volumeTuples.reserve(m_volumes.size());
    std::transform(m_volumes.begin(), m_volumes.end(),
                   std::back_inserter(volumeTuples),
                   [this](const auto& volume) {
                     return VolumeTuple{*volume, m_groupTransform};
                   });

    ACTS_VERBOSE("*** Initial volume configuration:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);

    ACTS_VERBOSE("Resize all volumes to new z bounds and update transforms");
    for (auto& volume : volumeTuples) {
      volume.set({
          {CylinderVolumeBounds::eHalfLengthZ, newHlZ},
      });
      volume.setLocalTransform(newVolume.localTransform, m_groupTransform);
    }

    ACTS_VERBOSE("*** Volume configuration after z resizing:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);

    if (oldMinR == newMinR && oldMaxR == newMaxR) {
      ACTS_VERBOSE("Radii are the same, no r resize needed");
    } else {
      if (m_resizeStrategy == VolumeResizeStrategy::Expand) {
        if (oldMinR > newMinR) {
          // expand innermost volume
          auto& first = volumeTuples.front();
          first.set({
              {CylinderVolumeBounds::eMinR, newMinR},
          });
          ACTS_VERBOSE(" -> z: [ " << first.minZ() << " <- " << first.midZ()
                                   << " -> " << first.maxZ() << " ], r: [ "
                                   << first.minR() << " <-> " << first.maxR()
                                   << " ]");
        }
        if (oldMaxR < newMaxR) {
          // expand outermost volume
          auto& last = volumeTuples.back();
          last.set({
              {CylinderVolumeBounds::eMaxR, newMaxR},
          });
          ACTS_VERBOSE(" -> z: [ " << last.minZ() << " <- " << last.midZ()
                                   << " -> " << last.maxZ() << " ], r: [ "
                                   << last.minR() << " <-> " << last.maxR()
                                   << " ]");
        }
      } else if (m_resizeStrategy == VolumeResizeStrategy::Gap) {
        auto printGapDimensions = [&](const VolumeTuple& gap,
                                      const std::string& prefix = "") {
          ACTS_VERBOSE(" -> gap" << prefix << ": [ " << gap.minZ() << " <- "
                                 << gap.midZ() << " -> " << gap.maxZ()
                                 << " ], r: [ " << gap.minR() << " <-> "
                                 << gap.maxR() << " ]");
        };

        if (oldMinR > newMinR) {
          auto& candidate = volumeTuples.front();
          if (isGap(candidate.volume)) {
            ACTS_VERBOSE("~> Reusing existing gap volume at inner r");
            auto& candidateCylBounds = dynamic_cast<CylinderVolumeBounds&>(
                candidate.volume->volumeBounds());
            printGapDimensions(candidate, " before");
            candidateCylBounds.set(CylinderVolumeBounds::eMinR, newMinR);
            candidate = VolumeTuple{*candidate.volume, m_groupTransform};
            printGapDimensions(candidate, " after ");
          } else {
            ACTS_VERBOSE("~> Creating new gap volume at inner r");
            auto gapBounds = std::make_shared<CylinderVolumeBounds>(
                newMinR, oldMinR, newHlZ);
            auto gapTransform = m_groupTransform;
            auto gapVolume = addGapVolume(gapTransform, gapBounds);
            volumeTuples.insert(volumeTuples.begin(),
                                VolumeTuple{*gapVolume, m_groupTransform});
            auto gap = volumeTuples.front();
            printGapDimensions(gap);
          }
        }
        if (oldMaxR < newMaxR) {
          auto& candidate = volumeTuples.back();
          if (isGap(candidate.volume)) {
            ACTS_VERBOSE("~> Reusing existing gap volume at outer r");
            auto& candidateCylBounds = dynamic_cast<CylinderVolumeBounds&>(
                candidate.volume->volumeBounds());
            printGapDimensions(candidate, " before");
            candidateCylBounds.set(CylinderVolumeBounds::eMaxR, newMaxR);
            candidate = VolumeTuple{*candidate.volume, m_groupTransform};
            printGapDimensions(candidate, " after ");
          } else {
            ACTS_VERBOSE("~> Creating new gap volume at outer r");
            auto gapBounds = std::make_shared<CylinderVolumeBounds>(
                oldMaxR, newMaxR, newHlZ);
            auto gapTransform = m_groupTransform;
            auto gapVolume = addGapVolume(gapTransform, gapBounds);
            volumeTuples.emplace_back(*gapVolume, m_groupTransform);
            auto gap = volumeTuples.back();
            printGapDimensions(gap);
          }
        }
      }

      ACTS_VERBOSE("*** Volume configuration after r resizing:");
      printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);
    }

    ACTS_VERBOSE("Commit and update outer vector of volumes");
    m_volumes.clear();
    for (auto& vt : volumeTuples) {
      vt.commit(logger);
      m_volumes.push_back(vt.volume);
    }
  }

  m_transform = newVolume.globalTransform;
  // @TODO: We probably can reuse m_transform
  m_groupTransform = m_transform;
  Volume::update(std::move(cylBounds), std::nullopt, logger);
}

void CylinderVolumeStack::checkNoPhiOrBevel(const CylinderVolumeBounds& bounds,
                                            const Logger& logger) {
  if (bounds.get(CylinderVolumeBounds::eHalfPhiSector) != std::numbers::pi) {
    ACTS_ERROR(
        "CylinderVolumeStack requires all volumes to have a full "
        "phi sector");
    throw std::invalid_argument(
        "CylinderVolumeStack requires all volumes to have a full phi sector");
  }

  if (bounds.get(CylinderVolumeBounds::eAveragePhi) != 0.0) {
    ACTS_ERROR(
        "CylinderVolumeStack requires all volumes to have an average "
        "phi of 0");
    throw std::invalid_argument(
        "CylinderVolumeStack requires all volumes to have an average phi of "
        "0");
  }

  if (bounds.get(CylinderVolumeBounds::eBevelMinZ) != 0.0) {
    ACTS_ERROR(
        "CylinderVolumeStack requires all volumes to have a bevel angle of "
        "0");
    throw std::invalid_argument(
        "CylinderVolumeStack requires all volumes to have a bevel angle of "
        "0");
  }

  if (bounds.get(CylinderVolumeBounds::eBevelMaxZ) != 0.0) {
    ACTS_ERROR(
        "CylinderVolumeStack requires all volumes to have a bevel angle of "
        "0");
    throw std::invalid_argument(
        "CylinderVolumeStack requires all volumes to have a bevel angle of "
        "0");
  }
}

std::shared_ptr<Volume> CylinderVolumeStack::addGapVolume(
    const Transform3& transform, const std::shared_ptr<VolumeBounds>& bounds) {
  auto gapVolume = std::make_shared<Volume>(transform, bounds);
  m_gaps.push_back(gapVolume);
  return gapVolume;
}

const std::vector<std::shared_ptr<Volume>>& CylinderVolumeStack::gaps() const {
  return m_gaps;
}

}  // namespace Acts
