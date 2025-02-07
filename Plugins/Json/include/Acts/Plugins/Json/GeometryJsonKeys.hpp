// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <string>

namespace Acts {

/// @struct jsonKey
///
/// @brief store in a single place the different key used for the material
/// mapping
struct jsonKey {
  /// The name identification
  std::string namekey = "NAME";
  /// The bin key
  std::string binkey = "binUtility";
  /// The material key
  std::string materialkey = "material";
  /// The local to global transformation key
  std::string transfokeys = "transformation";
  /// The type key -> proto, else
  std::string typekey = "type";
  /// The data key
  std::string datakey = "data";
  /// The mapping key, add surface to mapping procedure if true
  std::string mapkey = "mapMaterial";
  /// The mapping type key, used to select the type od material mapping for the
  /// surface
  std::string maptype = "mappingType";
};

}  // namespace Acts
