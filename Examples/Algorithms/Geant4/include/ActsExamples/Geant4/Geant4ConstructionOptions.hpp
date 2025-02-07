// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>
#include <vector>

namespace ActsExamples::Geant4 {
class RegionCreator;
}  // namespace ActsExamples::Geant4

namespace ActsExamples {

struct Geant4ConstructionOptions {
  std::vector<std::shared_ptr<Geant4::RegionCreator>> regionCreators;
};

}  // namespace ActsExamples
