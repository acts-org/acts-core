// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
