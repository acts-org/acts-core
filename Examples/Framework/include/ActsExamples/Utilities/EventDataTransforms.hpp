// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
