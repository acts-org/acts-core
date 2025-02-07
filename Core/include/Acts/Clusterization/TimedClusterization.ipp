// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

namespace Acts::Ccl {

template <Acts::Ccl::HasRetrievableTimeInfo Cell, std::size_t N>
TimedConnect<Cell, N>::TimedConnect(double time) : timeTolerance(time) {}

template <Acts::Ccl::HasRetrievableTimeInfo Cell, std::size_t N>
TimedConnect<Cell, N>::TimedConnect(double time, bool commonCorner)
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
