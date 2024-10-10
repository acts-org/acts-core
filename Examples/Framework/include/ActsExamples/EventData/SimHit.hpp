// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsFatras/EventData/Hit.hpp"

namespace ActsExamples {

using SimHit = ::ActsFatras::Hit;
/// Store hits ordered by geometry identifier.
using SimHitContainer = GeometryIdMultiset<SimHit>;

using HitParticlesMap = IndexMultimap<SimBarcode>;

using HitSimHitsMap = IndexMultimap<Index>;

}  // namespace ActsExamples
