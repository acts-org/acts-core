// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/BoostTrackBuilding.hpp"
#include "Acts/Plugins/ExaTrkX/CugraphTrackBuilding.hpp"
#include "Acts/Plugins/ExaTrkX/ExaTrkXPipeline.hpp"
#include "Acts/Plugins/ExaTrkX/OnnxEdgeClassifier.hpp"
#include "Acts/Plugins/ExaTrkX/OnnxMetricLearning.hpp"
#include "Acts/Plugins/ExaTrkX/TorchEdgeClassifier.hpp"
#include "Acts/Plugins/ExaTrkX/TorchMetricLearning.hpp"
#include "Acts/Plugins/ExaTrkX/TorchTruthGraphMetricsHook.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/TrackFindingExaTrkX/PrototracksToParameters.hpp"
#include "ActsExamples/TrackFindingExaTrkX/TrackFindingAlgorithmExaTrkX.hpp"
#include "ActsExamples/TrackFindingExaTrkX/TrackFindingFromPrototrackAlgorithm.hpp"
#include "ActsExamples/TrackFindingExaTrkX/TruthGraphBuilder.hpp"

#include <memory>

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

namespace Acts::Python {

void addExaTrkXTrackFinding(Context &ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  {
    using C = Acts::GraphConstructionBase;
    auto c = py::class_<C, std::shared_ptr<C>>(mex, "GraphConstructionBase");
  }
  {
    using C = Acts::EdgeClassificationBase;
    auto c = py::class_<C, std::shared_ptr<C>>(mex, "EdgeClassificationBase");
  }
  {
    using C = Acts::TrackBuildingBase;
    auto c = py::class_<C, std::shared_ptr<C>>(mex, "TrackBuildingBase");
  }

#ifdef ACTS_EXATRKX_TORCH_BACKEND
  {
    using Alg = Acts::TorchMetricLearning;
    using Config = Alg::Config;

    auto alg =
        py::class_<Alg, Acts::GraphConstructionBase, std::shared_ptr<Alg>>(
            mex, "TorchMetricLearning")
            .def(py::init([](const Config &c, Logging::Level lvl) {
                   return std::make_shared<Alg>(
                       c, getDefaultLogger("MetricLearning", lvl));
                 }),
                 py::arg("config"), py::arg("level"))
            .def_property_readonly("config", &Alg::config);

    auto c = py::class_<Config>(alg, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(modelPath);
    ACTS_PYTHON_MEMBER(numFeatures);
    ACTS_PYTHON_MEMBER(embeddingDim);
    ACTS_PYTHON_MEMBER(rVal);
    ACTS_PYTHON_MEMBER(knnVal);
    ACTS_PYTHON_MEMBER(deviceID);
    ACTS_PYTHON_STRUCT_END();
  }
  {
    using Alg = Acts::TorchEdgeClassifier;
    using Config = Alg::Config;

    auto alg =
        py::class_<Alg, Acts::EdgeClassificationBase, std::shared_ptr<Alg>>(
            mex, "TorchEdgeClassifier")
            .def(py::init([](const Config &c, Logging::Level lvl) {
                   return std::make_shared<Alg>(
                       c, getDefaultLogger("EdgeClassifier", lvl));
                 }),
                 py::arg("config"), py::arg("level"))
            .def_property_readonly("config", &Alg::config);

    auto c = py::class_<Config>(alg, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(modelPath);
    ACTS_PYTHON_MEMBER(numFeatures);
    ACTS_PYTHON_MEMBER(cut);
    ACTS_PYTHON_MEMBER(nChunks);
    ACTS_PYTHON_MEMBER(undirected);
    ACTS_PYTHON_MEMBER(deviceID);
    ACTS_PYTHON_STRUCT_END();
  }
  {
    using Alg = Acts::BoostTrackBuilding;

    auto alg = py::class_<Alg, Acts::TrackBuildingBase, std::shared_ptr<Alg>>(
                   mex, "BoostTrackBuilding")
                   .def(py::init([](Logging::Level lvl) {
                          return std::make_shared<Alg>(
                              getDefaultLogger("EdgeClassifier", lvl));
                        }),
                        py::arg("level"));
  }
#endif

#ifdef ACTS_EXATRKX_ONNX_BACKEND
  {
    using Alg = Acts::OnnxMetricLearning;
    using Config = Alg::Config;

    auto alg =
        py::class_<Alg, Acts::GraphConstructionBase, std::shared_ptr<Alg>>(
            mex, "OnnxMetricLearning")
            .def(py::init([](const Config &c, Logging::Level lvl) {
                   return std::make_shared<Alg>(
                       c, getDefaultLogger("MetricLearning", lvl));
                 }),
                 py::arg("config"), py::arg("level"))
            .def_property_readonly("config", &Alg::config);

    auto c = py::class_<Config>(alg, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(modelPath);
    ACTS_PYTHON_MEMBER(spacepointFeatures);
    ACTS_PYTHON_MEMBER(embeddingDim);
    ACTS_PYTHON_MEMBER(rVal);
    ACTS_PYTHON_MEMBER(knnVal);
    ACTS_PYTHON_STRUCT_END();
  }
  {
    using Alg = Acts::OnnxEdgeClassifier;
    using Config = Alg::Config;

    auto alg =
        py::class_<Alg, Acts::EdgeClassificationBase, std::shared_ptr<Alg>>(
            mex, "OnnxEdgeClassifier")
            .def(py::init([](const Config &c, Logging::Level lvl) {
                   return std::make_shared<Alg>(
                       c, getDefaultLogger("EdgeClassifier", lvl));
                 }),
                 py::arg("config"), py::arg("level"))
            .def_property_readonly("config", &Alg::config);

    auto c = py::class_<Config>(alg, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(modelPath);
    ACTS_PYTHON_MEMBER(cut);
    ACTS_PYTHON_STRUCT_END();
  }
  {
    using Alg = Acts::CugraphTrackBuilding;

    auto alg = py::class_<Alg, Acts::TrackBuildingBase, std::shared_ptr<Alg>>(
                   mex, "CugraphTrackBuilding")
                   .def(py::init([](Logging::Level lvl) {
                          return std::make_shared<Alg>(
                              getDefaultLogger("EdgeClassifier", lvl));
                        }),
                        py::arg("level"));
  }
#endif

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::TruthGraphBuilder, mex, "TruthGraphBuilder",
      inputSpacePoints, inputSimHits, inputParticles,
      inputMeasurementSimHitsMap, inputMeasurementParticlesMap, outputGraph,
      targetMinPT, targetMinSize, uniqueModules);

  py::enum_<TrackFindingAlgorithmExaTrkX::NodeFeature>(mex, "NodeFeature")
      .value("R", TrackFindingAlgorithmExaTrkX::NodeFeature::eR)
      .value("Phi", TrackFindingAlgorithmExaTrkX::NodeFeature::ePhi)
      .value("Z", TrackFindingAlgorithmExaTrkX::NodeFeature::eZ)
      .value("X", TrackFindingAlgorithmExaTrkX::NodeFeature::eX)
      .value("Y", TrackFindingAlgorithmExaTrkX::NodeFeature::eY)
      .value("Eta", TrackFindingAlgorithmExaTrkX::NodeFeature::eEta)
      .value("ClusterX", TrackFindingAlgorithmExaTrkX::NodeFeature::eClusterX)
      .value("ClusterY", TrackFindingAlgorithmExaTrkX::NodeFeature::eClusterY)
      .value("CellCount", TrackFindingAlgorithmExaTrkX::NodeFeature::eCellCount)
      .value("CellSum", TrackFindingAlgorithmExaTrkX::NodeFeature::eCellSum)
      .value("Cluster1R", TrackFindingAlgorithmExaTrkX::NodeFeature::eCluster1R)
      .value("Cluster2R", TrackFindingAlgorithmExaTrkX::NodeFeature::eCluster2R)
      .value("Cluster1Phi",
             TrackFindingAlgorithmExaTrkX::NodeFeature::eCluster1Phi)
      .value("Cluster2Phi",
             TrackFindingAlgorithmExaTrkX::NodeFeature::eCluster2Phi)
      .value("Cluster1Z", TrackFindingAlgorithmExaTrkX::NodeFeature::eCluster1Z)
      .value("Cluster2Z", TrackFindingAlgorithmExaTrkX::NodeFeature::eCluster2Z)
      .value("Cluster1Eta",
             TrackFindingAlgorithmExaTrkX::NodeFeature::eCluster1Eta)
      .value("Cluster2Eta",
             TrackFindingAlgorithmExaTrkX::NodeFeature::eCluster2Eta);

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::TrackFindingAlgorithmExaTrkX, mex,
                                "TrackFindingAlgorithmExaTrkX",
                                inputSpacePoints, inputClusters,
                                inputTruthGraph, outputProtoTracks, outputGraph,
                                graphConstructor, edgeClassifiers, trackBuilder,
                                nodeFeatures, featureScales, filterShortTracks);

  {
    auto cls =
        py::class_<Acts::ExaTrkXHook, std::shared_ptr<Acts::ExaTrkXHook>>(
            mex, "ExaTrkXHook");
  }

  {
    using Class = Acts::TorchTruthGraphMetricsHook;

    auto cls = py::class_<Class, Acts::ExaTrkXHook, std::shared_ptr<Class>>(
                   mex, "TorchTruthGraphMetricsHook")
                   .def(py::init([](const std::vector<std::int64_t> &g,
                                    Logging::Level lvl) {
                     return std::make_shared<Class>(
                         g, getDefaultLogger("PipelineHook", lvl));
                   }));
  }

  {
    using Class = Acts::ExaTrkXPipeline;

    auto cls =
        py::class_<Class, std::shared_ptr<Class>>(mex, "ExaTrkXPipeline")
            .def(py::init(
                     [](std::shared_ptr<GraphConstructionBase> g,
                        std::vector<std::shared_ptr<EdgeClassificationBase>> e,
                        std::shared_ptr<TrackBuildingBase> t,
                        Logging::Level lvl) {
                       return std::make_shared<Class>(
                           g, e, t, getDefaultLogger("MetricLearning", lvl));
                     }),
                 py::arg("graphConstructor"), py::arg("edgeClassifiers"),
                 py::arg("trackBuilder"), py::arg("level"))
            .def("run", &ExaTrkXPipeline::run, py::arg("features"),
                 py::arg("spacepoints"), py::arg("hook") = Acts::ExaTrkXHook{},
                 py::arg("timing") = nullptr);
  }

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::PrototracksToParameters, mex, "PrototracksToParameters",
      inputProtoTracks, inputSpacePoints, outputSeeds, outputParameters,
      outputProtoTracks, geometry, magneticField, buildTightSeeds);

  ACTS_PYTHON_DECLARE_ALGORITHM(
      ActsExamples::TrackFindingFromPrototrackAlgorithm, mex,
      "TrackFindingFromPrototrackAlgorithm", inputProtoTracks,
      inputMeasurements, inputSourceLinks, inputInitialTrackParameters,
      outputTracks, measurementSelectorCfg, trackingGeometry, magneticField,
      findTracks, tag);
}

}  // namespace Acts::Python
