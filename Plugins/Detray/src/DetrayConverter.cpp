// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
