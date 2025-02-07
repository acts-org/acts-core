// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/DD4hepDetector/DD4hepDetector.hpp"

#include "Acts/Plugins/DD4hep/ConvertDD4hepDetector.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>

#include <DD4hep/DetElement.h>
#include <DD4hep/Detector.h>
#include <DD4hep/Handle.h>
#include <DD4hep/Volumes.h>
#include <Parsers/Printout.h>
#include <TError.h>

namespace ActsExamples {

DD4hepDetector::DD4hepDetector(const Config& cfg)
    : Detector(Acts::getDefaultLogger("DD4hepDetector", cfg.logLevel)),
      m_cfg(cfg) {
  if (m_cfg.xmlFileNames.empty()) {
    throw std::invalid_argument("Missing DD4hep XML filenames");
  }

  m_nominalGeometryContext = Acts::GeometryContext();

  m_detector = buildDD4hepGeometry();

  auto logger = Acts::getDefaultLogger("DD4hepConversion", m_cfg.logLevel);
  m_trackingGeometry = Acts::convertDD4hepDetector(
      m_detector->world(), *logger, m_cfg.bTypePhi, m_cfg.bTypeR, m_cfg.bTypeZ,
      m_cfg.envelopeR, m_cfg.envelopeZ, m_cfg.defaultLayerThickness,
      m_cfg.sortDetectors, m_nominalGeometryContext, m_cfg.materialDecorator,
      m_cfg.geometryIdentifierHook);
}

dd4hep::Detector& DD4hepDetector::dd4hepDetector() {
  return *m_detector;
}

std::shared_ptr<Acts::DD4hepFieldAdapter> DD4hepDetector::field() const {
  throw_assert(m_detector != nullptr, "Detector not initialized");
  return std::make_shared<Acts::DD4hepFieldAdapter>(m_detector->field());
}

TGeoNode& DD4hepDetector::tgeoGeometry() {
  return *m_detector->world().placement().ptr();
}

std::unique_ptr<dd4hep::Detector> DD4hepDetector::buildDD4hepGeometry() const {
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

  std::unique_ptr<dd4hep::Detector> detector =
      dd4hep::Detector::make_unique(m_cfg.name);
  for (const auto& file : m_cfg.xmlFileNames) {
    detector->fromCompact(file);
  }
  detector->volumeManager();
  detector->apply("DD4hepVolumeManager", 0, nullptr);

  // restore the logging
  gErrorIgnoreLevel = old_gErrorIgnoreLevel;
  std::cout.clear();

  return detector;
}

}  // namespace ActsExamples

void ActsExamples::sortFCChhDetElements(std::vector<dd4hep::DetElement>& det) {
  std::vector<dd4hep::DetElement> tracker;
  std::vector<dd4hep::DetElement> eCal;
  std::vector<dd4hep::DetElement> hCal;
  std::vector<dd4hep::DetElement> muon;
  for (const auto& detElement : det) {
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
  std::ranges::sort(
      muon, [](const dd4hep::DetElement& a, const dd4hep::DetElement& b) {
        return (a.id() < b.id());
      });
  std::ranges::sort(
      eCal, [](const dd4hep::DetElement& a, const dd4hep::DetElement& b) {
        return (a.id() < b.id());
      });
  std::ranges::sort(
      hCal, [](const dd4hep::DetElement& a, const dd4hep::DetElement& b) {
        return (a.id() < b.id());
      });
  std::ranges::sort(
      tracker, [](const dd4hep::DetElement& a, const dd4hep::DetElement& b) {
        return (a.id() < b.id());
      });
  det.clear();
  det = tracker;

  det.insert(det.end(), eCal.begin(), eCal.end());
  det.insert(det.end(), hCal.begin(), hCal.end());
  det.insert(det.end(), muon.begin(), muon.end());
}
