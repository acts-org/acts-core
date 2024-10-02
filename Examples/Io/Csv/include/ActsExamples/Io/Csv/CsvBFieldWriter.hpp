// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

namespace Acts {
class InterpolatedMagneticField;
class MagneticFieldProvider;
}  // namespace Acts

namespace ActsExamples {
/// @brief Writer for B-fields that outputs field data in CSV format.
///
/// This tool allows users to dump their magnetic field data to disk in CSV
/// format, which is (relatively) human-readable and widely supported.
class CsvBFieldWriter {
 public:
  /// @brief Enumeration type for the coordinate system to use in the writing
  /// process.
  ///
  /// This enumeration sets Cartesian coordinates in the XYZ case, and
  /// symmetrical cylindrical coordinates in the RZ case.
  enum class CoordinateType { XYZ, RZ };

  /// @brief Configuration type for the magnetic field writer.
  ///
  /// @tparam Coord Sets the coordinate system to use: either Cartesian or
  /// cylindrical.
  /// @tparam Grid If true, assume that the magnetic field is grid-based and
  /// that it has a built-in bin count and size. Otherwise, assume it is
  /// analytical.
  template <CoordinateType Coord, bool Grid>
  struct Config {
    /// Helper type constructor that makes values optional in grid-based
    /// environments. The reason for this is that grid-based fields have
    /// built-in bins and sizes, and as such it is not strictly necessary for
    /// the user to supply them. For gridless fields this does not work, and the
    /// data must become mandatory.
    template <typename T>
    using Functor = std::conditional_t<Grid, std::optional<T>, T>;

    /// @brief Dimensionality of vectors.
    ///
    /// Set the number of scalars in our coordinates to 3 if we have 3D
    /// Cartesian coordinates, or 2 for cylindrical.
    static constexpr std::size_t NDims = Coord == CoordinateType::RZ ? 2 : 3;

    /// @brief Output file to write the magnetic field to.
    std::string fileName = "bfield.csv";

    /// @brief The range over which to dump the vector field.
    ///
    /// For each dimension, contains two optional values: the minimum and
    /// maximum in that dimension. If any value is non-extant, the gridded
    /// field's internal contents are used as a default.
    std::array<std::array<Functor<double>, 2>, NDims> range{};

    /// @brief Number of bins in the output vector field.
    ///
    /// The same logic as above holds for the ranges, use the user-supplied
    /// value if available, otherwise use the value from the grid.
    std::array<Functor<std::size_t>, NDims> bins{};

    /// @brief Magnetic field to read from.
    ///
    /// Note that the type of this field is different from grid-based and
    /// non-grid-based fields: @class Acts::InterpolatedMagneticField and
    /// @class Acts::MagneticFieldProvider, respectively.
    std::shared_ptr<std::add_const_t<std::conditional_t<
        Grid, Acts::InterpolatedMagneticField, Acts::MagneticFieldProvider>>>
        bField;
  };

  ///@brief Write magnetic field data to a CSV file.
  ///
  ///@tparam Coord Specifies the coordinate system to use while writing; this is
  /// either Cartesian (XYZ) or cylindrical (RZ).
  ///@tparam Grid Boolean specifying whether the magnetic field is a grid and has
  /// set size, or whether it is analytical.
  ///
  ///@param config Configuration of the writing job.
  ///@param level Log level to use during execution.
  template <CoordinateType Coord, bool Grid>
  static void run(const Config<Coord, Grid>& config,
                  Acts::Logging::Level level);
};
}  // namespace ActsExamples
