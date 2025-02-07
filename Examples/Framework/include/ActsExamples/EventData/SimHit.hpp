// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsFatras/EventData/Hit.hpp"

namespace ActsExamples {

using SimHit = ::ActsFatras::Hit;
/// Store hits ordered by geometry identifier.
using SimHitContainer = GeometryIdMultiset<SimHit>;

}  // namespace ActsExamples
