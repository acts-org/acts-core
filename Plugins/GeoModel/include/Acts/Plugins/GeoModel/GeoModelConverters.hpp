// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConversionError.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Plugins/GeoModel/IGeoShapeConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GenericGeoShapeConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoBoxConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoIntersectionAnnulusConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoPolygonConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoShiftConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoSubtractionConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoTrdConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoTubeConverter.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoUnionDoubleTrdConverter.hpp"
#include "Acts/Utilities/Result.hpp"

#include <memory>
#include <tuple>
#include <unordered_map>

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoShape.h>
#include <GeoModelKernel/GeoShapeSubtraction.h>

namespace Acts {

/// @brief The GeoBox converter
///
/// This is a dedicated converter for GeoBox shapes
using GeoBoxConverter =
    detail::GenericGeoShapeConverter<GeoBox, detail::GeoBoxConverter>;

using GeoSubtractionConverter =
    detail::GenericGeoShapeConverter<GeoShapeSubtraction,
                                     detail::GeoSubtractionConverter>;

using GeoPolygonConverter =
    detail::GenericGeoShapeConverter<GeoSimplePolygonBrep,
                                     detail::GeoPolygonConverter>;
/// @brief A dedicated converter for GeoInterseciton that describe annulus bounds
///
/// This is very much tailored to the AnnulusBounds class
using GeoIntersectionAnnulusConverter =
    detail::GenericGeoShapeConverter<GeoShapeIntersection,
                                     detail::GeoIntersectionAnnulusConverter>;

/// @brief The GeoShift + Trd/Box/Tube converter
///
/// This is a dedicated converter for GeoBox shapes
using GeoShiftConverter =
    detail::GenericGeoShapeConverter<GeoShapeShift, detail::GeoShiftConverter>;

/// @brief The GeoTrd converter
///
/// This is a dedicated converter for GeoTrd shapes
using GeoTrdConverter =
    detail::GenericGeoShapeConverter<GeoTrd, detail::GeoTrdConverter>;

/// @brief The GeoTube converter
///
/// This is a dedicated converter for GeoTube shapes
using GeoTubeConverter =
    detail::GenericGeoShapeConverter<GeoTube, detail::GeoTubeConverter>;

/// @brief The GeoTube converter
///
/// This is a dedicated converter for GeoTube shapes
using GeoUnionDoubleTrdConverter =
    detail::GenericGeoShapeConverter<GeoShapeUnion,
                                     detail::GeoUnionDoubleTrdConverter>;

/// @brief The map that maps the converters with the shapes

inline std::shared_ptr<const IGeoShapeConverter> geoShapesConverters(
    int geoShapeId) {
  static const std::unordered_map<int,
                                  std::shared_ptr<const IGeoShapeConverter>>
      converters{
          {GeoBox::getClassTypeID(), std::make_shared<GeoBoxConverter>()},
          {GeoShapeIntersection::getClassTypeID(),
           std::make_shared<GeoIntersectionAnnulusConverter>()},
          {GeoShapeShift::getClassTypeID(),
           std::make_shared<GeoShiftConverter>()},
          {GeoTrd::getClassTypeID(), std::make_shared<GeoTrdConverter>()},
          {GeoTube::getClassTypeID(), std::make_shared<GeoTubeConverter>()},
          {GeoSimplePolygonBrep::getClassTypeID(),
           std::make_shared<GeoPolygonConverter>()},
          {GeoShapeSubtraction::getClassTypeID(),
           std::make_shared<GeoSubtractionConverter>()},
          {GeoShapeUnion::getClassTypeID(),
           std::make_shared<GeoUnionDoubleTrdConverter>()}};
  auto itr = converters.find(geoShapeId);

  return itr != converters.end() ? itr->second : nullptr;
};

}  // namespace Acts
