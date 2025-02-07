// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/Blueprint.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/BlueprintNode.hpp"
#include "Acts/Geometry/CylinderContainerBlueprintNode.hpp"
#include "Acts/Geometry/CylinderVolumeStack.hpp"
#include "Acts/Geometry/LayerBlueprintNode.hpp"
#include "Acts/Geometry/MaterialDesignatorBlueprintNode.hpp"
#include "Acts/Geometry/StaticBlueprintNode.hpp"
#include "Acts/Geometry/VolumeAttachmentStrategy.hpp"
#include "Acts/Geometry/VolumeResizeStrategy.hpp"
#include "Acts/Navigation/NavigationStream.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <fstream>
#include <random>
#include <utility>

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {
namespace {
using std::uniform_real_distribution;

// This is temporary!
void pseudoNavigation(const TrackingGeometry& trackingGeometry,
                      const GeometryContext& gctx, std::filesystem::path& path,
                      std::size_t runs, std::size_t substepsPerCm,
                      std::pair<double, double> etaRange,
                      Logging::Level logLevel) {
  using namespace Acts::UnitLiterals;

  ACTS_LOCAL_LOGGER(getDefaultLogger("pseudoNavigation", logLevel));

  std::ofstream csv{path};
  csv << "x,y,z,volume,boundary,sensitive,material" << std::endl;

  std::mt19937 rnd{42};

  std::uniform_real_distribution<> dist{-1, 1};
  std::uniform_real_distribution<> subStepDist{0.01, 0.99};

  double thetaMin = 2 * std::atan(std::exp(-etaRange.first));
  double thetaMax = 2 * std::atan(std::exp(-etaRange.second));
  std::uniform_real_distribution<> thetaDist{thetaMin, thetaMax};

  using namespace Acts::UnitLiterals;

  for (std::size_t run = 0; run < runs; run++) {
    Vector3 position = Vector3::Zero();

    double theta = thetaDist(rnd);
    double phi = 2 * std::numbers::pi * dist(rnd);

    Vector3 direction;
    direction[0] = std::sin(theta) * std::cos(phi);
    direction[1] = std::sin(theta) * std::sin(phi);
    direction[2] = std::cos(theta);

    ACTS_VERBOSE("start navigation " << run);
    ACTS_VERBOSE("pos: " << position.transpose());
    ACTS_VERBOSE("dir: " << direction.transpose());
    ACTS_VERBOSE(direction.norm());

    std::mt19937 rng{static_cast<unsigned int>(run)};

    const auto* volume = trackingGeometry.lowestTrackingVolume(gctx, position);
    assert(volume != nullptr);
    ACTS_VERBOSE(volume->volumeName());

    NavigationStream main;
    const TrackingVolume* currentVolume = volume;

    csv << run << "," << position[0] << "," << position[1] << ","
        << position[2];
    csv << "," << volume->geometryId().volume();
    csv << "," << volume->geometryId().boundary();
    csv << "," << volume->geometryId().sensitive();
    csv << "," << 0;
    csv << std::endl;

    ACTS_VERBOSE("start pseudo navigation");

    auto writeIntersection = [&](const Vector3& pos, const Surface& surface) {
      csv << run << "," << pos[0] << "," << pos[1] << "," << pos[2];
      csv << "," << surface.geometryId().volume();
      csv << "," << surface.geometryId().boundary();
      csv << "," << surface.geometryId().sensitive();
      csv << "," << (surface.surfaceMaterial() != nullptr ? 1 : 0);
      csv << std::endl;
    };

    for (std::size_t i = 0; i < 100; i++) {
      assert(currentVolume != nullptr);
      main = NavigationStream{};

      AppendOnlyNavigationStream navStream{main};
      currentVolume->initializeNavigationCandidates(
          {.position = position, .direction = direction}, navStream, logger());

      ACTS_VERBOSE(main.candidates().size() << " candidates");

      for (const auto& candidate : main.candidates()) {
        ACTS_VERBOSE(" -> " << candidate.surface().geometryId());
        ACTS_VERBOSE("    " << candidate.surface().toStream(gctx));
      }

      ACTS_VERBOSE("initializing candidates");
      main.initialize(gctx, {position, direction}, BoundaryTolerance::None());

      ACTS_VERBOSE(main.candidates().size() << " candidates remaining");

      for (const auto& candidate : main.candidates()) {
        ACTS_VERBOSE(" -> " << candidate.surface().geometryId());
        ACTS_VERBOSE("    " << candidate.surface().toStream(gctx));
      }

      if (main.currentCandidate().surface().isOnSurface(gctx, position,
                                                        direction)) {
        ACTS_VERBOSE(
            "Already on surface at initialization, skipping candidate");

        writeIntersection(position, main.currentCandidate().surface());

        if (!main.switchToNextCandidate()) {
          ACTS_WARNING("candidates exhausted unexpectedly");
          break;
        }
      }

      bool terminated = false;
      while (main.remainingCandidates() > 0) {
        const auto& candidate = main.currentCandidate();

        ACTS_VERBOSE(candidate.portal);
        ACTS_VERBOSE(candidate.intersection.position().transpose());

        ACTS_VERBOSE("moving to position: " << position.transpose() << " (r="
                                            << VectorHelpers::perp(position)
                                            << ")");

        Vector3 delta = candidate.intersection.position() - position;

        std::size_t substeps =
            std::max(1l, std::lround(delta.norm() / 10_cm * substepsPerCm));

        for (std::size_t j = 0; j < substeps; j++) {
          Vector3 subpos = position + subStepDist(rng) * delta;
          csv << run << "," << subpos[0] << "," << subpos[1] << ","
              << subpos[2];
          csv << "," << currentVolume->geometryId().volume();
          csv << ",0,0,0";  // zero boundary and sensitive ids
          csv << std::endl;
        }

        position = candidate.intersection.position();
        ACTS_VERBOSE("                 -> "
                     << position.transpose()
                     << " (r=" << VectorHelpers::perp(position) << ")");

        writeIntersection(position, candidate.surface());

        if (candidate.portal != nullptr) {
          ACTS_VERBOSE(
              "On portal: " << candidate.portal->surface().toStream(gctx));
          currentVolume =
              candidate.portal->resolveVolume(gctx, position, direction)
                  .value();

          if (currentVolume == nullptr) {
            ACTS_VERBOSE("switched to nullptr -> we're done");
            terminated = true;
          }
          break;

        } else {
          ACTS_VERBOSE("Not on portal");
        }

        main.switchToNextCandidate();
      }

      if (terminated) {
        ACTS_VERBOSE("Terminate pseudo navigation");
        break;
      }

      ACTS_VERBOSE("switched to " << currentVolume->volumeName());

      ACTS_VERBOSE("-----");
    }
  }
}

}  // namespace

void addBlueprint(Context& ctx) {
  auto m = ctx.get("main");

  auto blueprintNode =
      py::class_<BlueprintNode, std::shared_ptr<BlueprintNode>>(
          m, "BlueprintNode");

  auto rootNode =
      py::class_<Blueprint, BlueprintNode, std::shared_ptr<Blueprint>>(
          m, "Blueprint");

  rootNode
      .def(py::init<const Blueprint::Config&>())
      // Return value needs to be shared pointer because python otherwise
      // can't manage the lifetime
      .def(
          "construct",
          [](Blueprint& self, const BlueprintOptions& options,
             const GeometryContext& gctx,
             Logging::Level level) -> std::shared_ptr<TrackingGeometry> {
            return self.construct(options, gctx,
                                  *getDefaultLogger("Blueprint", level));
          },
          py::arg("options"), py::arg("gctx"),
          py::arg("level") = Logging::INFO);

  {
    auto c = py::class_<Blueprint::Config>(rootNode, "Config").def(py::init());
    ACTS_PYTHON_STRUCT_BEGIN(c, Blueprint::Config);
    ACTS_PYTHON_MEMBER(envelope);
    ACTS_PYTHON_MEMBER(geometryIdentifierHook);
    ACTS_PYTHON_STRUCT_END();
  }

  auto addContextManagerProtocol = []<typename class_>(class_& cls) {
    using type = typename class_::type;
    cls.def("__enter__", [](type& self) -> type& { return self; })
        .def("__exit__", [](type& /*self*/, const py::object& /*exc_type*/,
                            const py::object& /*exc_value*/,
                            const py::object& /*traceback*/) {
          // No action needed on exit
        });
  };

  auto addNodeMethods = [&blueprintNode](const std::string& name,
                                         auto&& callable, auto&&... args) {
    blueprintNode.def(name.c_str(), callable, args...)
        .def(("add" + name).c_str(), callable, args...);
  };

  blueprintNode
      .def("__str__",
           [](const BlueprintNode& self) {
             std::stringstream ss;
             ss << self;
             return ss.str();
           })
      .def("addChild", &BlueprintNode::addChild)
      .def_property_readonly("children",
                             py::overload_cast<>(&BlueprintNode::children))
      .def("clearChildren", &BlueprintNode::clearChildren)
      .def_property_readonly("name", &BlueprintNode::name)
      .def_property_readonly("depth", &BlueprintNode::depth)
      .def("graphviz", [](BlueprintNode& self, const py::object& fh) {
        std::stringstream ss;
        self.graphviz(ss);
        fh.attr("write")(ss.str());
      });

  py::class_<BlueprintOptions>(m, "BlueprintOptions")
      .def(py::init<>())
      .def_readwrite("defaultNavigationPolicyFactory",
                     &BlueprintOptions::defaultNavigationPolicyFactory);

  py::class_<BlueprintNode::MutableChildRange>(blueprintNode,
                                               "MutableChildRange")
      .def(
          "__iter__",
          [](BlueprintNode::MutableChildRange& self) {
            return py::make_iterator(self.begin(), self.end());
          },
          py::keep_alive<0, 1>())
      .def(
          "__getitem__",
          [](BlueprintNode::MutableChildRange& self,
             int i) -> Acts::BlueprintNode& {
            if (i < 0) {
              i += self.size();
            }
            return self.at(i);
          },
          py::return_value_policy::reference_internal)
      .def("__len__", [](const BlueprintNode::MutableChildRange& self) {
        return self.size();
      });

  auto staticNode =
      py::class_<Acts::StaticBlueprintNode, Acts::BlueprintNode,
                 std::shared_ptr<Acts::StaticBlueprintNode>>(
          m, "StaticBlueprintNode")
          .def(py::init([](const Transform3& transform,
                           const std::shared_ptr<VolumeBounds>& bounds,
                           const std::string& name) {
                 return std::make_shared<Acts::StaticBlueprintNode>(
                     std::make_unique<Acts::TrackingVolume>(transform, bounds,
                                                            name));
               }),
               py::arg("transform"), py::arg("bounds"),
               py::arg("name") = "undefined")
          .def_property("navigationPolicyFactory",
                        &Acts::StaticBlueprintNode::navigationPolicyFactory,
                        &Acts::StaticBlueprintNode::setNavigationPolicyFactory);

  addContextManagerProtocol(staticNode);

  addNodeMethods(
      "StaticVolume",
      [](BlueprintNode& self, const Transform3& transform,
         const std::shared_ptr<VolumeBounds>& bounds, const std::string& name) {
        auto node = std::make_shared<Acts::StaticBlueprintNode>(
            std::make_unique<Acts::TrackingVolume>(transform, bounds, name));
        self.addChild(node);
        return node;
      },
      py::arg("transform"), py::arg("bounds"), py::arg("name") = "undefined");

  auto cylNode =
      py::class_<Acts::CylinderContainerBlueprintNode, Acts::BlueprintNode,
                 std::shared_ptr<Acts::CylinderContainerBlueprintNode>>(
          m, "CylinderContainerBlueprintNode")
          .def(py::init<const std::string&, AxisDirection,
                        VolumeAttachmentStrategy, VolumeResizeStrategy>(),
               py::arg("name"), py::arg("direction"),
               py::arg("attachmentStrategy") = VolumeAttachmentStrategy::Gap,
               py::arg("resizeStrategy") = VolumeResizeStrategy::Gap)
          .def_property(
              "attachmentStrategy",
              &Acts::CylinderContainerBlueprintNode::attachmentStrategy,
              &Acts::CylinderContainerBlueprintNode::setAttachmentStrategy)
          .def_property(
              "resizeStrategy",
              &Acts::CylinderContainerBlueprintNode::resizeStrategy,
              &Acts::CylinderContainerBlueprintNode::setResizeStrategy)
          .def_property("direction",
                        &Acts::CylinderContainerBlueprintNode::direction,
                        &Acts::CylinderContainerBlueprintNode::setDirection);

  addContextManagerProtocol(cylNode);

  addNodeMethods(
      "CylinderContainer",
      [](BlueprintNode& self, const std::string& name,
         AxisDirection direction) {
        auto cylinder =
            std::make_shared<CylinderContainerBlueprintNode>(name, direction);
        self.addChild(cylinder);
        return cylinder;
      },
      py::arg("name"), py::arg("direction"));

  auto matNode =
      py::class_<MaterialDesignatorBlueprintNode, BlueprintNode,
                 std::shared_ptr<MaterialDesignatorBlueprintNode>>(
          m, "MaterialDesignatorBlueprintNode")
          .def(py::init<const std::string&>(), "name"_a)
          .def_property("binning", &MaterialDesignatorBlueprintNode::binning,
                        &MaterialDesignatorBlueprintNode::setBinning);

  addContextManagerProtocol(matNode);

  addNodeMethods(
      "Material",
      [](BlueprintNode& self, const std::string& name) {
        auto child = std::make_shared<MaterialDesignatorBlueprintNode>(name);
        self.addChild(child);
        return child;
      },
      "name"_a);

  auto layerNode =
      py::class_<Acts::LayerBlueprintNode, Acts::StaticBlueprintNode,
                 std::shared_ptr<Acts::LayerBlueprintNode>>(
          m, "LayerBlueprintNode")
          .def(py::init<const std::string&>(), py::arg("name"))
          .def_property_readonly("name", &Acts::LayerBlueprintNode::name)
          .def_property("surfaces", &Acts::LayerBlueprintNode::surfaces,
                        &Acts::LayerBlueprintNode::setSurfaces)
          .def_property("transform", &Acts::LayerBlueprintNode::transform,
                        &Acts::LayerBlueprintNode::setTransform)
          .def_property("envelope", &Acts::LayerBlueprintNode::envelope,
                        &Acts::LayerBlueprintNode::setEnvelope)
          .def_property("layerType", &Acts::LayerBlueprintNode::layerType,
                        &Acts::LayerBlueprintNode::setLayerType)
          .def_property("navigationPolicyFactory",
                        &Acts::LayerBlueprintNode::navigationPolicyFactory,
                        &Acts::LayerBlueprintNode::setNavigationPolicyFactory);

  py::enum_<Acts::LayerBlueprintNode::LayerType>(layerNode, "LayerType")
      .value("Cylinder", Acts::LayerBlueprintNode::LayerType::Cylinder)
      .value("Disc", Acts::LayerBlueprintNode::LayerType::Disc)
      .value("Plane", Acts::LayerBlueprintNode::LayerType::Plane);

  addContextManagerProtocol(layerNode);

  addNodeMethods(
      "Layer",
      [](BlueprintNode& self, const std::string& name) {
        auto child = std::make_shared<LayerBlueprintNode>(name);
        self.addChild(child);
        return child;
      },
      py::arg("name"));

  // TEMPORARY
  m.def("pseudoNavigation", &pseudoNavigation, "trackingGeometry"_a, "gctx"_a,
        "path"_a, "runs"_a, "substepsPerCm"_a = 2,
        "etaRange"_a = std::pair{-4.5, 4.5}, "logLevel"_a = Logging::INFO);
}

}  // namespace Acts::Python
