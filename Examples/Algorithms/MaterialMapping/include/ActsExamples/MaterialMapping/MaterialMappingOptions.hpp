// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/Utilities/Options.hpp"

#include <iostream>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace ActsExamples {

namespace Options {

/// @brief Material mapping options, specially added
///
/// @tparam aopt_t Type of the options object (API bound to boost)
///
/// @param [in] opt_t The options object where the specific digitization
/// options are attached to
template <typename aopt_t>
void addMaterialMappingOptions(aopt_t& opt) {
  opt.add_options()(
      "mat-mapping-emptybins", po::value<bool>()->default_value(true),
      "Empty bin correction (recommended). Corrects for vacuum/empty "
      "assignments.")("mat-mapping-surfaces",
                      po::value<bool>()->default_value(true),
                      "Map the material onto the selected surfaces")(
      "mat-mapping-volumes", po::value<bool>()->default_value(false),
      "Map the material into the selected material volumes")(
      "mat-mapping-volume-stepsize",
      po::value<float>()->default_value(std::numeric_limits<float>::infinity()),
      "Step size of the sampling of volume material for the mapping "
      "(should be smaller than the size of the bins in depth)")(
      "mat-mapping-read-surfaces",
      po::value<bool>()->default_value(po::value<bool>()->default_value(false)),
      "Read the surface associated with each material hit, can be used to "
      "speed up the mapping. "
      "The mapping needs to have been performed at least once for the surface "
      "information to be there.");
}

}  // namespace Options
}  // namespace ActsExamples
