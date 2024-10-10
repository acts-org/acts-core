// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/ContextualDetector/InternalAlignmentDecorator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "ActsExamples/ContextualDetector/InternallyAlignedDetectorElement.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

#include <ostream>
#include <thread>
#include <utility>

ActsExamples::Contextual::InternalAlignmentDecorator::
    InternalAlignmentDecorator(const Config& cfg,
                               std::unique_ptr<const Acts::Logger> logger)
    : m_cfg(cfg), m_logger(std::move(logger)) {}

ActsExamples::ProcessCode
ActsExamples::Contextual::InternalAlignmentDecorator::decorate(
    AlgorithmContext& context) {
  // We need to lock the Decorator
  std::lock_guard<std::mutex> alignmentLock(m_alignmentMutex);

  // In which iov batch are we?
  unsigned int iov = context.eventNumber / m_cfg.iovSize;

  ACTS_VERBOSE("IOV handling in thread " << std::this_thread::get_id() << ".");
  ACTS_VERBOSE("IOV resolved to " << iov << " - from event "
                                  << context.eventNumber << ".");

  m_eventsSeen++;

  context.geoContext = InternallyAlignedDetectorElement::ContextType{iov};

  if (m_cfg.randomNumberSvc != nullptr) {
    if (auto it = m_activeIovs.find(iov); it != m_activeIovs.end()) {
      // Iov is already present, update last accessed
      it->second.lastAccessed = m_eventsSeen;
    } else {
      // Iov is not present yet, create it

      m_activeIovs.emplace(iov, IovStatus{m_eventsSeen});

      ACTS_VERBOSE("New IOV " << iov << " detected at event "
                              << context.eventNumber
                              << ", emulate new alignment.");

      // Create an algorithm local random number generator
      RandomEngine rng = m_cfg.randomNumberSvc->spawnGenerator(context);

      for (auto& lstore : m_cfg.detectorStore) {
        for (auto& ldet : lstore) {
          // get the nominal transform
          Acts::Transform3 tForm =
              ldet->nominalTransform(context.geoContext);  // copy
          // create a new transform
          applyTransform(tForm, m_cfg, rng, iov);
          // put it back into the store
          ldet->addAlignedTransform(tForm, iov);
        }
      }
    }
  }

  // Garbage collection
  if (m_cfg.doGarbageCollection) {
    for (auto it = m_activeIovs.begin(); it != m_activeIovs.end();) {
      unsigned int this_iov = it->first;
      auto& status = it->second;
      if (m_eventsSeen - status.lastAccessed > m_cfg.flushSize) {
        ACTS_DEBUG("IOV " << this_iov << " has not been accessed in the last "
                          << m_cfg.flushSize << " events, clearing");
        it = m_activeIovs.erase(it);
        for (auto& lstore : m_cfg.detectorStore) {
          for (auto& ldet : lstore) {
            ldet->clearAlignedTransform(this_iov);
          }
        }
      } else {
        it++;
      }
    }
  }

  return ProcessCode::SUCCESS;
}
