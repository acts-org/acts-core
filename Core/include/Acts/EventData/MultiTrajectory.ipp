// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Utilities/AlgebraHelpers.hpp"

#include <bitset>
#include <cstdint>
#include <type_traits>
#include <vector>

#include <Eigen/Core>

namespace Acts {

template <typename D>
template <typename F>
void MultiTrajectory<D>::visitBackwards(IndexType iendpoint, F&& callable) const
  requires detail_lt::VisitorConcept<F, ConstTrackStateProxy>
{
  if (iendpoint == MultiTrajectoryTraits::kInvalid) {
    throw std::runtime_error(
        "Cannot visit backwards with kInvalid as endpoint");
  }

  while (true) {
    auto ts = getTrackState(iendpoint);
    if constexpr (std::is_same_v<std::invoke_result_t<F, ConstTrackStateProxy>,
                                 bool>) {
      bool proceed = callable(ts);
      // this point has no parent and ends the trajectory, or a break was
      // requested
      if (!proceed || !ts.hasPrevious()) {
        break;
      }
    } else {
      callable(ts);
      // this point has no parent and ends the trajectory
      if (!ts.hasPrevious()) {
        break;
      }
    }
    iendpoint = ts.previous();
  }
}

}  // namespace Acts
