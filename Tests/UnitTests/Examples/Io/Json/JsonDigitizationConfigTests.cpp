// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/detail/GenerateParameters.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/Digitization/Smearers.hpp"
#include "ActsExamples/Io/Json/JsonDigitizationConfig.hpp"
#include "ActsFatras/Digitization/UncorrelatedHitSmearer.hpp"

#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using namespace Acts;

namespace {
template <typename generator_t>
struct Fixture {
  generator_t rng;
  // identifiers
  Acts::GeometryIdentifier gid;
  ActsFatras::Barcode pid;
  // geometry information
  std::shared_ptr<Acts::Surface> surface;
  Acts::GeometryContext geoCtx;
  // local and global track parameters
  Acts::BoundVector boundParams;
  Acts::FreeVector freeParams;
  // hit information
  ActsFatras::Hit hit;

  Fixture(std::uint64_t rngSeed, std::shared_ptr<Acts::Surface> surf)
      : rng(rngSeed),
        gid(Acts::GeometryIdentifier().setVolume(1).setLayer(2).setSensitive(
            3)),
        pid(ActsFatras::Barcode().setVertexPrimary(12).setParticle(23)),
        surface(std::move(surf)) {
    using namespace Acts::UnitLiterals;
    using Acts::VectorHelpers::makeVector4;

    surface->assignGeometryId(gid);

    // generate random track parameters
    auto [par, cov] =
        Acts::detail::Test::generateBoundParametersCovariance(rng, {});
    boundParams = par;

    freeParams =
        Acts::transformBoundToFreeParameters(*surface, geoCtx, boundParams);

    // construct hit from free parameters
    Acts::Vector4 r4;
    r4.segment<3>(Acts::ePos0) = freeParams.segment<3>(Acts::eFreePos0);
    r4[Acts::eTime] = freeParams[Acts::eFreeTime];
    // construct 4-momentum vector assuming m=0
    Acts::Vector4 p4;
    p4.segment<3>(Acts::eMom0) =
        freeParams.segment<3>(Acts::eFreeDir0).normalized();
    p4[Acts::eEnergy] = 1;
    p4 *= std::abs(1_e / freeParams[Acts::eFreeQOverP]);
    // same 4-momentum before/after hit
    hit = ActsFatras::Hit(gid, pid, r4, p4, p4, 13);
  }
};
}  // namespace

BOOST_AUTO_TEST_SUITE(JsonDigitizationConfig)

BOOST_AUTO_TEST_CASE(GaussianSmearing) {
  nlohmann::json djson = nlohmann::json::parse(R"(
    {
    "acts-geometry-hierarchy-map" : {
    "format-version" : 0,
    "value-identifier" : "digitization-configuration"
    },

  "entries"
      : [
        {
           "volume" : 1,
           "value" : {
            "smearing" : [
              {"index" : 0, "mean" : 0.0, "stddev" : 0.05, "type" : "Gauss", "forcePositiveValues" : true}


            ]
          }
        }
      ]
})");
  double radius = 5.;
  double halfZ = 8.;
  Fixture<ActsExamples::RandomEngine> f(
      123567,
      Acts::Surface::makeShared<Acts::StrawSurface>(
          Acts::Transform3(Acts::Translation3(0., 0., 0.)), radius, halfZ));

  // Get the smearing configuration from the json object
  auto digiConfig =
      ActsExamples::DigiConfigConverter("digitization-configuration")
          .fromJson(djson);
  ActsFatras::BoundParametersSmearer<ActsExamples::RandomEngine, 1u> s;

  for (auto& el : digiConfig) {
    for (auto& smearing : el.smearingDigiConfig) {
      // check if the forcePositiveValue parameter is successfully parsed
      BOOST_CHECK(smearing.forcePositiveValues);
      std::fill(std::begin(s.indices), std::end(s.indices),
                static_cast<Acts::BoundIndices>(smearing.index));
      std::fill(std::begin(s.smearFunctions), std::end(s.smearFunctions),
                smearing.smearFunction);
      std::fill(std::begin(s.forcePositive), std::end(s.forcePositive),
                smearing.forcePositiveValues);
    }
  }

  auto ret = s(f.rng, f.hit, *f.surface, f.geoCtx);

  BOOST_CHECK(ret.ok());
  auto [par, cov] = ret.value();
  for (std::size_t i = 0; i < s.indices.size(); i++) {
    BOOST_TEST_INFO("Comparing smeared measurement "
                    << i << " originating from bound parameter "
                    << s.indices[i]);
    double ref = f.boundParams[s.indices[i]];
    if (s.forcePositive[i]) {
      ref = std::abs(ref);
    }
    CHECK_CLOSE_REL(par[i], ref, 0.15);
  }
}

BOOST_AUTO_TEST_CASE(DigitizationConfigRoundTrip) {
  std::ofstream out;

  // As all SurfaceBounds have the same streaming API only a one is
  // tested here, all others are tests are identical

  ActsExamples::DigiComponentsConfig dcf;

  ActsExamples::GeometricConfig gdc;

  Acts::BinUtility segmentation;
  segmentation +=
      Acts::BinUtility(336, -8.4, 8.4, Acts::open, Acts::AxisDirection::AxisX);
  segmentation +=
      Acts::BinUtility(1280, -36, 36, Acts::open, Acts::AxisDirection::AxisY);

  gdc.segmentation = segmentation;
  gdc.threshold = 0.01;
  gdc.thickness = 0.15;
  gdc.indices = {Acts::eBoundLoc0, Acts::eBoundLoc1};
  gdc.chargeSmearer = ActsExamples::Digitization::Gauss(1.0);

  ActsExamples::DigiComponentsConfig dcRef;
  dcRef.geometricDigiConfig = gdc;

  nlohmann::json dcJsonOut(dcRef);
  out.open("DigiComponentsConfig.json");
  out << dcJsonOut.dump(2);
  out.close();

  auto in = std::ifstream("DigiComponentsConfig.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json dcJsonIn;
  in >> dcJsonIn;
  in.close();

  ActsExamples::DigiComponentsConfig dcTest(dcJsonIn);
  BOOST_CHECK(dcTest.geometricDigiConfig.indices ==
              dcRef.geometricDigiConfig.indices);
  BOOST_CHECK_EQUAL(dcTest.geometricDigiConfig.segmentation.dimensions(),
                    dcRef.geometricDigiConfig.segmentation.dimensions());
}

BOOST_AUTO_TEST_SUITE_END()
