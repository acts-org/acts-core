// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/ContextualDetector/ExternalAlignmentDecorator.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "ActsExamples/ContextualDetector/ExternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

#include <cassert>
#include <ostream>
#include <thread>
#include <utility>

ActsExamples::Contextual::ExternalAlignmentDecorator::
    ExternalAlignmentDecorator(const Config& cfg,
                               std::unique_ptr<const Acts::Logger> logger)
    : m_cfg(cfg), m_logger(std::move(logger)) {
  if (m_cfg.trackingGeometry != nullptr) {
    // parse and populate
    parseGeometry(*m_cfg.trackingGeometry.get());
  }
}

ActsExamples::ProcessCode
ActsExamples::Contextual::ExternalAlignmentDecorator::decorate(
    AlgorithmContext& context) {
  // Iov map access needs to be synchronized
  std::lock_guard lock{m_iovMutex};

  // In which iov batch are we?
  unsigned int iov = context.eventNumber / m_cfg.iovSize;
  ACTS_VERBOSE("IOV handling in thread " << std::this_thread::get_id() << ".");
  ACTS_VERBOSE("IOV resolved to " << iov << " - from event "
                                  << context.eventNumber << ".");

  m_eventsSeen++;

  if (m_cfg.randomNumberSvc != nullptr) {
    if (auto it = m_activeIovs.find(iov); it != m_activeIovs.end()) {
      // Iov is already present, update last accessed
      it->second->lastAccessed = m_eventsSeen;
      context.geoContext =
          ExternallyAlignedDetectorElement::ContextType{it->second};
    } else {
      // Iov is not present yet, create it
      auto alignmentStore =
          std::make_unique<ExternallyAlignedDetectorElement::AlignmentStore>();
      alignmentStore->lastAccessed = m_eventsSeen;

      ACTS_VERBOSE("New IOV " << iov << " detected at event "
                              << context.eventNumber
                              << ", emulate new alignment.");

      // Create an algorithm local random number generator
      RandomEngine rng = m_cfg.randomNumberSvc->spawnGenerator(context);

      alignmentStore->transforms = m_nominalStore;  // copy nominal alignment
      for (auto& tForm : alignmentStore->transforms) {
        // Multiply alignment in place
        applyTransform(tForm, m_cfg, rng, iov);
      }

      auto [insertIterator, inserted] =
          m_activeIovs.emplace(iov, std::move(alignmentStore));
      assert(inserted && "Expected IOV to be created in map, but wasn't");

      // make context from iov pointer, address should be stable
      context.geoContext =
          ExternallyAlignedDetectorElement::ContextType{insertIterator->second};
    }
  }

  // Garbage collection
  if (m_cfg.doGarbageCollection) {
    for (auto it = m_activeIovs.begin(); it != m_activeIovs.end();) {
      auto& status = it->second;
      if (m_eventsSeen - status->lastAccessed > m_cfg.flushSize) {
        ACTS_DEBUG("IOV " << iov << " has not been accessed in the last "
                          << m_cfg.flushSize << " events, clearing");
        it = m_activeIovs.erase(it);
      } else {
        it++;
      }
    }
  }

  return ProcessCode::SUCCESS;
}

void ActsExamples::Contextual::ExternalAlignmentDecorator::parseGeometry(
    const Acts::TrackingGeometry& tGeometry) {
  // Double-visit - first count
  std::size_t nTransforms = 0;
  tGeometry.visitSurfaces([&nTransforms](const auto*) { ++nTransforms; });

  Acts::GeometryContext nominalCtx{
      ExternallyAlignedDetectorElement::ContextType{}};

  // Collect the surfacas into the nominal store
  std::vector<Acts::Transform3> aStore(nTransforms,
                                       Acts::Transform3::Identity());

  auto fillTransforms = [&aStore, &nominalCtx](const auto* surface) -> void {
    if (surface == nullptr) {
      throw std::invalid_argument("Surface is nullptr.");
    }
    auto alignableElement =
        dynamic_cast<const ExternallyAlignedDetectorElement*>(
            surface->associatedDetectorElement());
    if (alignableElement == nullptr) {
      throw std::invalid_argument("Surface is not alignable");
    }
    aStore[alignableElement->identifier()] = surface->transform(nominalCtx);
  };

  tGeometry.visitSurfaces(fillTransforms);
  m_nominalStore = std::move(aStore);
}
