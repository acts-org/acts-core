// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
