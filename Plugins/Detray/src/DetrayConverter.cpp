// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Detray/DetrayConverter.hpp"

Acts::DetrayConverter::DetrayConverter(
    std::unique_ptr<const Acts::Logger> logger)
    : m_logger(std::move(logger)) {}

void Acts::DetrayConverter::writeToJson(
    const DetrayHostDetector& dDetector,
    const typename DetrayHostDetector::name_map& names,
    detray::io::detector_writer_config writer_cfg) {
  writer_cfg.format(detray::io::format::json);
  detray::io::write_detector(dDetector, names, writer_cfg);
}
