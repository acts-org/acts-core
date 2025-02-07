// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
