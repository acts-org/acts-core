// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/Vertexing/AdaptiveMultiVertexFinderAlgorithm.hpp"
#include "ActsExamples/Vertexing/IterativeVertexFinderAlgorithm.hpp"
#include "ActsExamples/Vertexing/SingleSeedVertexFinderAlgorithm.hpp"
#include "ActsExamples/Vertexing/VertexFitterAlgorithm.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

namespace Acts::Python {

void addVertexing(Context& ctx) {
  using Seeder = ActsExamples::AdaptiveMultiVertexFinderAlgorithm::SeedFinder;
  auto mex = ctx.get("examples");
  auto& m = ctx.get("main");

  py::enum_<Seeder>(m, "VertexSeedFinder")
      .value("TruthSeeder", Seeder::TruthSeeder)
      .value("GaussianSeeder", Seeder::GaussianSeeder)
      .value("AdaptiveGridSeeder", Seeder::AdaptiveGridSeeder);

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::AdaptiveMultiVertexFinderAlgorithm, mex,
      "AdaptiveMultiVertexFinderAlgorithm", inputTrackParameters,
      inputTruthParticles, inputTruthVertices, outputProtoVertices,
      outputVertices, seedFinder, bField, minWeight, doSmoothing, maxIterations,
      useTime, tracksMaxZinterval, initialVariances, doFullSplitting,
      tracksMaxSignificance, maxMergeVertexSignificance, spatialBinExtent,
      temporalBinExtent);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::IterativeVertexFinderAlgorithm,
                                mex, "IterativeVertexFinderAlgorithm",
                                inputTrackParameters, outputProtoVertices,
                                outputVertices, bField, maxIterations);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::VertexFitterAlgorithm, mex,
                                "VertexFitterAlgorithm", inputTrackParameters,
                                inputProtoVertices, outputVertices, bField,
                                doConstrainedFit, constraintPos, constraintCov);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::SingleSeedVertexFinderAlgorithm,
                                mex, "SingleSeedVertexFinderAlgorithm",
                                inputSpacepoints, outputVertices);
}

}  // namespace Acts::Python
