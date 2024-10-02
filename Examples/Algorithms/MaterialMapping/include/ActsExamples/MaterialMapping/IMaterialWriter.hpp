// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"

#include <map>
#include <memory>

namespace Acts {

class ISurfaceMaterial;
class IVolumeMaterial;

using SurfaceMaterialMap =
    std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>;

using VolumeMaterialMap =
    std::map<GeometryIdentifier, std::shared_ptr<const IVolumeMaterial>>;

using DetectorMaterialMaps = std::pair<SurfaceMaterialMap, VolumeMaterialMap>;
}  // namespace Acts

namespace ActsExamples {

/// @class IMaterialWriter
///
/// Interface definition for material writing
class IMaterialWriter {
 public:
  /// Virtual Destructor
  virtual ~IMaterialWriter() = default;

  /// The single writer class
  ///
  /// @param detMaterial the detector material maps
  virtual void writeMaterial(const Acts::DetectorMaterialMaps& detMaterial) = 0;
};

}  // namespace ActsExamples
