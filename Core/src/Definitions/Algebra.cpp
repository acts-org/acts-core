// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"

#include <nlohmann/json.hpp>

void Acts::to_json(nlohmann::json& j, const Acts::Transform3& t) {
  auto translation = t.translation();
  if (translation != Acts::Vector3(0., 0., 0)) {
    std::array<double, 3> tdata = {translation.x(), translation.y(),
                                   translation.z()};
    j["translation"] = tdata;
  } else {
    j["translation"] = nlohmann::json();
  }

  auto rotation = t.rotation();
  if (rotation != Acts::RotationMatrix3::Identity()) {
    std::array<double, 9> rdata = {
        rotation(0, 0), rotation(0, 1), rotation(0, 2),
        rotation(1, 0), rotation(1, 1), rotation(1, 2),
        rotation(2, 0), rotation(2, 1), rotation(2, 2)};
    j["rotation"] = rdata;
  } else {
    j["rotation"] = nlohmann::json();
  }
}

void Acts::from_json(const nlohmann::json& j, Acts::Transform3& t) {
  t = Acts::Transform3::Identity();
  if (j.find("rotation") != j.end() && !j["rotation"].empty()) {
    std::array<double, 9> rdata = j["rotation"];
    Acts::RotationMatrix3 rot;
    rot << rdata[0], rdata[1], rdata[2], rdata[3], rdata[4], rdata[5], rdata[6],
        rdata[7], rdata[8];
    t.prerotate(rot);
  }
  if (j.find("translation") != j.end() && !j["translation"].empty()) {
    std::array<double, 3> tdata = j["translation"];
    t.pretranslate(Acts::Vector3(tdata[0], tdata[1], tdata[2]));
  }
}
