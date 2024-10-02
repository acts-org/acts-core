// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Digitization/DigitizationConfigurator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Zip.hpp"
#include "ActsExamples/Digitization/SmearingConfig.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

namespace {
/// @note This does not really compare if the configs are equal, therefore
/// it is no operator==. The contained std::function types cannot really
/// be checked for equality.
bool digiConfigMaybeEqual(ActsExamples::DigiComponentsConfig &a,
                          ActsExamples::DigiComponentsConfig &b) {
  // Check smearing config
  for (const auto &[as, bs] :
       Acts::zip(a.smearingDigiConfig, b.smearingDigiConfig)) {
    if (as.index != bs.index) {
      return false;
    }
  }
  // Check geometric config
  const auto &ag = a.geometricDigiConfig;
  const auto &bg = b.geometricDigiConfig;
  return (ag.indices == bg.indices && ag.segmentation == bg.segmentation &&
          ag.thickness == bg.thickness && ag.threshold == bg.threshold &&
          ag.digital == bg.digital);
}
}  // namespace

void ActsExamples::DigitizationConfigurator::operator()(
    const Acts::Surface *surface) {
  if (surface->associatedDetectorElement() != nullptr) {
    Acts::GeometryIdentifier geoId = surface->geometryId();
    auto dInputConfig = inputDigiComponents.find((geoId));
    if (dInputConfig != inputDigiComponents.end()) {
      // The output config, copy over the smearing part
      DigiComponentsConfig dOutputConfig;
      dOutputConfig.smearingDigiConfig = dInputConfig->smearingDigiConfig;

      if (!dInputConfig->geometricDigiConfig.indices.empty()) {
        // Copy over what can be done
        dOutputConfig.geometricDigiConfig.indices =
            dInputConfig->geometricDigiConfig.indices;
        dOutputConfig.geometricDigiConfig.thickness =
            dInputConfig->geometricDigiConfig.thickness;
        dOutputConfig.geometricDigiConfig.chargeSmearer =
            dInputConfig->geometricDigiConfig.chargeSmearer;
        dOutputConfig.geometricDigiConfig.digital =
            dInputConfig->geometricDigiConfig.digital;

        const Acts::SurfaceBounds &sBounds = surface->bounds();
        auto boundValues = sBounds.values();

        const auto &inputSegmentation =
            dInputConfig->geometricDigiConfig.segmentation;
        Acts::BinUtility outputSegmentation;

        switch (sBounds.type()) {
          // The module is a rectangle module
          case Acts::SurfaceBounds::eRectangle: {
            if (inputSegmentation.binningData()[0].binvalue ==
                Acts::BinningValue::binX) {
              Acts::ActsScalar minX = boundValues[Acts::RectangleBounds::eMinX];
              Acts::ActsScalar maxX = boundValues[Acts::RectangleBounds::eMaxX];
              unsigned int nBins = static_cast<unsigned int>(std::round(
                  (maxX - minX) / inputSegmentation.binningData()[0].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, static_cast<float>(minX), static_cast<float>(maxX),
                  Acts::open, Acts::BinningValue::binX);
            }
            if (inputSegmentation.binningData()[0].binvalue ==
                    Acts::BinningValue::binY ||
                inputSegmentation.dimensions() == 2) {
              unsigned int accessBin =
                  inputSegmentation.dimensions() == 2 ? 1 : 0;
              Acts::ActsScalar minY = boundValues[Acts::RectangleBounds::eMinY];
              Acts::ActsScalar maxY = boundValues[Acts::RectangleBounds::eMaxY];
              unsigned int nBins = static_cast<unsigned int>(
                  std::round((maxY - minY) /
                             inputSegmentation.binningData()[accessBin].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, static_cast<float>(minY), static_cast<float>(maxY),
                  Acts::open, Acts::BinningValue::binY);
            }
          } break;

          // The module is a trapezoid module
          case Acts::SurfaceBounds::eTrapezoid: {
            if (inputSegmentation.binningData()[0].binvalue ==
                Acts::BinningValue::binX) {
              Acts::ActsScalar maxX = std::max(
                  boundValues[Acts::TrapezoidBounds::eHalfLengthXnegY],
                  boundValues[Acts::TrapezoidBounds::eHalfLengthXposY]);
              unsigned int nBins = static_cast<unsigned int>(std::round(
                  2 * maxX / inputSegmentation.binningData()[0].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, -static_cast<float>(maxX), static_cast<float>(maxX),
                  Acts::open, Acts::BinningValue::binX);
            }
            if (inputSegmentation.binningData()[0].binvalue ==
                    Acts::BinningValue::binY ||
                inputSegmentation.dimensions() == 2) {
              unsigned int accessBin =
                  inputSegmentation.dimensions() == 2 ? 1 : 0;
              Acts::ActsScalar maxY =
                  boundValues[Acts::TrapezoidBounds::eHalfLengthY];
              unsigned int nBins = static_cast<unsigned int>(
                  std::round((2 * maxY) /
                             inputSegmentation.binningData()[accessBin].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, -static_cast<float>(maxY), static_cast<float>(maxY),
                  Acts::open, Acts::BinningValue::binY);
            }
          } break;

          // The module is an annulus module
          case Acts::SurfaceBounds::eAnnulus: {
            if (inputSegmentation.binningData()[0].binvalue ==
                Acts::BinningValue::binR) {
              Acts::ActsScalar minR = boundValues[Acts::AnnulusBounds::eMinR];
              Acts::ActsScalar maxR = boundValues[Acts::AnnulusBounds::eMaxR];
              unsigned int nBins = static_cast<unsigned int>(std::round(
                  (maxR - minR) / inputSegmentation.binningData()[0].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, static_cast<float>(minR), static_cast<float>(maxR),
                  Acts::open, Acts::BinningValue::binR);
            }
            if (inputSegmentation.binningData()[0].binvalue ==
                    Acts::BinningValue::binPhi ||
                inputSegmentation.dimensions() == 2) {
              unsigned int accessBin =
                  inputSegmentation.dimensions() == 2 ? 1 : 0;
              Acts::ActsScalar averagePhi =
                  boundValues[Acts::AnnulusBounds::eAveragePhi];
              Acts::ActsScalar minPhi =
                  averagePhi - boundValues[Acts::AnnulusBounds::eMinPhiRel];
              Acts::ActsScalar maxPhi =
                  averagePhi + boundValues[Acts::AnnulusBounds::eMaxPhiRel];
              unsigned int nBins = static_cast<unsigned int>(
                  std::round((maxPhi - minPhi) /
                             inputSegmentation.binningData()[accessBin].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, static_cast<float>(minPhi), static_cast<float>(maxPhi),
                  Acts::open, Acts::BinningValue::binPhi);
            }

          } break;

          // The module is a Disc Trapezoid
          case Acts::SurfaceBounds::eDiscTrapezoid: {
            Acts::ActsScalar minR =
                boundValues[Acts::DiscTrapezoidBounds::eMinR];
            Acts::ActsScalar maxR =
                boundValues[Acts::DiscTrapezoidBounds::eMaxR];

            if (inputSegmentation.binningData()[0].binvalue ==
                Acts::BinningValue::binR) {
              unsigned int nBins = static_cast<unsigned int>(std::round(
                  (maxR - minR) / inputSegmentation.binningData()[0].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, static_cast<float>(minR), static_cast<float>(maxR),
                  Acts::open, Acts::BinningValue::binR);
            }
            if (inputSegmentation.binningData()[0].binvalue ==
                    Acts::BinningValue::binPhi ||
                inputSegmentation.dimensions() == 2) {
              unsigned int accessBin =
                  inputSegmentation.dimensions() == 2 ? 1 : 0;
              Acts::ActsScalar hxMinR =
                  boundValues[Acts::DiscTrapezoidBounds::eHalfLengthXminR];
              Acts::ActsScalar hxMaxR =
                  boundValues[Acts::DiscTrapezoidBounds::eHalfLengthXmaxR];

              Acts::ActsScalar averagePhi =
                  boundValues[Acts::DiscTrapezoidBounds::eAveragePhi];
              Acts::ActsScalar alphaMinR = std::atan2(minR, hxMinR);
              Acts::ActsScalar alphaMaxR = std::atan2(maxR, hxMaxR);
              Acts::ActsScalar alpha = std::max(alphaMinR, alphaMaxR);
              unsigned int nBins = static_cast<unsigned int>(std::round(
                  2 * alpha / inputSegmentation.binningData()[accessBin].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, static_cast<float>(averagePhi - alpha),
                  static_cast<float>(averagePhi + alpha), Acts::open,
                  Acts::BinningValue::binPhi);
            }

          } break;

          case Acts::SurfaceBounds::eDisc: {
            if (inputSegmentation.binningData()[0].binvalue ==
                Acts::BinningValue::binR) {
              Acts::ActsScalar minR = boundValues[Acts::RadialBounds::eMinR];
              Acts::ActsScalar maxR = boundValues[Acts::RadialBounds::eMaxR];
              unsigned int nBins = static_cast<unsigned int>(std::round(
                  (maxR - minR) / inputSegmentation.binningData()[0].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, static_cast<float>(minR), static_cast<float>(maxR),
                  Acts::open, Acts::BinningValue::binR);
            }
            if (inputSegmentation.binningData()[0].binvalue ==
                    Acts::BinningValue::binPhi ||
                inputSegmentation.dimensions() == 2) {
              unsigned int accessBin =
                  inputSegmentation.dimensions() == 2 ? 1 : 0;

              Acts::ActsScalar averagePhi =
                  boundValues[Acts::RadialBounds::eAveragePhi];
              Acts::ActsScalar halfPhiSector =
                  boundValues[Acts::RadialBounds::eHalfPhiSector];
              Acts::ActsScalar minPhi = averagePhi - halfPhiSector;
              Acts::ActsScalar maxPhi = averagePhi + halfPhiSector;

              unsigned int nBins = static_cast<unsigned int>(
                  std::round((maxPhi - minPhi) /
                             inputSegmentation.binningData()[accessBin].step));
              outputSegmentation += Acts::BinUtility(
                  nBins, static_cast<float>(minPhi), static_cast<float>(maxPhi),
                  Acts::open, Acts::BinningValue::binPhi);
            }

          } break;

          default:
            break;
        }
        // Set the adapted segmentation class
        dOutputConfig.geometricDigiConfig.segmentation = outputSegmentation;
      }

      // Compactify the output map where possible
      if (compactify) {
        // Check for a representing volume configuration, insert if not
        // present
        Acts::GeometryIdentifier volGeoId =
            Acts::GeometryIdentifier().setVolume(geoId.volume());

        auto volRep = volumeLayerComponents.find(volGeoId);
        if (volRep != volumeLayerComponents.end() &&
            digiConfigMaybeEqual(dOutputConfig, volRep->second)) {
          // return if the volume representation already covers this one
          return;
        } else {
          volumeLayerComponents[volGeoId] = dOutputConfig;
          outputDigiComponents.push_back({volGeoId, dOutputConfig});
        }

        // Check for a representing layer configuration, insert if not present
        Acts::GeometryIdentifier volLayGeoId =
            Acts::GeometryIdentifier(volGeoId).setLayer(geoId.layer());
        auto volLayRep = volumeLayerComponents.find(volLayGeoId);

        if (volLayRep != volumeLayerComponents.end() &&
            digiConfigMaybeEqual(dOutputConfig, volLayRep->second)) {
          return;
        } else {
          volumeLayerComponents[volLayGeoId] = dOutputConfig;
          outputDigiComponents.push_back({volLayGeoId, dOutputConfig});
        }
      }

      // Insert into the output list
      outputDigiComponents.push_back({geoId, dOutputConfig});
    }
  }
}
