// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include <TFile.h>
#include <TTree.h>
#include <boost/optional.hpp>

namespace ActsExamples {

/// @class RootBFieldWriter
///
/// Writes out the Acts::InterpolatedbFieldMap. Currently implemented for 'rz'
/// and 'xyz' field maps.
class RootBFieldWriter {
 public:
  /// Describes the axes definition of the grid of the magnetic field map.
  enum class GridType { rz = 0, xyz = 1 };

  struct Config {
    /// The name of the output tree
    std::string treeName = "TTree";
    /// The name of the output file
    std::string fileName = "TFile.root";
    /// the file access mode (recreate by default)
    std::string fileMode = "recreate";
    /// The magnetic field to be written out
    std::shared_ptr<const Acts::InterpolatedMagneticField> bField;
    /// How the magnetic field map should be written out
    GridType gridType = GridType::xyz;
    /// [optional] Setting the range to be printed out in either r (for
    /// cylinder coordinates) or x/y (in cartesian coordinates)
    /// @note setting this parameter is optional, in case no boundaries are
    /// handed over the full magnetic field map will be printed out
    std::optional<std::array<double, 2>> rBounds;
    /// [optional] Setting the range in z to be printed out
    /// @note setting this parameter is optional, in case no boundaries are
    /// handed over the full magnetic field map will be printed out
    std::optional<std::array<double, 2>> zBounds;
    /// Number of bins in r
    /// @note setting this parameter is optional, in case no bin numbers are
    /// handed over the full magnetic field map will be printed out
    std::size_t rBins = 200;
    /// Number of bins in z
    // @note setting this parameter is optional, in case no bin numbers are
    /// handed over the full magnetic field map will be printed out
    std::size_t zBins = 300;
    /// Number of bins in phi
    // @note setting this parameter is optional, in case no bin numbers are
    /// handed over the full magnetic field map will be printed out
    std::size_t phiBins = 100;
  };

  /// Write down an interpolated magnetic field map
  static void run(const Config& config,
                  std::unique_ptr<const Acts::Logger> p_logger =
                      Acts::getDefaultLogger("RootBFieldWriter",
                                             Acts::Logging::INFO));
};

}  // namespace ActsExamples
