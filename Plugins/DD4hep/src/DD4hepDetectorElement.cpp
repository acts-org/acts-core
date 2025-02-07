// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"

#include <utility>

#include <DD4hep/Alignments.h>
#include <DD4hep/DetElement.h>
#include <DD4hep/Volumes.h>

Acts::DD4hepDetectorElement::DD4hepDetectorElement(
    const dd4hep::DetElement detElement, const std::string& axes, double scalor,
    bool /*isDisc*/, std::shared_ptr<const ISurfaceMaterial> material)
    : TGeoDetectorElement(
          static_cast<TGeoDetectorElement::Identifier>(detElement.volumeID()),
          *(detElement.placement().ptr()),
          detElement.nominal().worldTransformation(), axes, scalor,
          std::move(material)),
      m_detElement(detElement) {}
