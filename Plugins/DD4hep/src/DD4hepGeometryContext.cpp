// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/DD4hep/DD4hepGeometryContext.hpp"

#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"

#include <iostream>

#include <nlohmann/json.hpp>

const Acts::Transform3& Acts::DD4hepGeometryContext::contextualTransform(
    const DD4hepDetectorElement& dElement) const {
  if (!this->isNominal()) {
    auto it = m_alignmentStore.find(Form("%lld", dElement.identifier()));
    if (it != m_alignmentStore.end()) {
      return it->second;
    } else {
      return dElement.nominalTransform(DD4hepGeometryContext());
    }
  } else {
    return dElement.nominalTransform(DD4hepGeometryContext());
  }
}

void Acts::DD4hepGeometryContext::setAlignmentStore(
    std::unordered_map<std::string, Transform3> alignmentStore) {
  m_alignmentStore = alignmentStore;
}
