// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <Acts/Geometry/GeometryIdentifier.hpp>
#include <ActsExamples/EventData/MeasurementCalibration.hpp>

#include <filesystem>

#include <TFile.h>
#include <TH2D.h>

namespace ActsExamples {

class ScalingCalibrator : public MeasurementCalibrator {
 public:
  struct ConstantTuple {
    double x_offset{0};
    double x_scale{1};
    double y_offset{0};
    double y_scale{1};
  };

  struct MapTuple {
    TH2D x_offset;
    TH2D x_scale;
    TH2D y_offset;
    TH2D y_scale;

    ConstantTuple at(std::size_t sizeLoc0, std::size_t sizeLoc1) const {
      ConstantTuple ct;
      ct.x_offset =
          x_offset.GetBinContent(x_offset.FindFixBin(sizeLoc0, sizeLoc1));
      ct.x_scale =
          x_scale.GetBinContent(x_scale.FindFixBin(sizeLoc0, sizeLoc1));
      ct.y_offset =
          y_offset.GetBinContent(y_offset.FindFixBin(sizeLoc0, sizeLoc1));
      ct.y_scale =
          y_scale.GetBinContent(y_scale.FindFixBin(sizeLoc0, sizeLoc1));
      return ct;
    }
  };

  explicit ScalingCalibrator(const std::filesystem::path& path);

  void calibrate(
      const MeasurementContainer& measurements,
      const ClusterContainer* clusters, const Acts::GeometryContext& gctx,
      const Acts::CalibrationContext& cctx, const Acts::SourceLink& sourceLink,
      Acts::VectorMultiTrajectory::TrackStateProxy& trackState) const override;

  bool needsClusters() const override { return true; }

 private:
  std::map<Acts::GeometryIdentifier, MapTuple> m_calib_maps;
  std::bitset<3> m_mask;
};

}  // namespace ActsExamples
