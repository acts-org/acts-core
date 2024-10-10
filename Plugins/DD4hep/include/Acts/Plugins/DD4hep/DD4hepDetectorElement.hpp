// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/TGeo/TGeoDetectorElement.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

#include <map>
#include <memory>
#include <string>

#include "DD4hep/DetElement.h"
#include "DD4hep/Segmentations.h"

namespace Acts {

/// Forward declaration of Digitization module is enough
class ISurfaceMaterial;

/// @class DD4hepDetectorElement
///
/// @brief DetectorElement class implementation for DD4hep geometry
///
/// DetectorElement plugin for DD4hep detector elements. DD4hep is based on
/// TGeo shapes, therefore the DD4hepDetectorElement inherits from
/// TGeoDetectorElement in order to perform the conversion.
///
/// The full geometrical information is provided by the TGeoDetectorElement.
/// The DD4hepDetectorElement extends the TGeoDetectorElement by containing a
/// segmentation for the readout.
///
class DD4hepDetectorElement : public TGeoDetectorElement {
 public:
  // Define the context type
  using DD4hepVolumeID = dd4hep::DDSegmentation::VolumeID;

  /// Broadcast the context type
  using ContextType = GeometryContext;

  /// Define a string based story
  using Store = std::map<std::string,
                         std::vector<std::shared_ptr<DD4hepDetectorElement>>>;

  /// Constructor
  /// @param detElement The DD4hep DetElement which should be associated to
  /// an ACTS surface
  ///
  /// @param axes is the axis orientation with respect to the tracking frame
  ///        it is a string of the three characters x, y and z (standing for
  ///        the three axes) There is a distinction between
  /// capital and lower case
  /// characters :
  /// 	- capital      -> positive orientation of the axis
  ///		- lower case   -> negative orientation of the axis
  ///
  ///
  /// Example options are:
  /// 	- "XYZ" -> identical frame definition (default value)
  /// 	- "YZX" -> node y axis is tracking x axis, etc.
  ///		- "XzY" -> negative node z axis is tracking y axis, etc.
  /// @param scalor is the scale factor for unit conversion if needed
  /// @param isDisc in case the sensitive detector module should be translated
  ///        as disc (e.g. for endcaps) this flag should be set to true
  /// @note In the translation from a 3D geometry (TGeo) which only knows
  ///       tubes to a 2D geometry (Tracking geometry) a distinction if the
  ///       module should be described as a cylinder or a disc surface needs to
  ///       be done. Since this information can not be taken just from the
  ///       geometry description (both can be described as TGeoTubeSeg), one
  ///       needs to set the flag 'isDisc' in case a volume with shape \c
  ///       TGeoTubeSeg should be translated to a disc surface. Per default it
  ///       will be translated into a cylindrical surface.
  /// @param material Optional material of detector element
  DD4hepDetectorElement(
      const dd4hep::DetElement detElement, const std::string& axes = "XYZ",
      double scalor = 1., bool isDisc = false,
      std::shared_ptr<const ISurfaceMaterial> material = nullptr);

  ~DD4hepDetectorElement() override = default;

  // Give access to the DD4hep detector element
  const dd4hep::DetElement& sourceElement() const { return m_detElement; }

 private:
  /// DD4hep detector element
  dd4hep::DetElement m_detElement;
  /// DD4hep segmentation
  dd4hep::Segmentation m_segmentation;
};

/// This extension holds an ACTS detector element belonging to a DD4hep detector
/// element, and synchronizes ownership
struct DD4hepDetectorElementExtension {
  explicit DD4hepDetectorElementExtension(
      std::shared_ptr<DD4hepDetectorElement> de)
      : detectorElement(std::move(de)) {
    throw_assert(detectorElement != nullptr,
                 "DD4hepDetectorElement is nullptr");
  }

 private:
  std::shared_ptr<DD4hepDetectorElement> detectorElement;
};

}  // namespace Acts
