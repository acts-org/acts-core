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

  ActsScalar midZ() const { return localTransform.translation()[eZ]; }
  ActsScalar halfLengthZ() const {
    return updatedBounds->get(CylinderVolumeBounds::eHalfLengthZ);
  }
  ActsScalar minZ() const { return midZ() - halfLengthZ(); }
  ActsScalar maxZ() const { return midZ() + halfLengthZ(); }

  ActsScalar minR() const {
    return updatedBounds->get(CylinderVolumeBounds::eMinR);
  }
  ActsScalar maxR() const {
    return updatedBounds->get(CylinderVolumeBounds::eMaxR);
  }
  ActsScalar midR() const { return (minR() + maxR()) / 2.0; }

  void set(std::initializer_list<
           std::pair<CylinderVolumeBounds::BoundValues, ActsScalar>>
               keyValues) {
    updatedBounds->set(keyValues);
  }

  void setLocalTransform(const Transform3& transform,
                         const Transform3& groupTransform) {
    localTransform = transform;
    globalTransform = groupTransform * localTransform;
    transformDirty = true;
  }

  void commit() {
    // make a copy so we can't accidentally modify in-place
    auto copy = std::make_shared<CylinderVolumeBounds>(*updatedBounds);

    std::optional<Transform3> transform = std::nullopt;
    if (transformDirty) {
      transform = globalTransform;
    }

    volume->update(std::move(updatedBounds), transform);
    bounds = copy.get();
    updatedBounds = std::move(copy);
    transformDirty = false;
  }
};

CylinderVolumeStack::CylinderVolumeStack(std::vector<Volume*>& volumes,
                                         BinningValue direction,
                                         AttachmentStrategy strategy,
                                         ResizeStrategy resizeStrategy,
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

void CylinderVolumeStack::initializeOuterVolume(BinningValue direction,
                                                AttachmentStrategy strategy,
                                                const Logger& logger) {
  ACTS_DEBUG("Creating CylinderVolumeStack from "
             << m_volumes.size() << " volumes in direction "
             << binningValueName(direction));
  if (m_volumes.empty()) {
    throw std::invalid_argument(
        "CylinderVolumeStack requires at least one volume");
  }

  if (direction != Acts::BinningValue::binZ &&
      direction != Acts::BinningValue::binR) {
    throw std::invalid_argument(binningValueName(direction) +
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
    Volume::update(std::make_shared<CylinderVolumeBounds>(*cylBounds));
    return;
  }

  ACTS_VERBOSE("Checking volume alignment");
  checkVolumeAlignment(volumeTuples, logger);

  if (direction == Acts::BinningValue::binZ) {
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

      vt.commit();
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

    ActsScalar minZ = volumeTuples.front().minZ();
    ActsScalar maxZ = volumeTuples.back().maxZ();

    ActsScalar midZ = (minZ + maxZ) / 2.0;
    ActsScalar hlZ = (maxZ - minZ) / 2.0;

    m_transform = m_groupTransform * Translation3{0, 0, midZ};

    Volume::update(std::make_shared<CylinderVolumeBounds>(minR, maxR, hlZ));
    ACTS_DEBUG("Outer bounds are:\n" << volumeBounds());
    ACTS_DEBUG("Outer transform / new group transform is:\n"
               << m_transform.matrix());

    // Update group transform to the new center
    // @TODO: We probably can reuse m_transform
    m_groupTransform = m_transform;

  } else if (direction == Acts::BinningValue::binR) {
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
      vt.commit();
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

    ActsScalar minR = volumeTuples.front().minR();
    ActsScalar maxR = volumeTuples.back().maxR();

    ActsScalar midZ = (minZ + maxZ) / 2.0;
    ActsScalar hlZ = (maxZ - minZ) / 2.0;

    m_transform = m_groupTransform * Translation3{0, 0, midZ};

    Volume::update(std::make_shared<CylinderVolumeBounds>(minR, maxR, hlZ));

    ACTS_DEBUG("Outer bounds are:\n" << volumeBounds());
    ACTS_DEBUG("Outer transform is:\n" << m_transform.matrix());

    // Update group transform to the new center
    // @TODO: We probably can reuse m_transform
    m_groupTransform = m_transform;

  } else {
    ACTS_ERROR("Binning in " << binningValueName(direction)
                             << " is not supported");
    throw std::invalid_argument(binningValueName(direction) +
                                " is not supported ");
  }
}

void CylinderVolumeStack::overlapPrint(
    const CylinderVolumeStack::VolumeTuple& a,
    const CylinderVolumeStack::VolumeTuple& b, const Logger& logger) {
  if (logger().doPrint(Acts::Logging::DEBUG)) {
    std::stringstream ss;
    ss << std::fixed;
    ss << std::setprecision(3);
    ss << std::setfill(' ');
    ACTS_VERBOSE("Checking overlap between");
    int w = 9;
    ss << " - "
       << " z: [ " << std::setw(w) << a.minZ() << " <- " << std::setw(w)
       << a.midZ() << " -> " << std::setw(w) << a.maxZ() << " ]";
    ACTS_VERBOSE(ss.str());

    ss.str("");
    ss << " - "
       << " z: [ " << std::setw(w) << b.minZ() << " <- " << std::setw(w)
       << b.midZ() << " -> " << std::setw(w) << b.maxZ() << " ]";
    ACTS_VERBOSE(ss.str());
  }
}

std::vector<CylinderVolumeStack::VolumeTuple>
CylinderVolumeStack::checkOverlapAndAttachInZ(
    std::vector<VolumeTuple>& volumes,
    CylinderVolumeStack::AttachmentStrategy strategy, const Logger& logger) {
  // Preconditions: volumes are sorted by z
  std::vector<VolumeTuple> gapVolumes;
  for (std::size_t i = 0; i < volumes.size() - 1; i++) {
    std::size_t j = i + 1;
    auto& a = volumes.at(i);
    auto& b = volumes.at(j);

    overlapPrint(a, b, logger);

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
      ActsScalar gapWidth = b.minZ() - a.maxZ();
      ACTS_VERBOSE("Gap width: " << gapWidth);

      ACTS_VERBOSE("Synchronizing bounds in z with strategy: " << strategy);
      switch (strategy) {
        case AttachmentStrategy::Midpoint: {
          ACTS_VERBOSE(" -> Strategy: Expand both volumes to midpoint");

          ActsScalar aZMidNew = (a.minZ() + a.maxZ()) / 2.0 + gapWidth / 4.0;
          ActsScalar aHlZNew = a.halfLengthZ() + gapWidth / 4.0;
          ACTS_VERBOSE("  - New halflength for first volume: " << aHlZNew);
          ACTS_VERBOSE("  - New bounds for first volume: ["
                       << (aZMidNew - aHlZNew) << " <- " << aZMidNew << " -> "
                       << (aZMidNew + aHlZNew) << "]");

          assert(a.minZ() == aZMidNew - aHlZNew && "Volume shrunk");
          assert(a.maxZ() <= aZMidNew + aHlZNew && "Volume shrunk");

          ActsScalar bZMidNew = (b.minZ() + b.maxZ()) / 2.0 - gapWidth / 4.0;
          ActsScalar bHlZNew = b.halfLengthZ() + gapWidth / 4.0;
          ACTS_VERBOSE("  - New halflength for second volume: " << bHlZNew);
          ACTS_VERBOSE("  - New bounds for second volume: ["
                       << (bZMidNew - bHlZNew) << " <- " << bZMidNew << " -> "
                       << (bZMidNew + bHlZNew) << "]");

          assert(b.minZ() >= bZMidNew - bHlZNew && "Volume shrunk");
          assert(b.maxZ() == bZMidNew + bHlZNew && "Volume shrunk");

          a.localTransform = Translation3{0, 0, aZMidNew};
          a.volume->setTransform(m_groupTransform * a.localTransform);
          a.updatedBounds->set(CylinderVolumeBounds::eHalfLengthZ, aHlZNew);

          b.localTransform = Translation3{0, 0, bZMidNew};
          b.volume->setTransform(m_groupTransform * b.localTransform);
          b.updatedBounds->set(CylinderVolumeBounds::eHalfLengthZ, bHlZNew);

          break;
        }
        case AttachmentStrategy::First: {
          ACTS_VERBOSE(" -> Strategy: Expand first volume");
          ActsScalar aZMidNew = (a.minZ() + b.minZ()) / 2.0;
          ActsScalar aHlZNew = (b.minZ() - a.minZ()) / 2.0;
          ACTS_VERBOSE("  - Gap width: " << gapWidth);
          ACTS_VERBOSE("  - New bounds for first volume: ["
                       << (aZMidNew - aHlZNew) << " <- " << aZMidNew << " -> "
                       << (aZMidNew + aHlZNew) << "]");

          assert(a.minZ() == aZMidNew - aHlZNew && "Volume shrunk");
          assert(a.maxZ() <= aZMidNew + aHlZNew && "Volume shrunk");

          a.localTransform = Translation3{0, 0, aZMidNew};
          a.volume->setTransform(m_groupTransform * a.localTransform);
          a.updatedBounds->set(CylinderVolumeBounds::eHalfLengthZ, aHlZNew);

          break;
        }
        case AttachmentStrategy::Second: {
          ACTS_VERBOSE(" -> Strategy: Expand second volume");
          ActsScalar bZMidNew = (a.maxZ() + b.maxZ()) / 2.0;
          ActsScalar bHlZNew = (b.maxZ() - a.maxZ()) / 2.0;
          ACTS_VERBOSE("  - New halflength for second volume: " << bHlZNew);
          ACTS_VERBOSE("  - New bounds for second volume: ["
                       << (bZMidNew - bHlZNew) << " <- " << bZMidNew << " -> "
                       << (bZMidNew + bHlZNew) << "]");

          assert(b.minZ() >= bZMidNew - bHlZNew && "Volume shrunk");
          assert(b.maxZ() == bZMidNew + bHlZNew && "Volume shrunk");

          b.localTransform = Translation3{0, 0, bZMidNew};
          b.volume->setTransform(m_groupTransform * b.localTransform);
          b.updatedBounds->set(CylinderVolumeBounds::eHalfLengthZ, bHlZNew);
          break;
        }
        case AttachmentStrategy::Gap: {
          ACTS_VERBOSE(" -> Strategy: Create a gap volume");
          ActsScalar gapHlZ = (b.minZ() - a.maxZ()) / 2.0;
          ActsScalar gapMidZ = (b.minZ() + a.maxZ()) / 2.0;

          ACTS_VERBOSE("  - Gap half length: " << gapHlZ
                                               << " at z: " << gapMidZ);

          Transform3 gapLocalTransform{Translation3{0, 0, gapMidZ}};
          Transform3 gapGlobalTransform = m_groupTransform * gapLocalTransform;
          auto gapBounds = std::make_shared<CylinderVolumeBounds>(
              a.minR(), b.maxR(), gapHlZ);

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
CylinderVolumeStack::checkOverlapAndAttachInR(
    std::vector<VolumeTuple>& volumes,
    CylinderVolumeStack::AttachmentStrategy strategy, const Logger& logger) {
  std::vector<VolumeTuple> gapVolumes;
  for (std::size_t i = 0; i < volumes.size() - 1; i++) {
    std::size_t j = i + 1;
    auto& a = volumes.at(i);
    auto& b = volumes.at(j);

    overlapPrint(a, b, logger);

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
      ActsScalar gapWidth = b.minR() - a.maxR();
      ACTS_VERBOSE("Gap width: " << gapWidth);

      ACTS_VERBOSE("Synchronizing bounds in r with strategy: " << strategy);
      switch (strategy) {
        case AttachmentStrategy::Midpoint: {
          ACTS_VERBOSE(" -> Strategy: Expand both volumes to midpoint");

          a.set({{CylinderVolumeBounds::eMaxR, a.maxR() + gapWidth / 2.0}});
          b.set({{CylinderVolumeBounds::eMinR, b.minR() - gapWidth / 2.0}});

          break;
        }
        case AttachmentStrategy::First: {
          ACTS_VERBOSE(" -> Strategy: Expand first volume");

          a.set({{CylinderVolumeBounds::eMaxR, b.minR()}});

          break;
        }
        case AttachmentStrategy::Second: {
          ACTS_VERBOSE(" -> Strategy: Expand second volume");

          b.set({{CylinderVolumeBounds::eMinR, a.maxR()}});

          break;
        }
        case AttachmentStrategy::Gap: {
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

std::pair<ActsScalar, ActsScalar> CylinderVolumeStack::synchronizeRBounds(
    std::vector<VolumeTuple>& volumes, const Logger& logger) {
  const ActsScalar minR =
      std::min_element(volumes.begin(), volumes.end(),
                       [](const auto& a, const auto& b) {
                         return a.bounds->get(CylinderVolumeBounds::eMinR) <
                                b.bounds->get(CylinderVolumeBounds::eMinR);
                       })
          ->bounds->get(CylinderVolumeBounds::eMinR);

  const ActsScalar maxR =
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

std::pair<ActsScalar, ActsScalar> CylinderVolumeStack::synchronizeZBounds(
    std::vector<VolumeTuple>& volumes, const Logger& logger) {
  const ActsScalar minZ = std::min_element(volumes.begin(), volumes.end(),
                                           [](const auto& a, const auto& b) {
                                             return a.minZ() < b.minZ();
                                           })
                              ->minZ();

  const ActsScalar maxZ = std::max_element(volumes.begin(), volumes.end(),
                                           [](const auto& a, const auto& b) {
                                             return a.maxZ() < b.maxZ();
                                           })
                              ->maxZ();
  const ActsScalar midZ = (minZ + maxZ) / 2.0;
  const ActsScalar hlZ = (maxZ - minZ) / 2.0;
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
                                 std::optional<Transform3> transform) {
  if (volbounds == nullptr) {
    throw std::invalid_argument("New bounds are nullptr");
  }
  auto cylBounds = std::dynamic_pointer_cast<CylinderVolumeBounds>(volbounds);
  if (cylBounds == nullptr) {
    throw std::invalid_argument(
        "CylinderVolumeStack requires CylinderVolumeBounds");
  }
  update(std::move(cylBounds), transform,
         *Acts::getDefaultLogger("CYLSTACK", Logging::VERBOSE));
}

void CylinderVolumeStack::update(
    std::shared_ptr<CylinderVolumeBounds> newBounds,
    std::optional<Transform3> transform, const Logger& logger) {
  ACTS_DEBUG(
      "Resizing CylinderVolumeStack with strategy: " << m_resizeStrategy);

  if (newBounds == nullptr) {
    throw std::invalid_argument("New bounds are nullptr");
  }

  if (*newBounds == volumeBounds()) {
    ACTS_VERBOSE("Bounds are the same, no resize needed");
    return;
  }

  if (transform.has_value()) {
    ACTS_VERBOSE(transform.value().matrix());
  }

  VolumeTuple oldVolume{*this, m_transform};
  VolumeTuple newVolume{*this, m_transform};
  newVolume.updatedBounds = std::make_shared<CylinderVolumeBounds>(*newBounds);
  newVolume.globalTransform = transform.value_or(m_transform);
  newVolume.localTransform =
      m_groupTransform.inverse() * newVolume.globalTransform;

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

  checkNoPhiOrBevel(*newBounds, logger);

  const ActsScalar newMinR = newVolume.minR();
  const ActsScalar newMaxR = newVolume.maxR();
  const ActsScalar newMinZ = newVolume.minZ();
  const ActsScalar newMaxZ = newVolume.maxZ();
  const ActsScalar newMidZ = newVolume.midZ();
  const ActsScalar newHlZ = newVolume.halfLengthZ();

  const ActsScalar oldMinR = oldVolume.minR();
  const ActsScalar oldMaxR = oldVolume.maxR();
  const ActsScalar oldMinZ = oldVolume.minZ();
  const ActsScalar oldMaxZ = oldVolume.maxZ();
  const ActsScalar oldMidZ = oldVolume.midZ();
  const ActsScalar oldHlZ = oldVolume.halfLengthZ();

  ACTS_VERBOSE("Previous bounds are: z: [ "
               << oldMinZ << " <- " << oldMidZ << " -> " << oldMaxZ
               << " ], r: [ " << oldMinR << " <-> " << oldMaxR << " ]");
  ACTS_VERBOSE("New bounds are: z:      [ "
               << newMinZ << " <- " << newMidZ << " -> " << newMaxZ
               << " ], r: [ " << newMinR << " <-> " << newMaxR << " ]");

  if (newMinZ > oldMinZ) {
    ACTS_ERROR("Shrinking the stack size in z is not supported");
    throw std::invalid_argument("Shrinking the stack is not supported");
  }

  if (newMaxZ < oldMaxZ) {
    ACTS_ERROR("Shrinking the stack size in z is not supported");
    throw std::invalid_argument("Shrinking the stack is not supported");
  }

  if (newMinR > oldMinR) {
    ACTS_ERROR("Shrinking the stack size in r is not supported");
    throw std::invalid_argument("Shrinking the stack is not supported");
  }

  if (newMaxR < oldMaxR) {
    ACTS_ERROR("Shrinking the stack size in r is not supported");
    throw std::invalid_argument("Shrinking the stack is not supported");
  }

  if (m_direction == BinningValue::binZ) {
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

    if (newMinR != oldMinR || newMaxR != oldMaxR) {
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

    if (newHlZ == oldHlZ) {
      ACTS_VERBOSE("Halflength z is the same, no z resize needed");
    } else {
      if (m_resizeStrategy == ResizeStrategy::Expand) {
        if (newMinZ < oldMinZ) {
          ACTS_VERBOSE("Expanding first volume to new z bounds");

          auto& first = volumeTuples.front();
          ActsScalar newMinZFirst = newVolume.minZ();
          ActsScalar newMidZFirst = (newMinZFirst + first.maxZ()) / 2.0;
          ActsScalar newHlZFirst = (first.maxZ() - newMinZFirst) / 2.0;

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
          ActsScalar newMaxZLast = newVolume.maxZ();
          ActsScalar newMidZLast = (last.minZ() + newMaxZLast) / 2.0;
          ActsScalar newHlZLast = (newMaxZLast - last.minZ()) / 2.0;

          ACTS_VERBOSE(" -> last z: [ " << last.minZ() << " <- " << newMidZLast
                                        << " -> " << newMaxZLast
                                        << " ] (hl: " << newHlZLast << ")");

          last.set({{CylinderVolumeBounds::eHalfLengthZ, newHlZLast}});
          last.setLocalTransform(Transform3{Translation3{0, 0, newMidZLast}},
                                 m_groupTransform);
        }
      } else if (m_resizeStrategy == ResizeStrategy::Gap) {
        ACTS_VERBOSE("Creating gap volumes to fill the new z bounds");

        if (newMinZ < oldMinZ) {
          ACTS_VERBOSE("Adding gap volume at negative z");

          ActsScalar gap1MinZ = newVolume.minZ();
          ActsScalar gap1MaxZ = oldVolume.minZ();
          ActsScalar gap1HlZ = (gap1MaxZ - gap1MinZ) / 2.0;
          ActsScalar gap1PZ = (gap1MaxZ + gap1MinZ) / 2.0;

          ACTS_VERBOSE(" -> gap1 z: [ " << gap1MinZ << " <- " << gap1PZ
                                        << " -> " << gap1MaxZ
                                        << " ] (hl: " << gap1HlZ << ")");

          auto gap1Bounds =
              std::make_shared<CylinderVolumeBounds>(newMinR, newMaxR, gap1HlZ);
          auto gap1Transform = m_groupTransform * Translation3{0, 0, gap1PZ};
          auto gap1 = addGapVolume(gap1Transform, std::move(gap1Bounds));
          volumeTuples.insert(volumeTuples.begin(),
                              VolumeTuple{*gap1, m_groupTransform});
        }

        if (newMaxZ > oldMaxZ) {
          ACTS_VERBOSE("Adding gap volume at positive z");

          ActsScalar gap2MinZ = oldVolume.maxZ();
          ActsScalar gap2MaxZ = newVolume.maxZ();
          ActsScalar gap2HlZ = (gap2MaxZ - gap2MinZ) / 2.0;
          ActsScalar gap2PZ = (gap2MaxZ + gap2MinZ) / 2.0;

          ACTS_VERBOSE(" -> gap2 z: [ " << gap2MinZ << " <- " << gap2PZ
                                        << " -> " << gap2MaxZ
                                        << " ] (hl: " << gap2HlZ << ")");

          auto gap2Bounds =
              std::make_shared<CylinderVolumeBounds>(newMinR, newMaxR, gap2HlZ);
          auto gap2Transform = m_groupTransform * Translation3{0, 0, gap2PZ};
          auto gap2 = addGapVolume(gap2Transform, std::move(gap2Bounds));
          volumeTuples.emplace_back(*gap2, m_groupTransform);
        }
      }

      ACTS_VERBOSE("*** Volume configuration after z resizing:");
      printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);
    }

    ACTS_VERBOSE("Commit and update outer vector of volumes");
    m_volumes.clear();
    for (auto& vt : volumeTuples) {
      vt.commit();
      m_volumes.push_back(vt.volume);
    }

  } else if (m_direction == BinningValue::binR) {
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
      if (m_resizeStrategy == ResizeStrategy::Expand) {
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
      } else if (m_resizeStrategy == ResizeStrategy::Gap) {
        if (oldMinR > newMinR) {
          ACTS_VERBOSE("Creating gap volume at the innermost end");
          auto gapBounds =
              std::make_shared<CylinderVolumeBounds>(newMinR, oldMinR, newHlZ);
          auto gapTransform = m_groupTransform;
          auto gapVolume = addGapVolume(gapTransform, gapBounds);
          volumeTuples.insert(volumeTuples.begin(),
                              VolumeTuple{*gapVolume, m_groupTransform});
          auto gap = volumeTuples.front();
          ACTS_VERBOSE(" -> gap inner: [ "
                       << gap.minZ() << " <- " << gap.midZ() << " -> "
                       << gap.maxZ() << " ], r: [ " << gap.minR() << " <-> "
                       << gap.maxR() << " ]");
        }
        if (oldMaxR < newMaxR) {
          ACTS_VERBOSE("Creating gap volume at the outermost end");
          auto gapBounds =
              std::make_shared<CylinderVolumeBounds>(oldMaxR, newMaxR, newHlZ);
          auto gapTransform = m_groupTransform;
          auto gapVolume = addGapVolume(gapTransform, gapBounds);
          volumeTuples.emplace_back(*gapVolume, m_groupTransform);
          auto gap = volumeTuples.back();
          ACTS_VERBOSE(" -> gap outer: [ "
                       << gap.minZ() << " <- " << gap.midZ() << " -> "
                       << gap.maxZ() << " ], r: [ " << gap.minR() << " <-> "
                       << gap.maxR() << " ]");
        }
      }

      ACTS_VERBOSE("*** Volume configuration after r resizing:");
      printVolumeSequence(volumeTuples, logger, Acts::Logging::DEBUG);
    }

    ACTS_VERBOSE("Commit and update outer vector of volumes");
    m_volumes.clear();
    for (auto& vt : volumeTuples) {
      vt.commit();
      m_volumes.push_back(vt.volume);
    }
  }

  m_transform = newVolume.globalTransform;
  Volume::update(std::move(newBounds));
}

void CylinderVolumeStack::checkNoPhiOrBevel(const CylinderVolumeBounds& bounds,
                                            const Logger& logger) {
  if (bounds.get(CylinderVolumeBounds::eHalfPhiSector) != M_PI) {
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

std::ostream& operator<<(std::ostream& os,
                         CylinderVolumeStack::AttachmentStrategy strategy) {
  switch (strategy) {
    case CylinderVolumeStack::AttachmentStrategy::First:
      os << "First";
      break;
    case CylinderVolumeStack::AttachmentStrategy::Second:
      os << "Second";
      break;
    case CylinderVolumeStack::AttachmentStrategy::Midpoint:
      os << "Midpoint";
      break;
    case CylinderVolumeStack::AttachmentStrategy::Gap:
      os << "Gap";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         CylinderVolumeStack::ResizeStrategy strategy) {
  switch (strategy) {
    case CylinderVolumeStack::ResizeStrategy::Expand:
      os << "Expand";
      break;
    case CylinderVolumeStack::ResizeStrategy::Gap:
      os << "Gap";
      break;
  }
  return os;
}

}  // namespace Acts
