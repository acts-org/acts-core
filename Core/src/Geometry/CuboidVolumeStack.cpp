// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CuboidVolumeStack.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iomanip>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace Acts {

struct CuboidVolumeStack::VolumeTuple {
  Volume* volume{};
  const CuboidVolumeBounds* bounds{};
  std::shared_ptr<CuboidVolumeBounds> updatedBounds{};
  Transform3 localTransform = Transform3::Identity();
  Transform3 globalTransform = Transform3::Identity();

  bool transformDirty = false;

  VolumeTuple(Volume& volume_, const Transform3& groupTransform)
      : volume{&volume_},
        localTransform{groupTransform.inverse() * volume_.transform()},
        globalTransform{volume_.transform()} {
    bounds = dynamic_cast<const CuboidVolumeBounds*>(&volume_.volumeBounds());
    assert(bounds != nullptr);
    updatedBounds = std::make_shared<CuboidVolumeBounds>(*bounds);
  }

  double mid(AxisDirection direction) const {
    return localTransform.translation()[axisToIndex(direction)];
  }
  double halfLength(AxisDirection direction) const {
    return updatedBounds->get(CuboidVolumeBounds::fromAxisDirection(direction));
  }
  double min(AxisDirection direction) const {
    return mid(direction) - halfLength(direction);
  }
  double max(AxisDirection direction) const {
    return mid(direction) + halfLength(direction);
  }

  void set(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>
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
    auto copy = std::make_shared<CuboidVolumeBounds>(*updatedBounds);

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

std::size_t CuboidVolumeStack::axisToIndex(AxisDirection direction) {
  switch (direction) {
    case AxisDirection::AxisX:
      return 0;
      break;
    case AxisDirection::AxisY:
      return 1;
      break;
    case AxisDirection::AxisZ:
      return 2;
      break;
    default:
      throw std::invalid_argument("Invalid axis direction");
  }
}

std::pair<AxisDirection, AxisDirection> CuboidVolumeStack::getOrthogonalAxes(
    AxisDirection direction) {
  switch (direction) {
    case AxisDirection::AxisX:
      return {AxisDirection::AxisY, AxisDirection::AxisZ};
      break;
    case AxisDirection::AxisY:
      return {AxisDirection::AxisZ, AxisDirection::AxisX};
      break;
    case AxisDirection::AxisZ:
      return {AxisDirection::AxisX, AxisDirection::AxisY};
      break;
    default:
      throw std::invalid_argument("Invalid axis direction");
  }
}

CuboidVolumeStack::CuboidVolumeStack(std::vector<Volume*>& volumes,
                                     AxisDirection direction,
                                     VolumeAttachmentStrategy strategy,
                                     VolumeResizeStrategy resizeStrategy,
                                     const Logger& logger)
    : Volume(initialVolume(volumes)),
      m_dir(direction),
      m_resizeStrategy(resizeStrategy),
      m_volumes(volumes) {
  std::tie(m_dirOrth1, m_dirOrth2) = getOrthogonalAxes(m_dir);

  initializeOuterVolume(strategy, logger);
}

CuboidVolumeStack::CuboidVolumeStack(std::vector<Volume*>& volumes,
                                     const Vector3& direction,
                                     VolumeAttachmentStrategy strategy,
                                     VolumeResizeStrategy resizeStrategy,
                                     const Logger& logger)
    : Volume(initialVolume(volumes)),
      m_resizeStrategy(resizeStrategy),
      m_volumes(volumes) {
  Vector3 localDirVector =
      m_volumes.front()->transform().rotation().inverse() * direction;
  if ((localDirVector - Vector3::UnitX()).norm() < 1e-4) {
    m_dir = AxisDirection::AxisX;
  } else if ((localDirVector - Vector3::UnitY()).norm() < 1e-4) {
    m_dir = AxisDirection::AxisY;
  } else if ((localDirVector - Vector3::UnitZ()).norm() < 1e-4) {
    m_dir = AxisDirection::AxisZ;
  } else {
    throw std::invalid_argument("CuboidVolumeStack: Invalid axis direction");
  }

  std::tie(m_dirOrth1, m_dirOrth2) = getOrthogonalAxes(m_dir);

  initializeOuterVolume(strategy, logger);
}

Volume& CuboidVolumeStack::initialVolume(const std::vector<Volume*>& volumes) {
  if (volumes.empty()) {
    throw std::invalid_argument(
        "CuboidVolumeStack requires at least one volume");
  }
  return *volumes.front();
}

void CuboidVolumeStack::initializeOuterVolume(VolumeAttachmentStrategy strategy,
                                              const Logger& logger) {
  ACTS_DEBUG("Creating CuboidVolumeStack from " << m_volumes.size()
                                                << " volumes in direction "
                                                << axisDirectionName(m_dir));
  if (m_volumes.empty()) {
    throw std::invalid_argument(
        "CuboidVolumeStack requires at least one volume");
  }

  if (m_dir != Acts::AxisDirection::AxisX &&
      m_dir != Acts::AxisDirection::AxisY &&
      m_dir != Acts::AxisDirection::AxisZ) {
    throw std::invalid_argument(axisDirectionName(m_dir) +
                                " is not supported ");
  }

  // For alignment check, we have to pick one of the volumes as the base
  m_groupTransform = m_volumes.front()->transform();
  ACTS_VERBOSE("Initial group transform is:\n" << m_groupTransform.matrix());

  std::vector<VolumeTuple> volumeTuples;
  volumeTuples.reserve(m_volumes.size());

  for (const auto& volume : m_volumes) {
    const auto* cuboidBounds =
        dynamic_cast<const CuboidVolumeBounds*>(&volume->volumeBounds());
    if (cuboidBounds == nullptr) {
      throw std::invalid_argument{
          "CuboidVolumeStack requires all volumes to "
          "have CuboidVolumeBounds"};
    }

    volumeTuples.emplace_back(*volume, m_groupTransform);
  }

  ACTS_DEBUG("*** Initial volume configuration:");
  printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);

  if (m_volumes.size() == 1) {
    ACTS_VERBOSE("Only one volume, returning");
    setTransform(m_volumes.front()->transform());
    const auto* bounds = dynamic_cast<const CuboidVolumeBounds*>(
        &m_volumes.front()->volumeBounds());
    assert(bounds != nullptr && "Volume bounds are not cuboid bounds");
    Volume::update(std::make_shared<CuboidVolumeBounds>(*bounds), std::nullopt,
                   logger);
    ACTS_VERBOSE("Transform is now: " << m_transform.matrix());
    return;
  }

  ACTS_VERBOSE("Checking volume alignment");
  checkVolumeAlignment(volumeTuples, logger);

  auto dirIdx = axisToIndex(m_dir);
  ACTS_VERBOSE("Sorting by volume " << axisDirectionName(m_dir) << " position");
  std::ranges::sort(volumeTuples, {}, [dirIdx](const auto& v) {
    return v.localTransform.translation()[dirIdx];
  });
  ACTS_VERBOSE("Checking for overlaps and attaching volumes in "
               << axisDirectionName(m_dir));
  std::vector<VolumeTuple> gapVolumes =
      checkOverlapAndAttach(volumeTuples, strategy, logger);

  ACTS_VERBOSE("Appending " << gapVolumes.size()
                            << " gap volumes to the end of the volume vector");
  std::copy(gapVolumes.begin(), gapVolumes.end(),
            std::back_inserter(volumeTuples));

  ACTS_VERBOSE("*** Volume configuration after " << axisDirectionName(m_dir)
                                                 << " attachment:");
  printVolumeSequence(volumeTuples, logger, Acts::Logging::VERBOSE);

  ACTS_VERBOSE("Synchronizing bounds in " << axisDirectionName(m_dirOrth1)
                                          << "/"
                                          << axisDirectionName(m_dirOrth2));
  const auto [hl1, hl2] = synchronizeBounds(volumeTuples, logger);

  for (auto& vt : volumeTuples) {
    ACTS_VERBOSE("Updated bounds for volume at "
                 << axisDirectionName(m_dir) << ": "
                 << vt.localTransform.translation()[dirIdx]);
    ACTS_VERBOSE(*vt.updatedBounds);

    vt.commit(logger);
  }

  ACTS_VERBOSE("*** Volume configuration after "
               << axisDirectionName(m_dirOrth1) << "/"
               << axisDirectionName(m_dirOrth2) << " synchronization:");
  printVolumeSequence(volumeTuples, logger, Acts::Logging::VERBOSE);

  std::ranges::sort(volumeTuples, {},
                    [*this](const auto& v) { return v.mid(m_dir); });

  m_volumes.clear();
  for (const auto& vt : volumeTuples) {
    m_volumes.push_back(vt.volume);
  }

  ACTS_DEBUG("*** Volume configuration after final " << axisDirectionName(m_dir)
                                                     << " sorting:");
  printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);

  double min = volumeTuples.front().min(m_dir);
  double max = volumeTuples.back().max(m_dir);

  double mid = std::midpoint(min, max);
  double hl = std::midpoint(max, -min);

  Translation3 translation(Vector3::Unit(dirIdx) * mid);
  m_transform = m_groupTransform * translation;
  auto bounds = std::make_shared<CuboidVolumeBounds>(
      std::initializer_list<std::pair<CuboidVolumeBounds::BoundValues, double>>{
          {CuboidVolumeBounds::fromAxisDirection(m_dir), hl},
          {CuboidVolumeBounds::fromAxisDirection(m_dirOrth1), hl1},
          {CuboidVolumeBounds::fromAxisDirection(m_dirOrth2), hl2}});
  Volume::update(bounds, std::nullopt, logger);
  ACTS_DEBUG("Outer bounds are:\n" << volumeBounds());
  ACTS_DEBUG("Outer transform / new group transform is:\n"
             << m_transform.matrix());

  // Update group transform to the new center
  // @TODO: We probably can reuse m_transform
  m_groupTransform = m_transform;
}

void CuboidVolumeStack::overlapPrint(const CuboidVolumeStack::VolumeTuple& a,
                                     const CuboidVolumeStack::VolumeTuple& b,
                                     const Logger& logger) {
  if (logger().doPrint(Acts::Logging::DEBUG)) {
    std::stringstream ss;
    ss << std::fixed;
    ss << std::setprecision(3);
    ss << std::setfill(' ');

    int w = 9;

    ACTS_VERBOSE("Checking overlap between");
    ss << " - " << " " << axisDirectionName(m_dir) << ": [ " << std::setw(w)
       << a.min(m_dir) << " <- " << std::setw(w) << a.mid(m_dir) << " -> "
       << std::setw(w) << a.max(m_dir) << " ]";
    ACTS_VERBOSE(ss.str());

    ss.str("");
    ss << " - " << " " << axisDirectionName(m_dir) << ": [ " << std::setw(w)
       << b.min(m_dir) << " <- " << std::setw(w) << b.mid(m_dir) << " -> "
       << std::setw(w) << b.max(m_dir) << " ]";
    ACTS_VERBOSE(ss.str());
  }
}

std::vector<CuboidVolumeStack::VolumeTuple>
CuboidVolumeStack::checkOverlapAndAttach(std::vector<VolumeTuple>& volumes,
                                         VolumeAttachmentStrategy strategy,
                                         const Logger& logger) {
  // Preconditions: volumes are sorted along stacking direction
  auto dirIdx = axisToIndex(m_dir);
  auto dirBoundIdx = CuboidVolumeBounds::fromAxisDirection(m_dir);

  std::vector<VolumeTuple> gapVolumes;
  for (std::size_t i = 0; i < volumes.size() - 1; i++) {
    std::size_t j = i + 1;
    auto& a = volumes.at(i);
    auto& b = volumes.at(j);

    overlapPrint(a, b, logger);

    // TODO: What's a good tolerance?
    constexpr auto tolerance = s_onSurfaceTolerance;
    if (a.max(m_dir) - tolerance > b.min(m_dir)) {
      ACTS_ERROR(" -> Overlap in " << axisDirectionName(m_dir));
      throw std::invalid_argument("Volumes overlap in " +
                                  axisDirectionName(m_dir));
    } else {
      ACTS_VERBOSE(" -> No overlap");
    }

    if (std::abs(a.max(m_dir) - b.min(m_dir)) < tolerance) {
      ACTS_VERBOSE("No gap between volumes, no attachment needed");
    } else {
      double gapWidth = b.min(m_dir) - a.max(m_dir);
      ACTS_VERBOSE("Gap width: " << gapWidth);

      ACTS_VERBOSE("Synchronizing bounds in "
                   << axisDirectionName(m_dir)
                   << " with strategy: " << strategy);
      switch (strategy) {
        case VolumeAttachmentStrategy::Midpoint: {
          ACTS_VERBOSE(" -> Strategy: Expand both volumes to midpoint");

          double aMidNew = (a.min(m_dir) + a.max(m_dir)) / 2.0 + gapWidth / 4.0;
          double aHlNew = a.halfLength(m_dir) + gapWidth / 4.0;
          ACTS_VERBOSE("  - New halflength for first volume: " << aHlNew);
          ACTS_VERBOSE("  - New bounds for first volume: ["
                       << (aMidNew - aHlNew) << " <- " << aMidNew << " -> "
                       << (aMidNew + aHlNew) << "]");

          assert(std::abs(a.min(m_dir) - (aMidNew - aHlNew)) < 1e-9 &&
                 "Volume shrunk");
          assert(aHlNew >= a.halfLength(m_dir) && "Volume shrunk");

          double bMidNew = (b.min(m_dir) + b.max(m_dir)) / 2.0 - gapWidth / 4.0;
          double bHlNew = b.halfLength(m_dir) + gapWidth / 4.0;
          ACTS_VERBOSE("  - New halflength for second volume: " << bHlNew);
          ACTS_VERBOSE("  - New bounds for second volume: ["
                       << (bMidNew - bHlNew) << " <- " << bMidNew << " -> "
                       << (bMidNew + bHlNew) << "]");

          assert(bHlNew >= b.halfLength(m_dir) && "Volume shrunk");
          assert(std::abs(b.max(m_dir) - (bMidNew + bHlNew)) < 1e-9 &&
                 "Volume shrunk");

          Translation3 translationA(Vector3::Unit(dirIdx) * aMidNew);
          a.setLocalTransform(Transform3{translationA}, m_groupTransform);
          a.updatedBounds->set(dirBoundIdx, aHlNew);

          Translation3 translationB(Vector3::Unit(dirIdx) * bMidNew);
          b.setLocalTransform(Transform3{translationB}, m_groupTransform);
          b.updatedBounds->set(dirBoundIdx, bHlNew);

          break;
        }
        case VolumeAttachmentStrategy::First: {
          ACTS_VERBOSE(" -> Strategy: Expand first volume");
          double aMidNew = (a.min(m_dir) + b.min(m_dir)) / 2.0;
          double aHlNew = (b.min(m_dir) - a.min(m_dir)) / 2.0;
          ACTS_VERBOSE("  - Gap width: " << gapWidth);
          ACTS_VERBOSE("  - New bounds for first volume: ["
                       << (aMidNew - aHlNew) << " <- " << aMidNew << " -> "
                       << (aMidNew + aHlNew) << "]");

          assert(std::abs(a.min(m_dir) - (aMidNew - aHlNew)) < 1e-9 &&
                 "Volume shrunk");
          assert(aHlNew >= a.halfLength(m_dir) && "Volume shrunk");

          Translation3 translationA(Vector3::Unit(dirIdx) * aMidNew);
          a.setLocalTransform(Transform3{translationA}, m_groupTransform);
          a.updatedBounds->set(dirBoundIdx, aHlNew);

          break;
        }
        case VolumeAttachmentStrategy::Second: {
          ACTS_VERBOSE(" -> Strategy: Expand second volume");
          double bMidNew = (a.max(m_dir) + b.max(m_dir)) / 2.0;
          double bHlNew = (b.max(m_dir) - a.max(m_dir)) / 2.0;
          ACTS_VERBOSE("  - New halflength for second volume: " << bHlNew);
          ACTS_VERBOSE("  - New bounds for second volume: ["
                       << (bMidNew - bHlNew) << " <- " << bMidNew << " -> "
                       << (bMidNew + bHlNew) << "]");

          assert(bHlNew >= b.halfLength(m_dir) && "Volume shrunk");
          assert(std::abs(b.max(m_dir) - (bMidNew + bHlNew)) < 1e-9 &&
                 "Volume shrunk");

          Translation3 translationB(Vector3::Unit(dirIdx) * bMidNew);
          b.setLocalTransform(Transform3{translationB}, m_groupTransform);
          b.updatedBounds->set(dirBoundIdx, bHlNew);
          break;
        }
        case VolumeAttachmentStrategy::Gap: {
          ACTS_VERBOSE(" -> Strategy: Create a gap volume");
          double gapHl = (b.min(m_dir) - a.max(m_dir)) / 2.0;
          double gapMid = (b.min(m_dir) + a.max(m_dir)) / 2.0;

          ACTS_VERBOSE("  - Gap half length: " << gapHl << " at "
                                               << axisDirectionName(m_dir)
                                               << ": " << gapMid);

          Translation3 gapTranslation(Vector3::Unit(dirIdx) * gapMid);

          double min1 = std::min(a.min(m_dirOrth1), b.min(m_dirOrth1));
          double max1 = std::max(a.max(m_dirOrth1), b.max(m_dirOrth1));

          double min2 = std::min(a.min(m_dirOrth2), b.min(m_dirOrth2));
          double max2 = std::max(a.max(m_dirOrth2), b.max(m_dirOrth2));

          Transform3 gapLocalTransform{gapTranslation};
          Transform3 gapGlobalTransform = m_groupTransform * gapLocalTransform;

          auto gapBounds = std::make_shared<CuboidVolumeBounds>(
              std::initializer_list<
                  std::pair<CuboidVolumeBounds::BoundValues, double>>{
                  {CuboidVolumeBounds::fromAxisDirection(m_dir), gapHl},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth1),
                   (max1 - min1) / 2},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth2),
                   (max2 - min2) / 2}});
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

void CuboidVolumeStack::printVolumeSequence(
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

    for (const auto& axis :
         {AxisDirection::AxisX, AxisDirection::AxisY, AxisDirection::AxisZ}) {
      ss << axisDirectionName(axis) << ": [ " << std::setw(w) << vt.min(axis)
         << " <- " << std::setw(w) << vt.mid(axis) << " -> " << std::setw(w)
         << vt.max(axis) << " ]\n";
    }
    logger().log(lvl, ss.str());
  }
}

void CuboidVolumeStack::checkVolumeAlignment(
    const std::vector<VolumeTuple>& volumes, const Logger& logger) const {
  std::size_t n = 0;
  auto dirIdx = axisToIndex(m_dir);
  auto dirOrth1Idx = axisToIndex(m_dirOrth1);
  auto dirOrth2Idx = axisToIndex(m_dirOrth2);

  for (auto& vt : volumes) {
    ACTS_VERBOSE("Checking volume #"
                 << n << " at " << axisDirectionName(m_dir) << ": "
                 << vt.localTransform.translation()[dirIdx]);
    ACTS_VERBOSE("- Local transform is:\n" << vt.localTransform.matrix());

    // @TODO: Rotation precision?
    constexpr auto tolerance = s_onSurfaceTolerance;

    // In the group coordinate system:

    // a) the volumes cannot have any relative rotation
    if ((vt.localTransform.rotation().matrix() - RotationMatrix3::Identity())
            .norm() > tolerance) {
      ACTS_ERROR("Volumes are not aligned: rotation is different");
      throw std::invalid_argument(
          "Volumes are not aligned: rotation is different");
    }

    ACTS_VERBOSE(" -> Rotation is ok!");

    // b) the volumes cannot have translation in orthogonal directions
    if (std::abs(vt.localTransform.translation()[dirOrth1Idx]) > tolerance ||
        std::abs(vt.localTransform.translation()[dirOrth2Idx]) > tolerance) {
      ACTS_ERROR("Volumes are not aligned: translation in "
                 << axisDirectionName(m_dirOrth1) << " or "
                 << axisDirectionName(m_dirOrth2));
      throw std::invalid_argument("Volumes are not aligned: translation in " +
                                  axisDirectionName(m_dirOrth1) + " or " +
                                  axisDirectionName(m_dirOrth2));
    }
    ACTS_VERBOSE(" -> Translation in " << axisDirectionName(m_dirOrth1) << "/"
                                       << axisDirectionName(m_dirOrth2)
                                       << " is ok!");

    n++;
  }
}

std::pair<double, double> CuboidVolumeStack::synchronizeBounds(
    std::vector<VolumeTuple>& volumes, const Logger& logger) {
  auto boundDirOrth1 = CuboidVolumeBounds::fromAxisDirection(m_dirOrth1);
  auto boundDirOrth2 = CuboidVolumeBounds::fromAxisDirection(m_dirOrth2);

  const double maxHl1 =
      std::max_element(volumes.begin(), volumes.end(),
                       [boundDirOrth1](const auto& a, const auto& b) {
                         return a.bounds->get(boundDirOrth1) <
                                b.bounds->get(boundDirOrth1);
                       })
          ->bounds->get(boundDirOrth1);
  const double maxHl2 =
      std::max_element(volumes.begin(), volumes.end(),
                       [boundDirOrth2](const auto& a, const auto& b) {
                         return a.bounds->get(boundDirOrth2) <
                                b.bounds->get(boundDirOrth2);
                       })
          ->bounds->get(boundDirOrth2);
  ACTS_VERBOSE("Found: half length " << axisDirectionName(m_dirOrth1) << ":"
                                     << maxHl1 << ", half length "
                                     << axisDirectionName(m_dirOrth2) << ":"
                                     << maxHl2);

  for (auto& vt : volumes) {
    vt.set({
        {boundDirOrth1, maxHl1},
        {boundDirOrth2, maxHl2},
    });
  }

  return {maxHl1, maxHl2};
}

void CuboidVolumeStack::update(std::shared_ptr<VolumeBounds> volbounds,
                               std::optional<Transform3> transform,
                               const Logger& logger) {
  ACTS_DEBUG("Resizing CuboidVolumeStack with strategy: " << m_resizeStrategy);
  ACTS_DEBUG("Currently have " << m_volumes.size() << " children");
  ACTS_DEBUG(m_gaps.size() << " gaps");
  for (const auto& v : m_volumes) {
    ACTS_DEBUG(" - volume bounds: \n" << v->volumeBounds());
    ACTS_DEBUG("          transform: \n" << v->transform().matrix());
  }

  ACTS_DEBUG("New bounds are: \n" << *volbounds);

  auto bounds = std::dynamic_pointer_cast<CuboidVolumeBounds>(volbounds);
  if (bounds == nullptr) {
    throw std::invalid_argument(
        "CuboidVolumeStack requires CuboidVolumeBounds");
  }

  if (*bounds == volumeBounds()) {
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
  newVolume.updatedBounds = std::make_shared<CuboidVolumeBounds>(*bounds);
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

  constexpr auto tolerance = s_onSurfaceTolerance;
  auto same = [](double a, double b) { return std::abs(a - b) < tolerance; };

  for (const auto& dir : {m_dir, m_dirOrth1, m_dirOrth2}) {
    const double newMin = newVolume.min(dir);
    const double newMax = newVolume.max(dir);
    const double newMid = newVolume.mid(dir);
    const double newHl = newVolume.halfLength(dir);

    const double oldMin = oldVolume.min(dir);
    const double oldMax = oldVolume.max(dir);
    const double oldMid = oldVolume.mid(dir);
    const double oldHl = oldVolume.halfLength(dir);

    ACTS_VERBOSE("Previous bounds are: " << axisDirectionName(dir) << ": [ "
                                         << oldMin << " <- " << oldMid << " -> "
                                         << oldMax << " ] (" << oldHl << ")\n");
    ACTS_VERBOSE("New bounds are: " << axisDirectionName(dir) << ":      [ "
                                    << newMin << " <- " << newMid << " -> "
                                    << newMax << " ] (" << newHl << ")\n");

    if (!same(newMin, oldMin) && newMin > oldMin) {
      ACTS_ERROR("Shrinking the stack size in "
                 << axisDirectionName(dir) << " is not supported: " << newMin
                 << " -> " << oldMin);
      throw std::invalid_argument("Shrinking the stack in " +
                                  axisDirectionName(dir) + " is not supported");
    }

    if (!same(newMax, oldMax) && newMax < oldMax) {
      ACTS_ERROR("Shrinking the stack size in "
                 << axisDirectionName(dir) << " is not supported: " << newMax
                 << " -> " << oldMax);
      throw std::invalid_argument("Shrinking the stack in " +
                                  axisDirectionName(dir) + " is not supported");
    }
  }
  auto isGap = [this](const Volume* vol) {
    return std::ranges::any_of(
        m_gaps, [&](const auto& gap) { return vol == gap.get(); });
  };
  ACTS_VERBOSE("Stack direction is " << axisDirectionName(m_dir));

  std::vector<VolumeTuple> volumeTuples;
  volumeTuples.reserve(m_volumes.size());
  std::transform(m_volumes.begin(), m_volumes.end(),
                 std::back_inserter(volumeTuples), [this](const auto& volume) {
                   return VolumeTuple{*volume, m_groupTransform};
                 });

  ACTS_VERBOSE("*** Initial volume configuration:");
  printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);
  for (const auto& dir : {m_dirOrth1, m_dirOrth2}) {
    if (!same(newVolume.min(dir), oldVolume.min(dir)) ||
        !same(newVolume.max(dir), oldVolume.max(dir))) {
      ACTS_VERBOSE("Resize all volumes to new " << axisDirectionName(dir)
                                                << " bounds");
      for (auto& volume : volumeTuples) {
        volume.set({{CuboidVolumeBounds::fromAxisDirection(dir),
                     newVolume.halfLength(dir)}});
      }
      ACTS_VERBOSE("*** Volume configuration after " << axisDirectionName(dir)
                                                     << " resizing:");
      printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);
    } else {
      ACTS_VERBOSE(axisDirectionName(dir)
                   << " bounds are the same, no " << axisDirectionName(dir)
                   << " resize needed");
    }
  }

  if (same(newVolume.halfLength(m_dir), oldVolume.halfLength(m_dir))) {
    ACTS_VERBOSE("Halflength " << axisDirectionName(m_dir) << "is the same, no "
                               << axisDirectionName(m_dir) << "resize needed");
  } else {
    auto dirIdx = axisToIndex(m_dir);
    auto boundDirIdx = CuboidVolumeBounds::fromAxisDirection(m_dir);
    if (m_resizeStrategy == VolumeResizeStrategy::Expand) {
      if (newVolume.min(m_dir) < oldVolume.min(m_dir)) {
        ACTS_VERBOSE("Expanding first volume to new "
                     << axisDirectionName(m_dir) << "bounds");

        auto& first = volumeTuples.front();
        double newMinFirst = newVolume.min(m_dir);
        double newMidFirst = (newMinFirst + first.max(m_dir)) / 2.0;
        double newHlFirst = (first.max(m_dir) - newMinFirst) / 2.0;

        ACTS_VERBOSE(" -> first " << axisDirectionName(m_dir) << ": [ "
                                  << newMinFirst << " <- " << newMidFirst
                                  << " -> " << first.max(m_dir)
                                  << " ] (hl: " << newHlFirst << ")");

        Translation3 translation(Vector3::Unit(dirIdx) * newMidFirst);
        first.set({{boundDirIdx, newHlFirst}});
        first.setLocalTransform(Transform3{translation}, m_groupTransform);
      }

      if (newVolume.max(m_dir) > oldVolume.max(m_dir)) {
        ACTS_VERBOSE("Expanding last volume to new " << axisDirectionName(m_dir)
                                                     << " bounds");

        auto& last = volumeTuples.back();
        double newMaxLast = newVolume.max(m_dir);
        double newMidLast = (last.min(m_dir) + newMaxLast) / 2.0;
        double newHlLast = (newMaxLast - last.min(m_dir)) / 2.0;

        ACTS_VERBOSE(" -> last " << axisDirectionName(m_dir) << ": [ "
                                 << last.min(m_dir) << " <- " << newMidLast
                                 << " -> " << newMaxLast
                                 << " ] (hl: " << newHlLast << ")");

        Translation3 translation(Vector3::Unit(dirIdx) * newMidLast);
        last.set({{boundDirIdx, newHlLast}});
        last.setLocalTransform(Transform3{translation}, m_groupTransform);
      }
    } else if (m_resizeStrategy == VolumeResizeStrategy::Gap) {
      ACTS_VERBOSE("Creating gap volumes to fill the new "
                   << axisDirectionName(m_dir) << " bounds");

      auto printGapDimensions = [&](const VolumeTuple& gap,
                                    const std::string& prefix = "") {
        for (const auto& dir : {m_dir, m_dirOrth1, m_dirOrth2}) {
          ACTS_VERBOSE(" -> gap" << prefix << ": " << axisDirectionName(dir)
                                 << ": [ " << gap.min(m_dir) << " <- "
                                 << gap.mid(dir) << " -> " << gap.max(dir)
                                 << " ]");
        }
      };

      if (!same(newVolume.min(m_dir), oldVolume.min(m_dir)) &&
          newVolume.min(m_dir) < oldVolume.min(m_dir)) {
        double gap1Min = newVolume.min(m_dir);
        double gap1Max = oldVolume.min(m_dir);
        double gap1Hl = (gap1Max - gap1Min) / 2.0;
        double gap1P = (gap1Max + gap1Min) / 2.0;

        // check if we need a new gap volume or reuse an existing one
        auto& candidate = volumeTuples.front();
        if (isGap(candidate.volume)) {
          ACTS_VERBOSE("~> Reusing existing gap volume at negative "
                       << axisDirectionName(m_dir));

          gap1Hl = candidate.bounds->get(
                       CuboidVolumeBounds::fromAxisDirection(m_dir)) +
                   gap1Hl;
          gap1Max = gap1Min + gap1Hl * 2;
          gap1P = (gap1Max + gap1Min) / 2.0;

          printGapDimensions(candidate, " before");

          auto gap1Bounds = std::make_shared<CuboidVolumeBounds>(
              std::initializer_list<
                  std::pair<CuboidVolumeBounds::BoundValues, double>>{
                  {CuboidVolumeBounds::fromAxisDirection(m_dir), gap1Hl},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth1),
                   newVolume.halfLength(m_dirOrth1)},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth2),
                   newVolume.halfLength(m_dirOrth2)}});
          Translation3 gap1Translation(Vector3::Unit(dirIdx) * gap1P);
          Transform3 gap1Transform = m_groupTransform * gap1Translation;
          candidate.volume->update(std::move(gap1Bounds), gap1Transform);
          candidate = VolumeTuple{*candidate.volume, m_groupTransform};
          ACTS_VERBOSE("After:");
          printGapDimensions(candidate, " after ");

        } else {
          ACTS_VERBOSE("~> Creating new gap volume at negative ");
          auto gap1Bounds = std::make_shared<CuboidVolumeBounds>(
              std::initializer_list<
                  std::pair<CuboidVolumeBounds::BoundValues, double>>{
                  {CuboidVolumeBounds::fromAxisDirection(m_dir), gap1Hl},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth1),
                   newVolume.halfLength(m_dirOrth1)},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth2),
                   newVolume.halfLength(m_dirOrth2)}});
          Translation3 gap1Translation(Vector3::Unit(dirIdx) * gap1P);
          Transform3 gap1Transform = m_groupTransform * gap1Translation;
          auto gap1 = addGapVolume(gap1Transform, std::move(gap1Bounds));
          volumeTuples.insert(volumeTuples.begin(),
                              VolumeTuple{*gap1, m_groupTransform});
          printGapDimensions(volumeTuples.front());
        }
      }

      if (!same(newVolume.max(m_dir), oldVolume.max(m_dir)) &&
          newVolume.max(m_dir) > oldVolume.max(m_dir)) {
        double gap2Min = oldVolume.max(m_dir);
        double gap2Max = newVolume.max(m_dir);
        double gap2Hl = (gap2Max - gap2Min) / 2.0;
        double gap2P = (gap2Max + gap2Min) / 2.0;

        // check if we need a new gap volume or reuse an existing one
        auto& candidate = volumeTuples.back();
        if (isGap(candidate.volume)) {
          ACTS_VERBOSE("~> Reusing existing gap volume at positive ");

          gap2Hl = candidate.bounds->get(
                       CuboidVolumeBounds::fromAxisDirection(m_dir)) +
                   gap2Hl;
          gap2Min = newVolume.max(m_dir) - gap2Hl * 2;
          gap2P = (gap2Max + gap2Min) / 2.0;

          auto gap2Bounds = std::make_shared<CuboidVolumeBounds>(
              std::initializer_list<
                  std::pair<CuboidVolumeBounds::BoundValues, double>>{
                  {CuboidVolumeBounds::fromAxisDirection(m_dir), gap2Hl},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth1),
                   newVolume.halfLength(m_dirOrth1)},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth2),
                   newVolume.halfLength(m_dirOrth2)}});
          Translation3 gap2Translation(Vector3::Unit(dirIdx) * gap2P);
          Transform3 gap2Transform = m_groupTransform * gap2Translation;

          candidate.volume->update(std::move(gap2Bounds), gap2Transform);
          candidate = VolumeTuple{*candidate.volume, m_groupTransform};
          printGapDimensions(candidate, " after ");
        } else {
          ACTS_VERBOSE("~> Creating new gap volume at positive ");
          auto gap2Bounds = std::make_shared<CuboidVolumeBounds>(
              std::initializer_list<
                  std::pair<CuboidVolumeBounds::BoundValues, double>>{
                  {CuboidVolumeBounds::fromAxisDirection(m_dir), gap2Hl},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth1),
                   newVolume.halfLength(m_dirOrth1)},
                  {CuboidVolumeBounds::fromAxisDirection(m_dirOrth2),
                   newVolume.halfLength(m_dirOrth2)}});
          Translation3 gap2Translation(Vector3::Unit(dirIdx) * gap2P);
          Transform3 gap2Transform = m_groupTransform * gap2Translation;
          auto gap2 = addGapVolume(gap2Transform, std::move(gap2Bounds));
          volumeTuples.emplace_back(*gap2, m_groupTransform);
          printGapDimensions(volumeTuples.back());
        }
      }
    }

    ACTS_VERBOSE("*** Volume configuration after " << axisDirectionName(m_dir)
                                                   << " resizing:");
    printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);
  }

  ACTS_VERBOSE("Commit and update outer vector of volumes");
  m_volumes.clear();
  for (auto& vt : volumeTuples) {
    vt.commit(logger);
    m_volumes.push_back(vt.volume);
  }

  m_transform = newVolume.globalTransform;
  // @TODO: We probably can reuse m_transform
  m_groupTransform = m_transform;
  Volume::update(std::move(bounds), std::nullopt, logger);
}

std::shared_ptr<Volume> CuboidVolumeStack::addGapVolume(
    const Transform3& transform, const std::shared_ptr<VolumeBounds>& bounds) {
  auto gapVolume = std::make_shared<Volume>(transform, bounds);
  m_gaps.push_back(gapVolume);
  return gapVolume;
}

const std::vector<std::shared_ptr<Volume>>& CuboidVolumeStack::gaps() const {
  return m_gaps;
}

}  // namespace Acts
