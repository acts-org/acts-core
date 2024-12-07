// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/AlgebraJsonConverter.hpp"

#include <array>
#include <stdexcept>

nlohmann::json Acts::Transform3JsonConverter::toJson(const Transform3& t,
                                                     const Options& options) {
  nlohmann::json jTransform;
  // Write out the translation
  auto translation = t.translation();
  if (translation != Acts::Vector3(0., 0., 0) || options.writeIdentity) {
    std::array<double, 3> tdata = {translation.x(), translation.y(),
                                   translation.z()};
    jTransform["translation"] = tdata;
  } else {
    jTransform["translation"] = nlohmann::json();
  }
  // Write out the rotation, could be transposed
  auto rotation = options.transpose ? t.rotation().transpose() : t.rotation();
  if (rotation != Acts::RotationMatrix3::Identity() || options.writeIdentity) {
    std::array<double, 9> rdata = {
        rotation(0, 0), rotation(0, 1), rotation(0, 2),
        rotation(1, 0), rotation(1, 1), rotation(1, 2),
        rotation(2, 0), rotation(2, 1), rotation(2, 2)};
    jTransform["rotation"] = rdata;
  } else {
    jTransform["rotation"] = nlohmann::json();
  }
  // Return the converted transform
  return jTransform;
}

Acts::Transform3 Acts::Transform3JsonConverter::fromJson(
    const nlohmann::json& jTransform) {
  Transform3 t = Transform3::Identity();
  Acts::from_json(jTransform, t);
  return t;
}
