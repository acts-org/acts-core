// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Plugins/ActSVG/EventDataSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Utilities/StringHelpers.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsExamples/Io/Svg/SvgTrackingGeometryWriter.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <fstream>
#include <mutex>

namespace ActsExamples {

/// Standard XYZ accessor
///
struct AccessorXYZ {
  template <typename T>
  auto x(const T& o) const {
    return o.x();
  }

  template <typename T>
  auto y(const T& o) const {
    return o.y();
  }

  template <typename T>
  auto z(const T& o) const {
    return o.z();
  }
};

/// Standard XYZ accessor from a position() call
///
struct AccessorPositionXYZ {
  template <typename T>
  auto x(const T& o) const {
    return o.position().x();
  }

  template <typename T>
  auto y(const T& o) const {
    return o.position().y();
  }

  template <typename T>
  auto z(const T& o) const {
    return o.position().z();
  }
};

/// Write out any 3D point collection in Svg format.
///
/// @tparam TColl is the collection type
/// @tparam Acc is the accessor type for x(), y(), z()
///
///
/// This writes one file per event into the configured output directory. By
/// default it writes to the current working directory. Files are named
/// using the following schema:
///
///     event000000001-spacepoints.svg
///     event000000002-spacepoints.svg
///
template <typename T, typename Acc = AccessorXYZ>
class SvgPointWriter final : public WriterT<GeometryIdMultiset<T>> {
 public:
  struct Config {
    std::string writerName = "PointWriter";  ///< the name of the writer
    std::string inputCollection;             ///< which collection to write
    std::string outputDir;                   ///< where to place output files
    std::size_t outputPrecision = 6;         ///< floating point precision

    Acts::ActsScalar spSize = 10.;  //!< size of the space point to be drawn
    Acts::Svg::Style spStyle =
        s_pointStyle;  //!< The style of the space point to be drawn

    std::string infoBoxTitle = "";  //!< If an info box title is set, draw it
    Acts::Svg::Style infoTitleStyle = s_infoStyle;
    Acts::Svg::Style infoBoxStyle = s_infoStyle;  // The style of the info box

    bool projectionXY = true;  ///< xy projection
    std::array<Acts::ActsScalar, 2> zRangeXY = {
        std::numeric_limits<Acts::ActsScalar>::lowest(),
        std::numeric_limits<Acts::ActsScalar>::max()};  ///< View range in z of
                                                        ///< the XY view
    bool projectionZR = true;                           ///< zr projection

    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry =
        nullptr;  ///< The tracking geometry, a set pointer will cause the
                  ///< geometry to be drawn

    Acts::Svg::TrackingGeometryConverter::Options trackingGeometryOptions =
        s_backgroundTrackingGeometryOptions;
  };

  SvgPointWriter(const Config& cfg,
                 Acts::Logging::Level level = Acts::Logging::INFO);

 protected:
  ActsExamples::ProcessCode writeT(
      const ActsExamples::AlgorithmContext& context,
      const GeometryIdMultiset<T>& pointCollection) final;

 private:
  Config m_cfg;

  std::mutex m_writeMutex;
};

}  // namespace ActsExamples

template <typename T, typename Acc>
ActsExamples::SvgPointWriter<T, Acc>::SvgPointWriter(
    const ActsExamples::SvgPointWriter<T, Acc>::Config& cfg,
    Acts::Logging::Level level)
    : WriterT<GeometryIdMultiset<T>>(cfg.inputCollection, cfg.writerName,
                                     level),
      m_cfg(cfg) {
  if (m_cfg.inputCollection.empty()) {
    throw std::invalid_argument("Missing input collection");
  }
}

template <typename T, typename Acc>
ActsExamples::ProcessCode ActsExamples::SvgPointWriter<T, Acc>::writeT(
    const ActsExamples::AlgorithmContext& context,
    const GeometryIdMultiset<T>& pointCollection) {
  // Ensure exclusive access to file writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  Acc xyz;

  // Open per-event files
  std::string pathXY =
      perEventFilepath(m_cfg.outputDir, "pointsXY.svg", context.eventNumber);

  // Open per-event files
  std::string pathZR =
      perEventFilepath(m_cfg.outputDir, "pointsZR.svg", context.eventNumber);

  Acts::Svg::TrackingGeometryProjections::Options tgpOptions;
  tgpOptions.trackingGeometryOptions = m_cfg.trackingGeometryOptions;
  auto [xyView, zrView] = Acts::Svg::TrackingGeometryProjections::convert(
      context.geoContext, *m_cfg.trackingGeometry, tgpOptions);

  // Fill the space points
  unsigned int id = 0;
  for (const auto& data : pointCollection) {
    // Use the accessor to build an x/y view
    Acts::Vector3 point3D = {xyz.x(data), xyz.y(data), xyz.z(data)};
    // Draw the xy view
    if (m_cfg.projectionXY && point3D.z() >= m_cfg.zRangeXY[0] &&
        point3D.z() <= m_cfg.zRangeXY[1]) {
      auto p = Acts::Svg::EventDataConverter::pointXY(point3D, m_cfg.spSize,
                                                      m_cfg.spStyle, id);
      xyView.add_object(p);
      // Draw a connected text box
      if (!m_cfg.infoBoxTitle.empty()) {
        auto xyIbox = Acts::Svg::infoBox(
            static_cast<actsvg::scalar>(point3D.x() + 10.),
            static_cast<actsvg::scalar>(point3D.y() - 10.), m_cfg.infoBoxTitle,
            m_cfg.infoTitleStyle, {"Position: " + Acts::toString(point3D)},
            m_cfg.infoBoxStyle, p);
        xyView.add_object(xyIbox);
      }
    }
    // Draw the zy view
    if (m_cfg.projectionZR) {
      auto p = Acts::Svg::EventDataConverter::pointZR(point3D, m_cfg.spSize,
                                                      m_cfg.spStyle, id);

      zrView.add_object(p);
    }
    ++id;
  }

  Acts::Svg::toFile({xyView}, pathXY);
  Acts::Svg::toFile({zrView}, pathZR);

  return ProcessCode::SUCCESS;
}
