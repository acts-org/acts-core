// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

#include <string>

#include <DD4hep/DetFactoryHelper.h>
#include <DD4hep/Objects.h>
#include <DDRec/DetectorData.h>
#include <XML/Utilities.h>

using namespace dd4hep;

namespace Acts {
class Surface;
}  // namespace Acts

namespace DD4hepTestsHelper {

/// @brief helper to ensure that an extension is set,
/// copied from the ODD detector code
///
/// @tparam T the type of the extension
/// @param elt the detector element
/// @return the extracted/created extennsion
template <typename T>
T& ensureExtension(dd4hep::DetElement& elt) {
  T* ext = elt.extension<T>(false);
  if (ext == nullptr) {
    ext = new T();
  }
  elt.addExtension<T>(ext);
  return *ext;
}

/// Helper method to decode the binning from what would appear in the
/// xml into variant parameters, such that it can be understood in the
/// downstream processing.
///
/// This parses the dediced \< surface_binning \> tag
/// - allowed/understood binnings are x,y,z,phi,r
/// - allowed/unserstood types are equidistant/variable (those are
/// auto-detected)
///
/// Example for e.g. bname = \"surface_binning\":
///
/// - Equidistant binning in r and phi:
///   \<acts_surface_binning nr=\"2\" rmin=\"25\" rmax=\"100\" nphi=\"22\"
///   phimin=\"-3.1415\" phimax=\"3.1415\" \/ \>
/// - Variable binning in z:
///   \<acts_surface_binning zboundaries=\"-100,-90,90,100\" \/ \>
///
/// And 2D combinations of this are allowed.
///
/// @param variantParams [in,out] the variant parameters that will be overwritten
/// @param xmlBinning the surface binning
/// @param bname the binning base name, e.g. surface_binning, material_binning
/// @param bvals the boundary values, i.e. x,y,z,phi,r
///
void decodeBinning(dd4hep::rec::VariantParameters& variantParams,
                   const xml_comp_t& xmlBinning, const std::string& bname,
                   const std::vector<std::string>& bvals);

/// Helper method to create a Transform3D from an xml detector
/// component
///
/// @param x_det_comp the xml detector component
///
/// @return a Transform3D (DD4hep type, aka ROOT::Math type)
Transform3D createTransform(const xml_comp_t& x_det_comp);

/// Helper method to convert an ACTS transform into XML
///
/// @param tf the transform in ACTS format
/// @param axes the identification which axes are building the local frame
///
/// @return a string representing the XML entry
std::string transformToXML(const Acts::Transform3& tf,
                           const std::array<int, 2u>& axes = {0, 1});

/// @brief  Helper method to convert a Surface into XML
///
/// @param gctx the geometry context of this call
/// @param surface the surface from ACTS to be written
/// @param ref the reference transform
///
/// @return a string representing the XML entry
std::string surfaceToXML(const Acts::GeometryContext& gctx,
                         const Acts::Surface& surface,
                         const Acts::Transform3& ref);

}  // namespace DD4hepTestsHelper
