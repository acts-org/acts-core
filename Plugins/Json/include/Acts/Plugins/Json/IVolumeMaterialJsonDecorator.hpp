// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Json/ActsJson.hpp"

namespace Acts {
class ISurfaceMaterial;
class IVolumeMaterial;

/// helper class to add extra information to surface or volume json objects
class IVolumeMaterialJsonDecorator {
 public:
  virtual ~IVolumeMaterialJsonDecorator() = default;

  /// Add extra elements to the json object already filled for the given
  /// surface material
  ///
  /// @param material the surface material which was used to
  ///    fill the json object
  /// @param json the json object that is enhanced
  virtual void decorate(const Acts::ISurfaceMaterial &material,
                        nlohmann::json &json) const = 0;

  /// Add extra elements to the json object already filled for the given
  /// volume material
  ///
  /// @param material the volume material which was used to
  ///    fill the json object
  /// @param json the json object that is enhanced
  virtual void decorate(const Acts::IVolumeMaterial &material,
                        nlohmann::json &json) const = 0;
};
}  // namespace Acts
