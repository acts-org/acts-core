// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "ActsExamples/EventData/Index.hpp"

#include <vector>

namespace ActsExamples {

/// A proto vertex is a collection of tracks identified by their indices.
using ProtoVertex = std::vector<Index>;
/// Container of proto vertices. Each proto vertex is identified by its index.
using ProtoVertexContainer = std::vector<ProtoVertex>;

}  // namespace ActsExamples
