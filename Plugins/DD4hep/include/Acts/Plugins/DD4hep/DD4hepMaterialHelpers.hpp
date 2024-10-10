// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <DD4hep/DetElement.h>
#include <DD4hep/DetFactoryHelper.h>
#include <DDRec/DetectorData.h>

namespace Acts {

class Layer;

/// Helper method to translate DD4hep material to Acts::ISurfaceMaterial
///
/// This is used to assign proto material to Cylinder Layers
///
/// @param detElement the DD4hep detector element for which this material is
///                   assigned
/// @param cylinderLayer is the target layer
/// @param logger a @c Logger for output
void addCylinderLayerProtoMaterial(dd4hep::DetElement detElement,
                                   Layer& cylinderLayer,
                                   const Logger& logger = getDummyLogger());

/// Helper method to translate DD4hep material to Acts::ISurfaceMaterial
///
/// Thisis used to assign proto material to Disc Layers
///
/// @param detElement the DD4hep detector element for which this material is
/// assigned
/// @param discLayer is the target layer
/// @param logger a @c Logger for output
void addDiscLayerProtoMaterial(dd4hep::DetElement detElement, Layer& discLayer,
                               const Logger& logger = getDummyLogger());

/// Helper method to be called for Cylinder and Disc Proto material
///
/// For both, cylinder and disc, the closed binning value is "binPhi"
///
/// @param params An instance of @c DD4hep::VariantParameters
/// @param layer the Layer to assign the proto material
/// @param binning the Binning prescription for the ActsExtension
/// @param logger a @c Logger for output
void addLayerProtoMaterial(
    const dd4hep::rec::VariantParameters& params, Layer& layer,
    const std::vector<std::pair<const std::string, Acts::BinningOption> >&
        binning,
    const Logger& logger = getDummyLogger());

/// Helper method to create proto material - to be called from the
/// addProto(...) methods
///
/// @param params An instance of @c DD4hep::VariantParameters
/// @param valueTag the xml tag for to ActsExtension to be parsed
/// @param binning the Binning prescription for the ActsExtension
/// @param logger a @c Logger for output
std::shared_ptr<Acts::ProtoSurfaceMaterial> createProtoMaterial(
    const dd4hep::rec::VariantParameters& params, const std::string& valueTag,
    const std::vector<std::pair<const std::string, Acts::BinningOption> >&
        binning,
    const Logger& logger = getDummyLogger());

}  // namespace Acts
