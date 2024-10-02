// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/SourceLink.hpp"

#include <concepts>

namespace ActsExamples {

template <typename T>
concept MeasurementConcept = requires(const T& m) {
  { m.size() } -> std::integral;
  { m.sourceLink() } -> Acts::Concepts::decayed_same_as<Acts::SourceLink>;
  { m.subspaceIndexVector() };
  { m.parameters() };
  { m.covariance() };
};
}  // namespace ActsExamples
