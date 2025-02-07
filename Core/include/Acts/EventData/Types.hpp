// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"

#include <cstdint>
#include <limits>

namespace Acts {

using TrackIndexType = std::uint32_t;
static constexpr TrackIndexType kTrackIndexInvalid =
    std::numeric_limits<TrackIndexType>::max();

using ProjectorBitset = std::uint64_t;

template <std::size_t measdim>
using SubspaceIndices = std::array<std::uint8_t, measdim>;
using BoundSubspaceIndices = SubspaceIndices<eBoundSize>;
static constexpr BoundSubspaceIndices kBoundSubspaceIndicesInvalid = {
    eBoundSize, eBoundSize, eBoundSize, eBoundSize, eBoundSize, eBoundSize};
using SerializedSubspaceIndices = std::uint64_t;

}  // namespace Acts
