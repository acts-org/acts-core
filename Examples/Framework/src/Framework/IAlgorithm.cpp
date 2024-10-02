// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Framework/IAlgorithm.hpp"

#include "Acts/Utilities/Logger.hpp"

#include <utility>

namespace ActsExamples {

IAlgorithm::IAlgorithm(std::string name, Acts::Logging::Level level)
    : m_name(std::move(name)),
      m_logger(Acts::getDefaultLogger(m_name, level)) {}

std::string ActsExamples::IAlgorithm::name() const {
  return m_name;
}

}  // namespace ActsExamples
