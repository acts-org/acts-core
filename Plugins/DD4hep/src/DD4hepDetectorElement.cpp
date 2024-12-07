// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"

#include "Acts/Plugins/DD4hep/DD4hepGeometryContext.hpp"

#include <iostream>
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

const Acts::Transform3& Acts::DD4hepDetectorElement::transform(
    const GeometryContext& gctx) const {
  const Acts::DD4hepGeometryContext* dd4hepGtx =
      gctx.maybeGet<DD4hepGeometryContext>();
  if (dd4hepGtx != nullptr && !dd4hepGtx->isNominal()) {
    return dd4hepGtx->contextualTransform(*this);
  }
  return TGeoDetectorElement::transform(gctx);
}
