// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Clusterization/Clusterization.hpp"
#include "Acts/Definitions/Algebra.hpp"

#include <limits>

namespace Acts::Ccl {

template <typename Cell>
concept HasRetrievableTimeInfo = requires(Cell cell) {
  { getCellTime(cell) } -> std::same_as<double>;
};

template <Acts::Ccl::HasRetrievableTimeInfo Cell, std::size_t N>
struct TimedConnect : public Acts::Ccl::DefaultConnect<Cell, N> {
  double timeTolerance{std::numeric_limits<double>::max()};

  TimedConnect() = default;
  TimedConnect(double time);
  TimedConnect(double time, bool commonCorner)
    requires(N == 2);
  ~TimedConnect() override = default;

  ConnectResult operator()(const Cell& ref, const Cell& iter) const override;
};

}  // namespace Acts::Ccl

#include "Acts/Clusterization/TimedClusterization.ipp"
