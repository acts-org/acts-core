// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/EventData/Index.hpp"

#include <vector>

#include <boost/container/small_vector.hpp>

namespace ActsExamples {

/// A proto track is a collection of hits identified by their indices.
using ProtoTrack = boost::container::small_vector<Index, 3>;
/// Container of proto tracks. Each proto track is identified by its index.
using ProtoTrackContainer = std::vector<ProtoTrack>;

}  // namespace ActsExamples
