// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/CuboidalContainerBuilder.hpp"
#include "Acts/Detector/CylindricalContainerBuilder.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorBuilder.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/DetectorVolumeBuilder.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/IndexedRootVolumeFinderBuilder.hpp"
#include "Acts/Detector/KdtSurfacesProvider.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Detector/VolumeStructureBuilder.hpp"
#include "Acts/Detector/interface/IDetectorBuilder.hpp"
#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Detector/interface/IExternalStructureBuilder.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Detector/interface/IInternalStructureBuilder.hpp"
#include "Acts/Detector/interface/IRootVolumeFinderBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeStack.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Geometry/PortalLinkBase.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingGeometryVisitor.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Geometry/VolumeAttachmentStrategy.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Geometry/VolumeResizeStrategy.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/RangeXD.hpp"
#include "Acts/Visualization/ViewConfig.hpp"
#include "ActsExamples/Geometry/VolumeAssociationTest.hpp"

#include <array>
#include <memory>
#include <numbers>
#include <unordered_map>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace {
struct GeometryIdentifierHookBinding : public Acts::GeometryIdentifierHook {
  py::object callable;

  Acts::GeometryIdentifier decorateIdentifier(
      Acts::GeometryIdentifier identifier,
      const Acts::Surface& surface) const override {
    return callable(identifier, surface.getSharedPtr())
        .cast<Acts::GeometryIdentifier>();
  }
};

struct MaterialSurfaceSelector {
  std::vector<const Acts::Surface*> surfaces = {};

  /// @param surface is the test surface
  void operator()(const Acts::Surface* surface) {
    if (surface->surfaceMaterial() != nullptr &&
        !rangeContainsValue(surfaces, surface)) {
      surfaces.push_back(surface);
    }
  }
};

struct IdentifierSurfacesCollector {
  std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*> surfaces;
  /// @param surface is the test surface
  void operator()(const Acts::Surface* surface) {
    surfaces[surface->geometryId()] = surface;
  }
};

#define _INVOKE(method, name, arg)                                  \
  pybind11::gil_scoped_acquire gil;                                 \
  pybind11::function override = pybind11::get_override(this, name); \
  if (!override) {                                                  \
    method(arg);                                                    \
    return;                                                         \
  }                                                                 \
  override(&arg);

// We only implement the mutable visitor here, because pybind11 always casts
// away const ness in any case
class PyTrackingGeometryVisitor : public Acts::TrackingGeometryMutableVisitor {
 public:
  void visitVolume(Acts::TrackingVolume& volume) override {
    _INVOKE(Acts::TrackingGeometryMutableVisitor::visitVolume, "visitVolume",
            volume);
  }

  void visitPortal(Acts::Portal& portal) override {
    _INVOKE(Acts::TrackingGeometryMutableVisitor::visitPortal, "visitPortal",
            portal);
  }

  void visitLayer(Acts::Layer& layer) override {
    _INVOKE(Acts::TrackingGeometryMutableVisitor::visitLayer, "visitLayer",
            layer);
  }

  void visitSurface(Acts::Surface& surface) override {
    _INVOKE(Acts::TrackingGeometryMutableVisitor::visitSurface, "visitSurface",
            surface);
  }
};

#undef _INVOKE

}  // namespace

namespace Acts::Python {

void addBlueprint(Context& ctx);

void addGeometry(Context& ctx) {
  auto m = ctx.get("main");
  {
    py::class_<Acts::GeometryIdentifier>(m, "GeometryIdentifier")
        .def(py::init<>())
        .def(py::init<Acts::GeometryIdentifier::Value>())

        .def_property("volume", &Acts::GeometryIdentifier::volume,
                      &Acts::GeometryIdentifier::setVolume)
        .def_property("layer", &Acts::GeometryIdentifier::layer,
                      &Acts::GeometryIdentifier::setLayer)
        .def_property("boundary", &Acts::GeometryIdentifier::boundary,
                      &Acts::GeometryIdentifier::setBoundary)
        .def_property("approach", &Acts::GeometryIdentifier::approach,
                      &Acts::GeometryIdentifier::setApproach)
        .def_property("sensitive", &Acts::GeometryIdentifier::sensitive,
                      &Acts::GeometryIdentifier::setSensitive)
        .def_property("extra", &Acts::GeometryIdentifier::extra,
                      &Acts::GeometryIdentifier::setExtra)
        .def_property_readonly("value", &Acts::GeometryIdentifier::value)
        .def("__str__", [](const Acts::GeometryIdentifier& self) {
          std::stringstream ss;
          ss << self;
          return ss.str();
        });
  }

  {
    py::class_<Acts::Surface, std::shared_ptr<Acts::Surface>>(m, "Surface")
        // Can't bind directly because GeometryObject is virtual base of Surface
        .def_property_readonly(
            "geometryId", [](const Surface& self) { return self.geometryId(); })
        .def("center", &Surface::center)
        .def_property_readonly("type", &Surface::type)
        .def("visualize", &Surface::visualize)
        .def_property_readonly("surfaceMaterial",
                               &Acts::Surface::surfaceMaterialSharedPtr);
  }

  {
    py::enum_<Acts::Surface::SurfaceType>(m, "SurfaceType")
        .value("Cone", Acts::Surface::SurfaceType::Cone)
        .value("Cylinder", Acts::Surface::SurfaceType::Cylinder)
        .value("Disc", Acts::Surface::SurfaceType::Disc)
        .value("Perigee", Acts::Surface::SurfaceType::Perigee)
        .value("Plane", Acts::Surface::SurfaceType::Plane)
        .value("Straw", Acts::Surface::SurfaceType::Straw)
        .value("Curvilinear", Acts::Surface::SurfaceType::Curvilinear)
        .value("Other", Acts::Surface::SurfaceType::Other);
  }

  {
    py::enum_<Acts::VolumeBounds::BoundsType>(m, "VolumeBoundsType")
        .value("Cone", Acts::VolumeBounds::BoundsType::eCone)
        .value("Cuboid", Acts::VolumeBounds::BoundsType::eCuboid)
        .value("CutoutCylinder",
               Acts::VolumeBounds::BoundsType::eCutoutCylinder)
        .value("Cylinder", Acts::VolumeBounds::BoundsType::eCylinder)
        .value("GenericCuboid", Acts::VolumeBounds::BoundsType::eGenericCuboid)
        .value("Trapezoid", Acts::VolumeBounds::BoundsType::eTrapezoid)
        .value("Other", Acts::VolumeBounds::BoundsType::eOther);
  }

  {
    py::class_<Acts::TrackingGeometry, std::shared_ptr<Acts::TrackingGeometry>>(
        m, "TrackingGeometry")
        .def(py::init([](const MutableTrackingVolumePtr& volPtr,
                         std::shared_ptr<const IMaterialDecorator> matDec,
                         const GeometryIdentifierHook& hook,
                         Acts::Logging::Level level) {
          auto logger = Acts::getDefaultLogger("TrackingGeometry", level);
          auto trkGeo = std::make_shared<Acts::TrackingGeometry>(
              volPtr, matDec.get(), hook, *logger);
          return trkGeo;
        }))
        .def("visitSurfaces",
             [](Acts::TrackingGeometry& self, py::function& func) {
               self.visitSurfaces(func);
             })
        .def("geoIdSurfaceMap", &Acts::TrackingGeometry::geoIdSurfaceMap)
        .def("extractMaterialSurfaces",
             [](Acts::TrackingGeometry& self) {
               MaterialSurfaceSelector selector;
               self.visitSurfaces(selector, false);
               return selector.surfaces;
             })
        .def_property_readonly(
            "highestTrackingVolume",
            &Acts::TrackingGeometry::highestTrackingVolumePtr)
        .def("visualize", &Acts::TrackingGeometry::visualize, py::arg("helper"),
             py::arg("gctx"), py::arg("viewConfig") = s_viewVolume,
             py::arg("portalViewConfig") = s_viewPortal,
             py::arg("sensitiveViewConfig") = s_viewSensitive)
        .def("apply", py::overload_cast<TrackingGeometryMutableVisitor&>(
                          &TrackingGeometry::apply));
  }

  {
    py::class_<Acts::VolumeBounds, std::shared_ptr<Acts::VolumeBounds>>(
        m, "VolumeBounds")
        .def("type", &Acts::VolumeBounds::type)
        .def("__str__", [](const Acts::VolumeBounds& self) {
          std::stringstream ss;
          ss << self;
          return ss.str();
        });

    auto cvb =
        py::class_<Acts::CylinderVolumeBounds,
                   std::shared_ptr<Acts::CylinderVolumeBounds>,
                   Acts::VolumeBounds>(m, "CylinderVolumeBounds")
            .def(py::init<double, double, double, double, double, double,
                          double>(),
                 "rmin"_a, "rmax"_a, "halfz"_a, "halfphi"_a = std::numbers::pi,
                 "avgphi"_a = 0., "bevelMinZ"_a = 0., "bevelMaxZ"_a = 0.);

    py::enum_<CylinderVolumeBounds::Face>(cvb, "Face")
        .value("PositiveDisc", CylinderVolumeBounds::Face::PositiveDisc)
        .value("NegativeDisc", CylinderVolumeBounds::Face::NegativeDisc)
        .value("OuterCylinder", CylinderVolumeBounds::Face::OuterCylinder)
        .value("InnerCylinder", CylinderVolumeBounds::Face::InnerCylinder)
        .value("NegativePhiPlane", CylinderVolumeBounds::Face::NegativePhiPlane)
        .value("PositivePhiPlane",
               CylinderVolumeBounds::Face::PositivePhiPlane);
  }

  {
    py::class_<Acts::Volume, std::shared_ptr<Acts::Volume>>(m, "Volume");

    py::class_<Acts::TrackingVolume, Acts::Volume,
               std::shared_ptr<Acts::TrackingVolume>>(m, "TrackingVolume")
        .def(py::init<const Transform3&, std::shared_ptr<Acts::VolumeBounds>,
                      std::string>());
  }

  {
    py::class_<Acts::GeometryIdentifierHook,
               std::shared_ptr<Acts::GeometryIdentifierHook>>(
        m, "GeometryIdentifierHook")
        .def(py::init([](py::object callable) {
          auto hook = std::make_shared<GeometryIdentifierHookBinding>();
          hook->callable = callable;
          return hook;
        }));
  }

  {
    py::class_<TrackingGeometryMutableVisitor, PyTrackingGeometryVisitor,
               std::shared_ptr<TrackingGeometryMutableVisitor>>(
        m, "TrackingGeometryMutableVisitor")
        .def(py::init<>());
  }

  py::class_<ExtentEnvelope>(m, "ExtentEnvelope")
      .def(py::init<>())
      .def(py::init<const Envelope&>())
      .def(py::init([](Envelope x, Envelope y, Envelope z, Envelope r,
                       Envelope phi, Envelope rPhi, Envelope theta,
                       Envelope eta, Envelope mag) {
             return ExtentEnvelope({.x = x,
                                    .y = y,
                                    .z = z,
                                    .r = r,
                                    .phi = phi,
                                    .rPhi = rPhi,
                                    .theta = theta,
                                    .eta = eta,
                                    .mag = mag});
           }),
           py::arg("x") = zeroEnvelope, py::arg("y") = zeroEnvelope,
           py::arg("z") = zeroEnvelope, py::arg("r") = zeroEnvelope,
           py::arg("phi") = zeroEnvelope, py::arg("rPhi") = zeroEnvelope,
           py::arg("theta") = zeroEnvelope, py::arg("eta") = zeroEnvelope,
           py::arg("mag") = zeroEnvelope)
      .def_static("Zero", &ExtentEnvelope::Zero)
      .def("__getitem__", [](ExtentEnvelope& self,
                             AxisDirection bValue) { return self[bValue]; })
      .def("__setitem__", [](ExtentEnvelope& self, AxisDirection bValue,
                             const Envelope& value) { self[bValue] = value; })
      .def("__str__", [](const ExtentEnvelope& self) {
        std::array<std::string, numAxisDirections()> values;

        std::stringstream ss;
        for (AxisDirection val : allAxisDirections()) {
          ss << val << "=(" << self[val][0] << ", " << self[val][1] << ")";
          values.at(toUnderlying(val)) = ss.str();
          ss.str("");
        }

        ss.str("");
        ss << "ExtentEnvelope(";
        ss << boost::algorithm::join(values, ", ");
        ss << ")";
        return ss.str();
      });

  py::class_<Extent>(m, "Extent")
      .def(py::init<const ExtentEnvelope&>(),
           py::arg("envelope") = ExtentEnvelope::Zero())
      .def("range",
           [](const Acts::Extent& self,
              Acts::AxisDirection bval) -> std::array<double, 2> {
             return {self.min(bval), self.max(bval)};
           })
      .def("setRange",
           [](Extent& self, Acts::AxisDirection bval,
              const std::array<double, 2>& range) {
             self.set(bval, range[0], range[1]);
           })
      .def("__str__", &Extent::toString);

  {
    py::enum_<VolumeAttachmentStrategy>(m, "VolumeAttachmentStrategy")
        .value("Gap", VolumeAttachmentStrategy::Gap)
        .value("First", VolumeAttachmentStrategy::First)
        .value("Second", VolumeAttachmentStrategy::Second)
        .value("Midpoint", VolumeAttachmentStrategy::Midpoint);

    py::enum_<VolumeResizeStrategy>(m, "VolumeResizeStrategy")
        .value("Gap", VolumeResizeStrategy::Gap)
        .value("Expand", VolumeResizeStrategy::Expand);
  }

  addBlueprint(ctx);
}

void addExperimentalGeometry(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  using namespace Acts::Experimental;

  // Detector volume definition
  py::class_<DetectorVolume, std::shared_ptr<DetectorVolume>>(m,
                                                              "DetectorVolume")
      .def("surfaces", &DetectorVolume::surfaces)
      .def("surfacePtrs", &DetectorVolume::surfacePtrs);

  // Detector definition
  py::class_<Detector, std::shared_ptr<Detector>>(m, "Detector")
      .def("volumes", &Detector::volumes)
      .def("volumePtrs", &Detector::volumePtrs)
      .def("numberVolumes",
           [](const Detector& self) { return self.volumes().size(); })
      .def("extractMaterialSurfaces",
           [](const Detector& self) {
             MaterialSurfaceSelector selector;
             self.visitSurfaces(selector);
             return selector.surfaces;
           })
      .def("geoIdSurfaceMap",
           [](const Detector& self) {
             IdentifierSurfacesCollector collector;
             self.visitSurfaces(collector);
             return collector.surfaces;
           })
      .def("cylindricalVolumeRepresentation",
           [](const Detector& self, const Acts::GeometryContext& gctx) {
             // Loop over the volumes and gather the extent
             Extent extent;
             for (const auto& volume : self.volumes()) {
               extent.extend(volume->extent(gctx));
             }
             auto bounds = std::make_shared<Acts::CylinderVolumeBounds>(
                 0., extent.max(Acts::AxisDirection::AxisR),
                 extent.max(Acts::AxisDirection::AxisZ));

             return std::make_shared<Acts::Volume>(Transform3::Identity(),
                                                   std::move(bounds));
           });

  // Portal definition
  py::class_<Experimental::Portal, std::shared_ptr<Experimental::Portal>>(
      m, "Portal");

  {
    // The surface hierarchy map
    using SurfaceHierarchyMap =
        Acts::GeometryHierarchyMap<std::shared_ptr<Surface>>;

    py::class_<SurfaceHierarchyMap, std::shared_ptr<SurfaceHierarchyMap>>(
        m, "SurfaceHierarchyMap");

    // Extract volume / layer surfaces
    mex.def("extractVolumeLayerSurfaces", [](const SurfaceHierarchyMap& smap,
                                             bool sensitiveOnly) {
      std::map<unsigned int,
               std::map<unsigned int, std::vector<std::shared_ptr<Surface>>>>
          surfaceVolumeLayerMap;
      for (const auto& surface : smap) {
        auto gid = surface->geometryId();
        // Exclusion criteria
        if (sensitiveOnly && gid.sensitive() == 0) {
          continue;
        };
        surfaceVolumeLayerMap[gid.volume()][gid.layer()].push_back(surface);
      }
      // Return the surface volume map
      return surfaceVolumeLayerMap;
    });
  }

  {
    // Be able to construct a proto binning
    py::class_<ProtoBinning>(m, "ProtoBinning")
        .def(py::init<Acts::AxisDirection, Acts::AxisBoundaryType,
                      const std::vector<double>&, std::size_t>(),
             "bValue"_a, "bType"_a, "e"_a, "exp"_a = 0u)
        .def(py::init<Acts::AxisDirection, Acts::AxisBoundaryType, double,
                      double, std::size_t, std::size_t>(),
             "bValue"_a, "bType"_a, "minE"_a, "maxE"_a, "nbins"_a, "exp"_a = 0u)
        .def(py::init<Acts::AxisDirection, Acts::AxisBoundaryType, std::size_t,
                      std::size_t>(),
             "bValue"_a, "bType"_a, "nbins"_a, "exp"_a = 0u);
  }

  {
    // The internal layer structure builder
    py::class_<Acts::Experimental::IInternalStructureBuilder,
               std::shared_ptr<Acts::Experimental::IInternalStructureBuilder>>(
        m, "IInternalStructureBuilder");

    auto lsBuilder =
        py::class_<LayerStructureBuilder,
                   Acts::Experimental::IInternalStructureBuilder,
                   std::shared_ptr<LayerStructureBuilder>>(
            m, "LayerStructureBuilder")
            .def(py::init([](const LayerStructureBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<LayerStructureBuilder>(
                  config, getDefaultLogger(name, level));
            }));

    auto lsConfig =
        py::class_<LayerStructureBuilder::Config>(lsBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(lsConfig, LayerStructureBuilder::Config);
    ACTS_PYTHON_MEMBER(surfacesProvider);
    ACTS_PYTHON_MEMBER(supports);
    ACTS_PYTHON_MEMBER(binnings);
    ACTS_PYTHON_MEMBER(quarterSegments);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();

    // The internal layer structure builder
    py::class_<Acts::Experimental::ISurfacesProvider,
               std::shared_ptr<Acts::Experimental::ISurfacesProvider>>(
        m, "ISurfacesProvider");

    py::class_<LayerStructureBuilder::SurfacesHolder,
               Acts::Experimental::ISurfacesProvider,
               std::shared_ptr<LayerStructureBuilder::SurfacesHolder>>(
        lsBuilder, "SurfacesHolder")
        .def(py::init<std::vector<std::shared_ptr<Surface>>>());
  }

  {
    using RangeXDDim1 = Acts::RangeXD<1u, double>;
    using KdtSurfacesDim1Bin100 = Acts::Experimental::KdtSurfaces<1u, 100u>;
    using KdtSurfacesProviderDim1Bin100 =
        Acts::Experimental::KdtSurfacesProvider<1u, 100u>;

    py::class_<RangeXDDim1>(m, "RangeXDDim1")
        .def(py::init([](const std::array<double, 2u>& irange) {
          RangeXDDim1 range;
          range[0].shrink(irange[0], irange[1]);
          return range;
        }));

    py::class_<KdtSurfacesDim1Bin100, std::shared_ptr<KdtSurfacesDim1Bin100>>(
        m, "KdtSurfacesDim1Bin100")
        .def(py::init<const GeometryContext&,
                      const std::vector<std::shared_ptr<Acts::Surface>>&,
                      const std::array<Acts::AxisDirection, 1u>&>())
        .def("surfaces", py::overload_cast<const RangeXDDim1&>(
                             &KdtSurfacesDim1Bin100::surfaces, py::const_));

    py::class_<KdtSurfacesProviderDim1Bin100,
               Acts::Experimental::ISurfacesProvider,
               std::shared_ptr<KdtSurfacesProviderDim1Bin100>>(
        m, "KdtSurfacesProviderDim1Bin100")
        .def(py::init<std::shared_ptr<KdtSurfacesDim1Bin100>, const Extent&>());
  }

  {
    using RangeXDDim2 = Acts::RangeXD<2u, double>;
    using KdtSurfacesDim2Bin100 = Acts::Experimental::KdtSurfaces<2u, 100u>;
    using KdtSurfacesProviderDim2Bin100 =
        Acts::Experimental::KdtSurfacesProvider<2u, 100u>;

    py::class_<RangeXDDim2>(m, "RangeXDDim2")
        .def(py::init([](const std::array<double, 2u>& range0,
                         const std::array<double, 2u>& range1) {
          RangeXDDim2 range;
          range[0].shrink(range0[0], range0[1]);
          range[1].shrink(range1[0], range1[1]);
          return range;
        }));

    py::class_<KdtSurfacesDim2Bin100, std::shared_ptr<KdtSurfacesDim2Bin100>>(
        m, "KdtSurfacesDim2Bin100")
        .def(py::init<const GeometryContext&,
                      const std::vector<std::shared_ptr<Acts::Surface>>&,
                      const std::array<Acts::AxisDirection, 2u>&>())
        .def("surfaces", py::overload_cast<const RangeXDDim2&>(
                             &KdtSurfacesDim2Bin100::surfaces, py::const_));

    py::class_<KdtSurfacesProviderDim2Bin100,
               Acts::Experimental::ISurfacesProvider,
               std::shared_ptr<KdtSurfacesProviderDim2Bin100>>(
        m, "KdtSurfacesProviderDim2Bin100")
        .def(py::init<std::shared_ptr<KdtSurfacesDim2Bin100>, const Extent&>());
  }

  {
    using RangeXDDim3 = Acts::RangeXD<3u, double>;

    py::class_<RangeXDDim3>(m, "RangeXDDim3")
        .def(py::init([](const std::array<double, 2u>& range0,
                         const std::array<double, 2u>& range1,
                         const std::array<double, 2u>& range2) {
          RangeXDDim3 range;
          range[0].shrink(range0[0], range0[1]);
          range[1].shrink(range1[0], range1[1]);
          range[2].shrink(range2[0], range2[1]);
          return range;
        }));
  }

  {
    // The external volume structure builder
    py::class_<Acts::Experimental::IExternalStructureBuilder,
               std::shared_ptr<Acts::Experimental::IExternalStructureBuilder>>(
        m, "IExternalStructureBuilder");

    auto vsBuilder =
        py::class_<VolumeStructureBuilder,
                   Acts::Experimental::IExternalStructureBuilder,
                   std::shared_ptr<VolumeStructureBuilder>>(
            m, "VolumeStructureBuilder")
            .def(py::init([](const VolumeStructureBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<VolumeStructureBuilder>(
                  config, getDefaultLogger(name, level));
            }));

    auto vsConfig =
        py::class_<VolumeStructureBuilder::Config>(vsBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(vsConfig, VolumeStructureBuilder::Config);
    ACTS_PYTHON_MEMBER(boundsType);
    ACTS_PYTHON_MEMBER(boundValues);
    ACTS_PYTHON_MEMBER(transform);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::Experimental::IGeometryIdGenerator,
               std::shared_ptr<Acts::Experimental::IGeometryIdGenerator>>(
        m, "IGeometryIdGenerator");

    auto geoIdGen =
        py::class_<Acts::Experimental::GeometryIdGenerator,
                   Acts::Experimental::IGeometryIdGenerator,
                   std::shared_ptr<Acts::Experimental::GeometryIdGenerator>>(
            m, "GeometryIdGenerator")
            .def(py::init([](Acts::Experimental::GeometryIdGenerator::Config&
                                 config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<Acts::Experimental::GeometryIdGenerator>(
                  config, getDefaultLogger(name, level));
            }));

    auto geoIdGenConfig =
        py::class_<Acts::Experimental::GeometryIdGenerator::Config>(geoIdGen,
                                                                    "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(geoIdGenConfig,
                             Acts::Experimental::GeometryIdGenerator::Config);
    ACTS_PYTHON_MEMBER(containerMode);
    ACTS_PYTHON_MEMBER(containerId);
    ACTS_PYTHON_MEMBER(resetSubCounters);
    ACTS_PYTHON_MEMBER(overrideExistingIds);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    // Put them together to a detector volume
    py::class_<Acts::Experimental::IDetectorComponentBuilder,
               std::shared_ptr<Acts::Experimental::IDetectorComponentBuilder>>(
        m, "IDetectorComponentBuilder");

    auto dvBuilder =
        py::class_<DetectorVolumeBuilder,
                   Acts::Experimental::IDetectorComponentBuilder,
                   std::shared_ptr<DetectorVolumeBuilder>>(
            m, "DetectorVolumeBuilder")
            .def(py::init([](const DetectorVolumeBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<DetectorVolumeBuilder>(
                  config, getDefaultLogger(name, level));
            }))
            .def("construct", &DetectorVolumeBuilder::construct);

    auto dvConfig =
        py::class_<DetectorVolumeBuilder::Config>(dvBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(dvConfig, DetectorVolumeBuilder::Config);
    ACTS_PYTHON_MEMBER(name);
    ACTS_PYTHON_MEMBER(internalsBuilder);
    ACTS_PYTHON_MEMBER(externalsBuilder);
    ACTS_PYTHON_MEMBER(geoIdGenerator);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    // The external volume structure builder
    py::class_<Acts::Experimental::IRootVolumeFinderBuilder,
               std::shared_ptr<Acts::Experimental::IRootVolumeFinderBuilder>>(
        m, "IRootVolumeFinderBuilder");

    auto irvBuilder =
        py::class_<Acts::Experimental::IndexedRootVolumeFinderBuilder,
                   Acts::Experimental::IRootVolumeFinderBuilder,
                   std::shared_ptr<
                       Acts::Experimental::IndexedRootVolumeFinderBuilder>>(
            m, "IndexedRootVolumeFinderBuilder")
            .def(py::init<std::vector<Acts::AxisDirection>>());
  }

  {
    // Cylindrical container builder
    auto ccBuilder =
        py::class_<CylindricalContainerBuilder,
                   Acts::Experimental::IDetectorComponentBuilder,
                   std::shared_ptr<CylindricalContainerBuilder>>(
            m, "CylindricalContainerBuilder")
            .def(py::init([](const CylindricalContainerBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<CylindricalContainerBuilder>(
                  config, getDefaultLogger(name, level));
            }))
            .def("construct", &CylindricalContainerBuilder::construct);

    auto ccConfig =
        py::class_<CylindricalContainerBuilder::Config>(ccBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(ccConfig, CylindricalContainerBuilder::Config);
    ACTS_PYTHON_MEMBER(builders);
    ACTS_PYTHON_MEMBER(binning);
    ACTS_PYTHON_MEMBER(rootVolumeFinderBuilder);
    ACTS_PYTHON_MEMBER(geoIdGenerator);
    ACTS_PYTHON_MEMBER(geoIdReverseGen);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    // Cuboidal container builder
    auto ccBuilder =
        py::class_<CuboidalContainerBuilder,
                   Acts::Experimental::IDetectorComponentBuilder,
                   std::shared_ptr<CuboidalContainerBuilder>>(
            m, "CuboidalContainerBuilder")
            .def(py::init([](const CuboidalContainerBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<CuboidalContainerBuilder>(
                  config, getDefaultLogger(name, level));
            }))
            .def("construct", &CuboidalContainerBuilder::construct);

    auto ccConfig =
        py::class_<CuboidalContainerBuilder::Config>(ccBuilder, "Config")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(ccConfig, CuboidalContainerBuilder::Config);
    ACTS_PYTHON_MEMBER(builders);
    ACTS_PYTHON_MEMBER(binning);
    ACTS_PYTHON_MEMBER(rootVolumeFinderBuilder);
    ACTS_PYTHON_MEMBER(geoIdGenerator);
    ACTS_PYTHON_MEMBER(geoIdReverseGen);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    // Detector builder
    auto dBuilder =
        py::class_<DetectorBuilder, std::shared_ptr<DetectorBuilder>>(
            m, "DetectorBuilder")
            .def(py::init([](const DetectorBuilder::Config& config,
                             const std::string& name,
                             Acts::Logging::Level level) {
              return std::make_shared<DetectorBuilder>(
                  config, getDefaultLogger(name, level));
            }))
            .def("construct", &DetectorBuilder::construct);

    auto dConfig = py::class_<DetectorBuilder::Config>(dBuilder, "Config")
                       .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(dConfig, DetectorBuilder::Config);
    ACTS_PYTHON_MEMBER(name);
    ACTS_PYTHON_MEMBER(builder);
    ACTS_PYTHON_MEMBER(geoIdGenerator);
    ACTS_PYTHON_MEMBER(materialDecorator);
    ACTS_PYTHON_MEMBER(auxiliary);
    ACTS_PYTHON_STRUCT_END();
  }

  ACTS_PYTHON_DECLARE_ALGORITHM(ActsExamples::VolumeAssociationTest, mex,
                                "VolumeAssociationTest", name, ntests,
                                randomNumbers, randomRange, detector);

  py::class_<ProtoLayer>(m, "ProtoLayer")
      .def(py::init<const GeometryContext&,
                    const std::vector<std::shared_ptr<Surface>>&,
                    const Transform3&>(),
           "gctx"_a, "surfaces"_a, "transform"_a = Transform3::Identity())
      .def("min", &ProtoLayer::min, "bval"_a, "addenv"_a = true)
      .def("max", &ProtoLayer::max, "bval"_a, "addenv"_a = true)
      .def_property_readonly("surfaces", &ProtoLayer::surfaces);
}

}  // namespace Acts::Python
