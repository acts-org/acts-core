// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/DD4hepDetector/DD4hepGeometryService.hpp"

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Plugins/DD4hep/ConvertDD4hepDetector.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>

#include <DD4hep/Detector.h>
#include <DD4hep/Handle.h>
#include <DD4hep/Volumes.h>
#include <Parsers/Printout.h>
#include <TError.h>

class TGeoNode;

ActsExamples::DD4hep::DD4hepGeometryService::DD4hepGeometryService(
    const ActsExamples::DD4hep::DD4hepGeometryService::Config& cfg)
    : m_cfg(cfg),
      m_logger{Acts::getDefaultLogger("DD4hepGeometryService", cfg.logLevel)} {
  if (m_cfg.xmlFileNames.empty()) {
    throw std::invalid_argument("Missing DD4hep XML filenames");
  }
}

ActsExamples::DD4hep::DD4hepGeometryService::~DD4hepGeometryService() {
  drop();
}

ActsExamples::ProcessCode
ActsExamples::DD4hep::DD4hepGeometryService::buildDD4hepGeometry() {
  const int old_gErrorIgnoreLevel = gErrorIgnoreLevel;
  switch (m_cfg.dd4hepLogLevel) {
    case Acts::Logging::Level::VERBOSE:
      dd4hep::setPrintLevel(dd4hep::PrintLevel::VERBOSE);
      break;
    case Acts::Logging::Level::DEBUG:
      dd4hep::setPrintLevel(dd4hep::PrintLevel::DEBUG);
      break;
    case Acts::Logging::Level::INFO:
      dd4hep::setPrintLevel(dd4hep::PrintLevel::INFO);
      break;
    case Acts::Logging::Level::WARNING:
      dd4hep::setPrintLevel(dd4hep::PrintLevel::WARNING);
      gErrorIgnoreLevel = kWarning;
      break;
    case Acts::Logging::Level::ERROR:
      dd4hep::setPrintLevel(dd4hep::PrintLevel::ERROR);
      gErrorIgnoreLevel = kError;
      break;
    case Acts::Logging::Level::FATAL:
      dd4hep::setPrintLevel(dd4hep::PrintLevel::FATAL);
      gErrorIgnoreLevel = kFatal;
      break;
    case Acts::Logging::Level::MAX:
      dd4hep::setPrintLevel(dd4hep::PrintLevel::ALWAYS);
      break;
  }
  // completely silence std::cout as DD4HEP is using it for logging
  if (m_cfg.dd4hepLogLevel >= Acts::Logging::Level::WARNING) {
    std::cout.setstate(std::ios_base::failbit);
  }

  m_detector = &dd4hep::Detector::getInstance();
  for (auto& file : m_cfg.xmlFileNames) {
    m_detector->fromCompact(file.c_str());
  }
  m_detector->volumeManager();
  m_detector->apply("DD4hepVolumeManager", 0, nullptr);
  m_geometry = m_detector->world();

  // restore the logging
  gErrorIgnoreLevel = old_gErrorIgnoreLevel;
  std::cout.clear();

  return ActsExamples::ProcessCode::SUCCESS;
}

dd4hep::Detector&
ActsExamples::DD4hep::DD4hepGeometryService::DD4hepGeometryService::detector() {
  if (m_detector == nullptr) {
    buildDD4hepGeometry();
  }
  return *m_detector;
}

dd4hep::DetElement& ActsExamples::DD4hep::DD4hepGeometryService::geometry() {
  if (!m_geometry) {
    buildDD4hepGeometry();
  }
  return m_geometry;
}

TGeoNode& ActsExamples::DD4hep::DD4hepGeometryService::tgeoGeometry() {
  if (!m_geometry) {
    buildDD4hepGeometry();
  }
  return *m_geometry.placement().ptr();
}

ActsExamples::ProcessCode
ActsExamples::DD4hep::DD4hepGeometryService::buildTrackingGeometry(
    const Acts::GeometryContext& gctx) {
  // Set the tracking geometry
  auto logger = Acts::getDefaultLogger("DD4hepConversion", m_cfg.logLevel);
  m_trackingGeometry = Acts::convertDD4hepDetector(
      geometry(), *logger, m_cfg.bTypePhi, m_cfg.bTypeR, m_cfg.bTypeZ,
      m_cfg.envelopeR, m_cfg.envelopeZ, m_cfg.defaultLayerThickness,
      m_cfg.sortDetectors, gctx, m_cfg.matDecorator,
      m_cfg.geometryIdentifierHook);
  return ActsExamples::ProcessCode::SUCCESS;
}

std::shared_ptr<const Acts::TrackingGeometry>
ActsExamples::DD4hep::DD4hepGeometryService::trackingGeometry(
    const Acts::GeometryContext& gctx) {
  if (!m_trackingGeometry) {
    buildTrackingGeometry(gctx);
  }
  return m_trackingGeometry;
}

void ActsExamples::DD4hep::DD4hepGeometryService::drop() {
  if (m_detector == nullptr) {
    return;
  }
  dd4hep::Detector::destroyInstance(m_cfg.name);
  m_detector = nullptr;
  m_geometry = dd4hep::DetElement();
  m_trackingGeometry = nullptr;
}

void ActsExamples::DD4hep::sortFCChhDetElements(
    std::vector<dd4hep::DetElement>& det) {
  std::vector<dd4hep::DetElement> tracker;
  std::vector<dd4hep::DetElement> eCal;
  std::vector<dd4hep::DetElement> hCal;
  std::vector<dd4hep::DetElement> muon;
  for (auto& detElement : det) {
    std::string detName = detElement.name();
    if (detName.find("Muon") != std::string::npos) {
      muon.push_back(detElement);
    } else if (detName.find("ECal") != std::string::npos) {
      eCal.push_back(detElement);
    } else if (detName.find("HCal") != std::string::npos) {
      hCal.push_back(detElement);
    } else {
      tracker.push_back(detElement);
    }
  }
  sort(muon.begin(), muon.end(),
       [](const dd4hep::DetElement& a, const dd4hep::DetElement& b) {
         return (a.id() < b.id());
       });
  sort(eCal.begin(), eCal.end(),
       [](const dd4hep::DetElement& a, const dd4hep::DetElement& b) {
         return (a.id() < b.id());
       });
  sort(hCal.begin(), hCal.end(),
       [](const dd4hep::DetElement& a, const dd4hep::DetElement& b) {
         return (a.id() < b.id());
       });
  sort(tracker.begin(), tracker.end(),
       [](const dd4hep::DetElement& a, const dd4hep::DetElement& b) {
         return (a.id() < b.id());
       });
  det.clear();
  det = tracker;

  det.insert(det.end(), eCal.begin(), eCal.end());
  det.insert(det.end(), hCal.begin(), hCal.end());
  det.insert(det.end(), muon.begin(), muon.end());
}
