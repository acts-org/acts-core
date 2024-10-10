// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/FpeMonitoring/FpeMonitor.hpp"
#include <Acts/Geometry/GeometryContext.hpp>
#include <Acts/MagneticField/MagneticFieldContext.hpp>
#include <Acts/Utilities/CalibrationContext.hpp>

namespace ActsExamples {

class WhiteBoard;

/// Aggregated information to run one algorithm over one event.
struct AlgorithmContext {
  /// @brief constructor with arguments
  ///
  /// @param alg is the algorithm/service/writer number
  /// @param event is the event number
  /// @param store is the event-wise event store
  ///
  /// @note the event dependent contexts are to be added by the
  /// Sequencer::m_decorators list
  AlgorithmContext(std::size_t alg, std::size_t event, WhiteBoard& store)
      : algorithmNumber(alg), eventNumber(event), eventStore(store) {}

  /// @brief ++operator overload to increase the algorithm number
  AlgorithmContext& operator++() {
    algorithmNumber += 1;
    return (*this);
  }

  std::size_t algorithmNumber;       ///< Unique algorithm identifier
  std::size_t eventNumber;           ///< Unique event identifier
  WhiteBoard& eventStore;            ///< Per-event data store
  Acts::GeometryContext geoContext;  ///< Per-event geometry context
  Acts::MagneticFieldContext
      magFieldContext;                    ///< Per-event magnetic Field context
  Acts::CalibrationContext calibContext;  ///< Per-event calibration context

  Acts::FpeMonitor* fpeMonitor = nullptr;
};

}  // namespace ActsExamples
