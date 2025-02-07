// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/MagneticField/ScalableBFieldService.hpp"

#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/MagneticField/ScalableBField.hpp"

#include <any>
#include <cmath>

namespace {
const std::string s_name = "ScalableBFieldService";
}

namespace ActsExamples {

ScalableBFieldService::ScalableBFieldService(const Config& cfg,
                                             Acts::Logging::Level lvl)
    : m_cfg(cfg), m_logger(Acts::getDefaultLogger(s_name, lvl)) {}

const std::string& ScalableBFieldService::name() const {
  return s_name;
}

ProcessCode ScalableBFieldService::decorate(AlgorithmContext& ctx) {
  ScalableBFieldContext magCtx;
  magCtx.scalor = std::pow(m_cfg.scalor, ctx.eventNumber);
  ctx.magFieldContext = std::make_any<ScalableBFieldContext>(magCtx);
  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
