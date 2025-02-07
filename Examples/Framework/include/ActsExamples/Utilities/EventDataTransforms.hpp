// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"

namespace ActsExamples {

ProtoTrack seedToPrototrack(const SimSeed &seed);

const SimSpacePoint *findSpacePointForIndex(
    Index index, const SimSpacePointContainer &spacepoints);

SimSeed prototrackToSeed(const ProtoTrack &track,
                         const SimSpacePointContainer &spacepoints);

}  // namespace ActsExamples
