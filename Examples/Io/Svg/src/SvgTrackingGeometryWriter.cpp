// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/Svg/SvgTrackingGeometryWriter.hpp"

#include "ActsExamples/Utilities/Paths.hpp"
#include <Acts/Geometry/Layer.hpp>
#include <Acts/Geometry/TrackingGeometry.hpp>
#include <Acts/Geometry/TrackingVolume.hpp>
#include <Acts/Plugins/ActSVG/LayerSvgConverter.hpp>

#include <iostream>
#include <string>

ActsExamples::SvgTrackingGeometryWriter::SvgTrackingGeometryWriter(
    const ActsExamples::SvgTrackingGeometryWriter::Config& config,
    Acts::Logging::Level level)
    : m_logger{Acts::getDefaultLogger(name(), level)}, m_cfg(config) {}

std::string ActsExamples::SvgTrackingGeometryWriter::name() const {
  return "SvgTrackingGeometryWriter";
}

ActsExamples::ProcessCode ActsExamples::SvgTrackingGeometryWriter::write(
    const AlgorithmContext& context, const Acts::TrackingGeometry& tGeometry) {
  ACTS_DEBUG(">>Svg: Writer for TrackingGeometry object called.");

  m_writeMutex.lock();

  auto geometrySheets = Acts::Svg::TrackingGeometryConverter::convert(
      context.geoContext, tGeometry, m_cfg.converterOptions);

  // Write them out
  for (const auto& sheet : geometrySheets) {
    Acts::Svg::toFile({sheet}, joinPaths(m_cfg.outputDir, sheet._id + ".svg"));
  }
  // Successfully done
  return ActsExamples::ProcessCode::SUCCESS;
}
