// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/MagneticField/ScalableBFieldService.hpp"

#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/MagneticField/ScalableBField.hpp"

#include <any>
#include <cmath>

namespace {
const std::string s_name = "ScalableBFieldService";
}

ActsExamples::ScalableBFieldService::ScalableBFieldService(
    const Config& cfg, Acts::Logging::Level lvl)
    : m_cfg(cfg), m_logger(Acts::getDefaultLogger(s_name, lvl)) {}

const std::string& ActsExamples::ScalableBFieldService::name() const {
  return s_name;
}

ActsExamples::ProcessCode ActsExamples::ScalableBFieldService::decorate(
    AlgorithmContext& ctx) {
  ScalableBFieldContext magCtx;
  magCtx.scalor = std::pow(m_cfg.scalor, ctx.eventNumber);
  ctx.magFieldContext = std::make_any<ScalableBFieldContext>(magCtx);
  return ProcessCode::SUCCESS;
}
