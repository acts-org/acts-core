// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Clusterization/Clusterization.hpp"
#include "Acts/Definitions/Algebra.hpp"

#include <limits>

namespace Acts::Ccl {

template <typename Cell>
concept HasRetrievableTimeInfo = requires(Cell cell) {
  { getCellTime(cell) } -> std::same_as<Acts::ActsScalar>;
};

template <Acts::Ccl::HasRetrievableTimeInfo Cell, std::size_t N>
struct TimedConnect : public Acts::Ccl::DefaultConnect<Cell, N> {
  Acts::ActsScalar timeTolerance{std::numeric_limits<Acts::ActsScalar>::max()};

  TimedConnect() = default;
  TimedConnect(Acts::ActsScalar time);
  TimedConnect(Acts::ActsScalar time, bool commonCorner)
    requires(N == 2);
  ~TimedConnect() override = default;

  ConnectResult operator()(const Cell& ref, const Cell& iter) const override;
};

}  // namespace Acts::Ccl

#include "Acts/Clusterization/TimedClusterization.ipp"
