// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/GeoModel/GeoModelDetectorElementITk.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <GeoModelKernel/GeoBox.h>
#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoLogVol.h>
#include <GeoModelKernel/GeoMaterial.h>

BOOST_AUTO_TEST_SUITE(GeoModelPlugin)

BOOST_AUTO_TEST_CASE(ITkIdentifierTests) {
  auto test = [](int hw, int bec, int lw, int em, int pm, int side) {
    Acts::ITkIdentifier id(hw, bec, lw, em, pm, side);
    BOOST_CHECK_EQUAL(id.hardware(), hw);
    BOOST_CHECK_EQUAL(id.barrelEndcap(), bec);
    BOOST_CHECK_EQUAL(id.layerWheel(), lw);
    BOOST_CHECK_EQUAL(id.etaModule(), em);
    BOOST_CHECK_EQUAL(id.phiModule(), pm);
    BOOST_CHECK_EQUAL(id.side(), side);
  };

  for (int hw : {0, 1}) {
    for (int bec : {-2, 0, 2}) {
      for (int lw : {0, 10}) {
        for (int em : {-10, 0, 10}) {
          for (int pm : {10, 0}) {
            for (int side : {0, 1}) {
              test(hw, bec, lw, em, pm, side);
            }
          }
        }
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(GeoModelDetectorElementConstruction) {
  Acts::GeometryContext gctx{};

  auto material = GeoIntrusivePtr(new GeoMaterial("Material", 1.0));
  auto box = GeoIntrusivePtr(new GeoBox(100, 200, 2));
  auto log = GeoIntrusivePtr(new GeoLogVol("LogVolumeXY", box, material));
  auto fphys = GeoIntrusivePtr(new GeoFullPhysVol(log));
  auto rBounds = std::make_shared<Acts::RectangleBounds>(100, 200);

  auto element =
      Acts::GeoModelDetectorElement::createDetectorElement<Acts::PlaneSurface>(
          fphys, rBounds, Acts::Transform3::Identity(), 2.0);

  const int hardware = 0, barrelEndcap = -2, layerWheel = 100, phiModule = 200,
            etaModule = 300, side = 1;

  auto [itkElement, _] = Acts::GeoModelDetectorElementITk::convertFromGeomodel(
      element, element->surface().getSharedPtr(), gctx, hardware, barrelEndcap,
      layerWheel, etaModule, phiModule, side);

  BOOST_CHECK_EQUAL(element->surface().type(), itkElement->surface().type());
  BOOST_CHECK_EQUAL(element->surface().bounds().type(),
                    itkElement->surface().bounds().type());
  BOOST_CHECK_NE(element->surface().associatedDetectorElement(),
                 itkElement->surface().associatedDetectorElement());
  BOOST_CHECK_EQUAL(itkElement->identifier().barrelEndcap(), barrelEndcap);
  BOOST_CHECK_EQUAL(itkElement->identifier().hardware(), hardware);
  BOOST_CHECK_EQUAL(itkElement->identifier().layerWheel(), layerWheel);
  BOOST_CHECK_EQUAL(itkElement->identifier().phiModule(), phiModule);
  BOOST_CHECK_EQUAL(itkElement->identifier().etaModule(), etaModule);
  BOOST_CHECK_EQUAL(itkElement->identifier().side(), side);
}

BOOST_AUTO_TEST_SUITE_END()
