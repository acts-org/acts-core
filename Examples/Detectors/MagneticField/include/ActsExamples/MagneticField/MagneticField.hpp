// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/MagneticField/NullBField.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/detail/grid_helper.hpp"
#include "ActsExamples/MagneticField/ScalableBField.hpp"

#include <memory>
#include <variant>
#include <vector>

namespace ActsExamples::detail {

using InterpolatedMagneticField2 = Acts::InterpolatedBFieldMap<
    Acts::Grid<Acts::Vector2, Acts::Axis<Acts::AxisType::Equidistant>,
               Acts::Axis<Acts::AxisType::Equidistant>>>;

using InterpolatedMagneticField3 = Acts::InterpolatedBFieldMap<
    Acts::Grid<Acts::Vector3, Acts::Axis<Acts::AxisType::Equidistant>,
               Acts::Axis<Acts::AxisType::Equidistant>,
               Acts::Axis<Acts::AxisType::Equidistant>>>;

}  // namespace ActsExamples::detail
