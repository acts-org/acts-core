// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

namespace Acts::Ccl {

template <Acts::Ccl::HasRetrievableTimeInfo Cell, std::size_t N>
TimedConnect<Cell, N>::TimedConnect(Acts::ActsScalar time)
    : timeTolerance(time) {}

template <Acts::Ccl::HasRetrievableTimeInfo Cell, std::size_t N>
TimedConnect<Cell, N>::TimedConnect(Acts::ActsScalar time, bool commonCorner)
  requires(N == 2)
    : Acts::Ccl::DefaultConnect<Cell, N>(commonCorner), timeTolerance(time) {}

template <Acts::Ccl::HasRetrievableTimeInfo Cell, std::size_t N>
Acts::Ccl::ConnectResult TimedConnect<Cell, N>::operator()(
    const Cell& ref, const Cell& iter) const {
  Acts::Ccl::ConnectResult spaceCompatibility =
      Acts::Ccl::DefaultConnect<Cell, N>::operator()(ref, iter);
  if (spaceCompatibility != Acts::Ccl::ConnectResult::eConn) {
    return spaceCompatibility;
  }

  if (std::abs(getCellTime(ref) - getCellTime(iter)) < timeTolerance) {
    return Acts::Ccl::ConnectResult::eConn;
  }

  return Acts::Ccl::ConnectResult::eNoConn;
}

}  // namespace Acts::Ccl
