// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvBFieldWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"

#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace ActsExamples {
template <CsvBFieldWriter::CoordinateType Coord, bool Grid>
void CsvBFieldWriter::run(const Config<Coord, Grid>& config,
                          Acts::Logging::Level level) {
  ACTS_LOCAL_LOGGER(Acts::getDefaultLogger("CsvBFieldWriter", level));

  // Some helper typedefs, which will make our life easier down the line.
  using ConfigType = std::decay_t<decltype(config)>;
  using FieldType = typename decltype(ConfigType::bField)::element_type;
  using Vector = Acts::ActsVector<ConfigType::NDims>;

  FieldType& field = *config.bField;

  // Determine the columns to write to the CSV file. For Cartesian coordinates,
  // we give the x, y, and z coordinates for both the position and the field
  // vector. For cylindrical coordinates, we give r and z.
  std::vector<std::string> fields;

  if constexpr (Coord == CoordinateType::XYZ) {
    fields = {"x", "y", "z", "Bx", "By", "Bz"};
  } else {
    fields = {"r", "z", "Br", "Bz"};
  }

  // Initialize a CSV writer to the specified filename using the specified
  // column names.
  CsvWriter writer(fields, config.fileName);

  // We proceed by finding the number of bins, as well as the minimum and
  // maximum coordinates. This process depends quite heavily on the structure
  // of the magnetic field, so we need some compile-time conditionals.
  std::array<std::size_t, ConfigType::NDims> bins{};
  Vector min, max;

  if constexpr (Grid) {
    // First, we should check whether the Bfield actually has a
    // three-dimensional bin count and size.
    if (bins.size() != ConfigType::NDims ||
        field.getMin().size() != ConfigType::NDims ||
        field.getMax().size() != ConfigType::NDims) {
      throw std::invalid_argument("Incorrect input B-field dimensionality.");
    }

    // If our magnetic field is grid-like, we know that it has a built-in size
    // and bin count. We can use these, but the user may want to override the
    // values with custom values.
    for (std::size_t i = 0; i < ConfigType::NDims; ++i) {
      // For each dimension, we first get the corresponding value from the
      // grid-based vector field...
      bins[i] = field.getNBins()[i];
      min[i] = field.getMin()[i];
      max[i] = field.getMax()[i];

      // ...and then we override them with the optional user-supplied values.
      if (config.bins[i]) {
        bins[i] = *config.bins[i];
      }

      if (config.range[i][0]) {
        min[i] = *config.range[i][0];
      }

      if (config.range[i][1]) {
        max[i] = *config.range[i][1];
      }
    }
  } else {
    // If the vector field is not grid based, then there is no side and we are
    // forced to use the user-supplied data. Remember that in this case, the
    // values are not optional.
    for (std::size_t i = 0; i < ConfigType::NDims; ++i) {
      bins[i] = config.bins[i];
      min[i] = config.range[i][0];
      max[i] = config.range[i][1];
    }
  }

  // Next, we calculate the size (in physical space) of each bin.
  Vector delta;

  for (std::size_t i = 0; i < ConfigType::NDims; ++i) {
    delta[i] = (max[i] - min[i]) / (bins[i] - 1);
  }

  // Create the appropriate magnetic field context and cache to interact with
  // the B fields.
  Acts::MagneticFieldContext mctx{};
  typename FieldType::Cache cache = field.makeCache(mctx);

  // Finally, we can begin to fill the output file with data from our B field.
  // Again, the procedure is slightly different depending on whether we are
  // working with Cartesian or cylindrical coordinates.
  if constexpr (Coord == CoordinateType::XYZ) {
    ACTS_INFO("Writing XYZ field of size " << bins[0] << " x " << bins[1]
                                           << " x " << bins[2] << " to file "
                                           << config.fileName << "...");

    std::size_t total_items = bins[0] * bins[1] * bins[2];

    // For Cartesian coordinates, iterate over bins in the x, y, and z
    // directions. Note that we iterate one additional time because we are
    // writing the _edges_ of the bins, and the final bin needs to be closed.
    for (std::size_t x = 0; x < bins[0]; ++x) {
      for (std::size_t y = 0; y < bins[1]; ++y) {
        for (std::size_t z = 0; z < bins[2]; ++z) {
          // Compute the geometric position of this bin, then request the
          // magnetic field vector at that position.
          Acts::Vector3 pos = {x * delta[0] + min[0], y * delta[1] + min[1],
                               z * delta[2] + min[2]};

          Acts::Vector3 bField;
          if (auto fieldMap =
                  dynamic_cast<const Acts::InterpolatedMagneticField*>(
                      &field)) {
            // InterpolatedMagneticField::getField() returns an error for the
            // final point (upper edge), which is just outside the field volume.
            // So we use getFieldUnchecked instead.
            bField = fieldMap->getFieldUnchecked(pos);
          } else {
            Acts::Result<Acts::Vector3> flx = field.getField(pos, cache);

            // The aforementioned method is not guaranteed to succeed, so we
            // must check for a valid result, and then write it to disk. If the
            // result is invalid, throw an exception.
            if (flx.ok()) {
              bField = *flx;
            } else {
              throw std::runtime_error("B-field returned a non-extant value!");
            }
          }

          writer.append(pos[0] / Acts::UnitConstants::mm,
                        pos[1] / Acts::UnitConstants::mm,
                        pos[2] / Acts::UnitConstants::mm,
                        bField[0] / Acts::UnitConstants::T,
                        bField[1] / Acts::UnitConstants::T,
                        bField[2] / Acts::UnitConstants::T);

          // This final part is some diagnostic to convince the user that the
          // program is still running. We periodically provide the user with
          // some useful data.
          std::size_t idx = (x * bins[1] * bins[2]) + (y * bins[2]) + z + 1;

          if (idx % 10000 == 0 || idx == total_items) {
            ACTS_VERBOSE("Wrote " << idx << " out of " << total_items
                                  << " items (" << std::setprecision(3)
                                  << ((100.f * idx) / total_items) << "%).");
          }
        }
      }
    }
  } else {
    ACTS_INFO("Writing RZ field of size " << bins[0] << " x " << bins[1]
                                          << " to file " << config.fileName
                                          << "...");

    std::size_t total_items = bins[0] * bins[1];

    // For cylindrical coordinates, we only need to iterate over the r and z
    // coordinates, because we assume rotational cylindrical symmetry. This
    // makes the procedure quite a bit faster, too. Great!
    for (std::size_t r = 0; r < bins[0]; ++r) {
      for (std::size_t z = 0; z < bins[1]; ++z) {
        // Calculate the position (still in three dimensions), assuming that
        // the phi coordinate is zero. Then grab the field.
        Acts::Vector3 pos(min[0] + r * delta[0], 0.f, min[1] + z * delta[1]);

        Acts::Vector3 bField;
        if (auto fieldMap =
                dynamic_cast<const Acts::InterpolatedMagneticField*>(&field)) {
          // InterpolatedMagneticField::getField() returns an error for the
          // final point (upper edge), which is just outside the field volume.
          // So we use getFieldUnchecked instead.
          bField = fieldMap->getFieldUnchecked(pos);
        } else {
          Acts::Result<Acts::Vector3> flx = field.getField(pos, cache);

          // Check the result, then write to disk. We write the r and z
          // positions as they are, then we write the z component of the result
          // vector as is, and we compute the r-value from the other components
          // of the vector.
          if (flx.ok()) {
            bField = *flx;
          } else {
            throw std::runtime_error("B-field returned a non-extant value!");
          }
        }

        writer.append(
            pos[0] / Acts::UnitConstants::mm, pos[2] / Acts::UnitConstants::mm,
            Acts::VectorHelpers::perp(bField) / Acts::UnitConstants::T,
            bField[2] / Acts::UnitConstants::T);

        // As before, print some progress reports for the user to enjoy while
        // they wait.
        std::size_t idx = (r * bins[1]) + z + 1;

        if (idx % 10000 == 0 || idx == total_items) {
          ACTS_VERBOSE("Wrote " << idx << " out of " << total_items
                                << " items (" << std::setprecision(3)
                                << ((100.f * idx) / total_items) << "%).");
        }
      }
    }
  }
}

// Note that this is a C++ source file, and not a header file. The reason for
// this is that we can easily enumerate the different template parameter
// combinations for the function above, and so we want to speed up compilation
// times by just compiling these symbols once into the object file
// corresponding to this TU.
template void CsvBFieldWriter::run<CsvBFieldWriter::CoordinateType::XYZ, true>(
    const Config<CoordinateType::XYZ, true>&, Acts::Logging::Level);
template void CsvBFieldWriter::run<CsvBFieldWriter::CoordinateType::RZ, true>(
    const Config<CoordinateType::RZ, true>&, Acts::Logging::Level);
template void CsvBFieldWriter::run<CsvBFieldWriter::CoordinateType::XYZ, false>(
    const Config<CoordinateType::XYZ, false>&, Acts::Logging::Level);
template void CsvBFieldWriter::run<CsvBFieldWriter::CoordinateType::RZ, false>(
    const Config<CoordinateType::RZ, false>&, Acts::Logging::Level);
}  // namespace ActsExamples
