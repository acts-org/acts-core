// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/EventData/TrackParameters.hpp"

#include "Acts/EventData/TrackParametersConcept.hpp"

namespace Acts {

// ensure concrete classes satisfy the concepts

static_assert(BoundTrackParametersConcept<SinglyChargedBoundTrackParameters>);
static_assert(
    BoundTrackParametersConcept<SinglyChargedCurvilinearTrackParameters>);
static_assert(FreeTrackParametersConcept<SinglyChargedFreeTrackParameters>);

static_assert(BoundTrackParametersConcept<NeutralBoundTrackParameters>);
static_assert(BoundTrackParametersConcept<NeutralCurvilinearTrackParameters>);
static_assert(FreeTrackParametersConcept<NeutralFreeTrackParameters>);

static_assert(BoundTrackParametersConcept<BoundTrackParameters>);
static_assert(BoundTrackParametersConcept<CurvilinearTrackParameters>);
static_assert(FreeTrackParametersConcept<FreeTrackParameters>);

}  // namespace Acts
