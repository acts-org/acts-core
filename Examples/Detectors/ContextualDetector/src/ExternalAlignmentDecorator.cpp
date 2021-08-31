// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/ContextualDetector/ExternalAlignmentDecorator.hpp"

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "ActsExamples/ContextualDetector/ExternallyAlignedDetectorElement.hpp"

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
    } else {
      // Iov is not present yet, create it
      auto iovStatus = std::make_unique<IovStatus>();
      iovStatus->lastAccessed = m_eventsSeen;

      ACTS_VERBOSE("New IOV " << iov << " detected at event "
                              << context.eventNumber
                              << ", emulate new alignment.");

      // Create an algorithm local random number generator
      RandomEngine rng = m_cfg.randomNumberSvc->spawnGenerator(context);

      iovStatus->transforms = m_nominalStore;  // copy nominal alignment
      for (auto& tForm : iovStatus->transforms) {
        // Multiply alignment in place
        applyTransform(tForm, m_cfg, rng, iov);
      }

      auto [insertIterator, inserted] =
          m_activeIovs.emplace(iov, std::move(iovStatus));
      assert(inserted && "Expected IOV to be created in map, but wasn't");

      // make context from iov pointer, address should be stable
      context.geoContext = ExternallyAlignedDetectorElement::ContextType{
          &insertIterator->second->transforms};
    }
  }

  // Garbage collection
  for (auto it = m_activeIovs.begin(); it != m_activeIovs.end();) {
    auto& [iov, status] = *it;
    if (m_eventsSeen - status->lastAccessed > m_cfg.flushSize) {
      ACTS_DEBUG("IOV " << iov << " has not been accessed in the last "
                        << m_cfg.flushSize << " events, clearing");
      it = m_activeIovs.erase(it);
    } else {
      it++;
    }
  }

  return ProcessCode::SUCCESS;
}

void ActsExamples::Contextual::ExternalAlignmentDecorator::parseGeometry(
    const Acts::TrackingGeometry& tGeometry) {
  // Double-visit - first count
  size_t nTransforms = 0;
  tGeometry.visitSurfaces([&nTransforms](const auto*) { ++nTransforms; });

  Acts::GeometryContext nominalCtx{
      ExternallyAlignedDetectorElement::ContextType{}};

  // Collect the surfacas into the nominal store
  std::vector<Acts::Transform3> aStore(nTransforms,
                                       Acts::Transform3::Identity());

  auto fillTransforms = [&aStore, &nominalCtx](const auto* surface) -> void {
    auto alignableElement =
        dynamic_cast<const ExternallyAlignedDetectorElement*>(
            surface->associatedDetectorElement());
    aStore[alignableElement->identifier()] = surface->transform(nominalCtx);
  };

  tGeometry.visitSurfaces(fillTransforms);
  m_nominalStore = std::move(aStore);
}
