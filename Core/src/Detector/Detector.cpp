// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/Detector.hpp"

#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

Acts::Experimental::Detector::Detector(
    std::string name, std::vector<std::shared_ptr<DetectorVolume>> rootVolumes,
    ExternalNavigationDelegate detectorVolumeFinder)
    : m_name(std::move(name)),
      m_rootVolumes(std::move(rootVolumes)),
      m_volumeFinder(std::move(detectorVolumeFinder)) {
  if (m_rootVolumes.internal.empty()) {
    throw std::invalid_argument("Detector: no volume were given.");
  }
  if (!m_volumeFinder.connected()) {
    throw std::invalid_argument(
        "Detector: volume finder delegate is not connected.");
  }

  // Fill volumes
  auto collectVolumes = [&]() {
    std::vector<std::shared_ptr<DetectorVolume>> volumes;
    auto recurse = [&volumes](const std::shared_ptr<DetectorVolume>& volume,
                              auto& callback) -> void {
      volumes.push_back(volume);
      for (const auto& v : volume->volumePtrs()) {
        callback(v, callback);
      }
    };
    for (const auto& root : m_rootVolumes.internal) {
      recurse(root, recurse);
    }
    return volumes;
  };
  m_volumes = DetectorVolume::ObjectStore<std::shared_ptr<DetectorVolume>>(
      collectVolumes());

  // Fill the surface map
  std::unordered_map<GeometryIdentifier, const Surface*> surfaceGeoIdMap;
  // Map for the volume geometry id
  std::unordered_map<GeometryIdentifier, const DetectorVolume*> volumeGeoIdMap;

  // Check for unique names and fill the volume name / index map
  for (auto [iv, v] : enumerate(m_volumes.internal)) {
    // Assign this detector
    v->assignDetector(*this);
    // Close the portals
    v->closePortals();
    // Store the name
    const std::string vName = v->name();
    if (m_volumeNameIndex.contains(vName)) {
      throw std::invalid_argument("Detector: duplicate volume name " + vName +
                                  " detected.");
    }
    m_volumeNameIndex[vName] = iv;

    // ---------------------------------------------------------------
    // Check volume geometry id
    auto vgeoID = v->geometryId();
    // Check for undefined geometry id
    if (vgeoID.value() == 0u) {
      throw std::invalid_argument("Detector: volume '" + v->name() +
                                  "' with undefined geometry id detected" +
                                  ". Make sure a GeometryIdGenerator is used.");
    }
    if (volumeGeoIdMap.contains(vgeoID)) {
      std::stringstream ss;
      ss << vgeoID;
      throw std::invalid_argument("Detector: duplicate volume geometry id '" +
                                  ss.str() + "' detected" +
                                  ". Make sure a GeometryIdGenerator is used.");
    }
    volumeGeoIdMap.emplace(vgeoID, v.get());
    // ---------------------------------------------------------------

    for (const auto* s : v->surfaces()) {
      auto sgeoID = s->geometryId();

      // ---------------------------------------------------------------
      // Check for undefined geometry id
      if (sgeoID.value() == 0u) {
        std::stringstream ss;
        ss << s->name();
        throw std::invalid_argument(
            "Detector: surface '" + ss.str() + "' with undefined geometry id " +
            "detected in volume '" + v->name() +
            "'. Make sure a GeometryIdGenerator is used.");
      }
      // ---------------------------------------------------------------

      if (surfaceGeoIdMap.contains(sgeoID)) {
        std::stringstream ss;
        ss << sgeoID;
        throw std::invalid_argument(
            "Detector: duplicate sensitive surface geometry id '" + ss.str() +
            "' detected in volume '" + v->name() +
            "'. Make sure a GeometryIdGenerator is used.");
      }
      surfaceGeoIdMap.emplace(sgeoID, s);
    }
  }
  // Let us transfer the surfaces into the hierarchy map
  std::vector<std::pair<GeometryIdentifier, const Surface*>> surfaceGeoIdVec;
  surfaceGeoIdVec.reserve(surfaceGeoIdMap.size());
  for (auto [geoID, surface] : surfaceGeoIdMap) {
    surfaceGeoIdVec.emplace_back(geoID, surface);
  }
  m_sensitiveHierarchyMap =
      GeometryHierarchyMap<const Surface*>(std::move(surfaceGeoIdVec));
}

std::shared_ptr<Acts::Experimental::Detector>
Acts::Experimental::Detector::makeShared(
    std::string name, std::vector<std::shared_ptr<DetectorVolume>> rootVolumes,
    ExternalNavigationDelegate detectorVolumeFinder) {
  return std::shared_ptr<Detector>(
      new Detector(std::move(name), std::move(rootVolumes),
                   std::move(detectorVolumeFinder)));
}

std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>&
Acts::Experimental::Detector::rootVolumePtrs() {
  return m_rootVolumes.internal;
}

const std::vector<const Acts::Experimental::DetectorVolume*>&
Acts::Experimental::Detector::rootVolumes() const {
  return m_rootVolumes.external;
}

std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>&
Acts::Experimental::Detector::volumePtrs() {
  return m_volumes.internal;
}

const std::vector<const Acts::Experimental::DetectorVolume*>&
Acts::Experimental::Detector::volumes() const {
  return m_volumes.external;
}

void Acts::Experimental::Detector::updateDetectorVolumeFinder(
    ExternalNavigationDelegate detectorVolumeFinder) {
  m_volumeFinder = std::move(detectorVolumeFinder);
}

const Acts::Experimental::ExternalNavigationDelegate&
Acts::Experimental::Detector::detectorVolumeFinder() const {
  return m_volumeFinder;
}

const std::string& Acts::Experimental::Detector::name() const {
  return m_name;
}

std::shared_ptr<Acts::Experimental::Detector>
Acts::Experimental::Detector::getSharedPtr() {
  return shared_from_this();
}

std::shared_ptr<const Acts::Experimental::Detector>
Acts::Experimental::Detector::getSharedPtr() const {
  return shared_from_this();
}

void Acts::Experimental::Detector::updateDetectorVolume(
    const GeometryContext& gctx, NavigationState& nState) const {
  m_volumeFinder(gctx, nState);
}

const Acts::Experimental::DetectorVolume*
Acts::Experimental::Detector::findDetectorVolume(
    const GeometryContext& gctx, const Vector3& position) const {
  NavigationState nState;
  nState.currentDetector = this;
  nState.position = position;
  m_volumeFinder(gctx, nState);
  return nState.currentVolume;
}

const Acts::Experimental::DetectorVolume*
Acts::Experimental::Detector::findDetectorVolume(
    const std::string& name) const {
  auto vCandidate = m_volumeNameIndex.find(name);
  if (vCandidate != m_volumeNameIndex.end()) {
    return m_volumes.external[vCandidate->second];
  }
  return nullptr;
}

const Acts::GeometryHierarchyMap<const Acts::Surface*>&
Acts::Experimental::Detector::sensitiveHierarchyMap() const {
  return m_sensitiveHierarchyMap;
}

const Acts::Surface* Acts::Experimental::Detector::findSurface(
    GeometryIdentifier geoID) const {
  return *m_sensitiveHierarchyMap.find(geoID);
}
