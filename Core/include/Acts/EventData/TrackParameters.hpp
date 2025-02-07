// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/GenericCurvilinearTrackParameters.hpp"
#include "Acts/EventData/GenericFreeTrackParameters.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"

namespace Acts {

using SinglyChargedBoundTrackParameters =
    GenericBoundTrackParameters<SinglyChargedParticleHypothesis>;
using SinglyChargedCurvilinearTrackParameters =
    GenericCurvilinearTrackParameters<SinglyChargedParticleHypothesis>;
using SinglyChargedFreeTrackParameters =
    GenericFreeTrackParameters<SinglyChargedParticleHypothesis>;

using NeutralBoundTrackParameters =
    GenericBoundTrackParameters<NeutralParticleHypothesis>;
using NeutralCurvilinearTrackParameters =
    GenericCurvilinearTrackParameters<NeutralParticleHypothesis>;
using NeutralFreeTrackParameters =
    GenericFreeTrackParameters<NeutralParticleHypothesis>;

/// @brief BoundTrackParameters can hold any kind of charge
using BoundTrackParameters = GenericBoundTrackParameters<ParticleHypothesis>;
/// @brief CurvilinearTrackParameters can hold any kind of charge
using CurvilinearTrackParameters =
    GenericCurvilinearTrackParameters<ParticleHypothesis>;
/// @brief FreeTrackParameters can hold any kind of charge
using FreeTrackParameters = GenericFreeTrackParameters<ParticleHypothesis>;

}  // namespace Acts
