// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/DD4hepDetector/DD4hepAlignmentDecorator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"
#include "Acts/Plugins/DD4hep/DD4hepGeometryContext.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include <Acts/Geometry/TrackingGeometry.hpp>

#include <ostream>
#include <thread>
#include <utility>

ActsExamples::DD4hep::DD4hepAlignmentDecorator::DD4hepAlignmentDecorator(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : m_cfg(cfg), m_logger(std::move(logger)) {
  if (m_cfg.trackingGeometry != nullptr) {
    parseGeometry(*m_cfg.trackingGeometry.get());
  }
}

ActsExamples::ProcessCode
ActsExamples::DD4hep::DD4hepAlignmentDecorator::decorate(
    AlgorithmContext& context) {
  Acts::DD4hepGeometryContext dd4hepGeoCtx =
      Acts::DD4hepGeometryContext(m_cfg.nominal);
  if (!m_cfg.nominal) {
    initializeMisFromJson(m_cfg.misAlignedGeoJsonPath);
    for (const auto& entry : m_misalignmentAtConstruction) {
      const std::string& identifier = entry.first;
      const Acts::Transform3& misalignmentTransform = entry.second;
      auto nominalIt = m_nominalStore.find(identifier);
      if (nominalIt != m_nominalStore.end()) {
        const Acts::Transform3& nominalTransform = nominalIt->second;
        Eigen::Matrix3d R1 = nominalTransform.rotation();
        Eigen::Vector3d T1 = nominalTransform.translation();
        Eigen::Matrix3d R2 = misalignmentTransform.rotation();
        Eigen::Vector3d T2 = misalignmentTransform.translation();
        Eigen::Matrix3d R3 = R1 * R2;
        Eigen::Vector3d T3 = T1 + T2;
        m_mistransform[identifier] =
            Eigen::Affine3d(Eigen::Translation3d(T3)) * Eigen::Affine3d(R3);
      }
    }
    dd4hepGeoCtx.setAlignmentStore(m_mistransform);
  }
  context.geoContext = dd4hepGeoCtx;
  return ProcessCode::SUCCESS;
}

void ActsExamples::DD4hep::DD4hepAlignmentDecorator::parseGeometry(
    const Acts::TrackingGeometry& tGeometry) {
  // Double-visit - first count
  std::size_t nTransforms = 0;
  tGeometry.visitSurfaces([&nTransforms](const auto*) { ++nTransforms; });
  std::unordered_map<std::string, Acts::Transform3> aStore;
  Acts::GeometryContext nominalCtx = {};
  // Collect the surfacas into the nominal store
  auto fillTransforms = [&aStore, &nominalCtx](const auto* surface) -> void {
    if (surface == nullptr) {
      throw std::invalid_argument("Surface is nullptr.");
    }
    auto alignableElement = dynamic_cast<const Acts::DD4hepDetectorElement*>(
        surface->associatedDetectorElement());
    if (alignableElement == nullptr) {
      throw std::invalid_argument("Surface is not alignable");
    }
    unsigned long long id = alignableElement->identifier();
    aStore[Form("%lld", id)] = surface->transform(nominalCtx);
  };
  tGeometry.visitSurfaces(fillTransforms);
  m_nominalStore = std::move(aStore);
}
