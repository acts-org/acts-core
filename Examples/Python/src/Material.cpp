// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Material/BinnedSurfaceMaterialAccumulater.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/IMaterialDecorator.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/IntersectionMaterialAssigner.hpp"
#include "Acts/Material/MaterialMapper.hpp"
#include "Acts/Material/MaterialValidater.hpp"
#include "Acts/Material/PropagatorMaterialAssigner.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Material/SurfaceMaterialMapper.hpp"
#include "Acts/Material/VolumeMaterialMapper.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/Json/MaterialMapJsonConverter.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Io/Root/RootMaterialDecorator.hpp"
#include "ActsExamples/MaterialMapping/CoreMaterialMapping.hpp"
#include "ActsExamples/MaterialMapping/MappingMaterialDecorator.hpp"
#include "ActsExamples/MaterialMapping/MaterialMapping.hpp"
#include "ActsExamples/MaterialMapping/MaterialValidation.hpp"

#include <array>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace Acts {
class TrackingGeometry;
}  // namespace Acts
namespace ActsExamples {
class IAlgorithm;
}  // namespace ActsExamples

namespace py = pybind11;
using namespace pybind11::literals;

using namespace ActsExamples;

namespace Acts::Python {
void addMaterial(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  {
    py::class_<Acts::ISurfaceMaterial, std::shared_ptr<ISurfaceMaterial>>(
        m, "ISurfaceMaterial")
        .def("toString", &Acts::ISurfaceMaterial::toString);

    py::class_<Acts::ProtoGridSurfaceMaterial, Acts::ISurfaceMaterial,
               std::shared_ptr<ProtoGridSurfaceMaterial>>(
        m, "ProtoGridSurfaceMaterial");

    py::class_<Acts::ProtoSurfaceMaterial, Acts::ISurfaceMaterial,
               std::shared_ptr<ProtoSurfaceMaterial>>(m,
                                                      "ProtoSurfaceMaterial");

    py::class_<Acts::HomogeneousSurfaceMaterial, Acts::ISurfaceMaterial,
               std::shared_ptr<HomogeneousSurfaceMaterial>>(
        m, "HomogeneousSurfaceMaterial");

    py::class_<Acts::IVolumeMaterial, std::shared_ptr<IVolumeMaterial>>(
        m, "IVolumeMaterial");
  }

  {
    py::class_<Acts::IMaterialDecorator,
               std::shared_ptr<Acts::IMaterialDecorator>>(m,
                                                          "IMaterialDecorator")
        .def("decorate", py::overload_cast<Surface&>(
                             &Acts::IMaterialDecorator::decorate, py::const_));
  }

  {
    auto rmd =
        py::class_<RootMaterialDecorator, Acts::IMaterialDecorator,
                   std::shared_ptr<RootMaterialDecorator>>(
            mex, "RootMaterialDecorator")
            .def(
                py::init<RootMaterialDecorator::Config, Acts::Logging::Level>(),
                py::arg("config"), py::arg("level"));

    using Config = RootMaterialDecorator::Config;
    auto c = py::class_<Config>(rmd, "Config").def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(voltag);
    ACTS_PYTHON_MEMBER(boutag);
    ACTS_PYTHON_MEMBER(laytag);
    ACTS_PYTHON_MEMBER(apptag);
    ACTS_PYTHON_MEMBER(sentag);
    ACTS_PYTHON_MEMBER(ntag);
    ACTS_PYTHON_MEMBER(vtag);
    ACTS_PYTHON_MEMBER(otag);
    ACTS_PYTHON_MEMBER(mintag);
    ACTS_PYTHON_MEMBER(maxtag);
    ACTS_PYTHON_MEMBER(ttag);
    ACTS_PYTHON_MEMBER(x0tag);
    ACTS_PYTHON_MEMBER(l0tag);
    ACTS_PYTHON_MEMBER(atag);
    ACTS_PYTHON_MEMBER(ztag);
    ACTS_PYTHON_MEMBER(rhotag);
    ACTS_PYTHON_MEMBER(fileName);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<MappingMaterialDecorator, Acts::IMaterialDecorator,
               std::shared_ptr<MappingMaterialDecorator>>(
        m, "MappingMaterialDecorator")
        .def(py::init<const Acts::TrackingGeometry&, Acts::Logging::Level, bool,
                      bool>(),
             py::arg("tGeometry"), py::arg("level"),
             py::arg("clearSurfaceMaterial") = true,
             py::arg("clearVolumeMaterial") = true)
        .def("binningMap", &MappingMaterialDecorator::binningMap)
        .def("setBinningMap", &MappingMaterialDecorator::setBinningMap);
  }

  {
    using Alg = ActsExamples::MaterialMapping;

    auto alg = py::class_<Alg, ActsExamples::IAlgorithm, std::shared_ptr<Alg>>(
                   mex, "MaterialMapping")
                   .def(py::init<const Alg::Config&, Acts::Logging::Level>(),
                        py::arg("config"), py::arg("level"))
                   .def("scoringParameters", &Alg::scoringParameters)
                   .def_property_readonly("config", &Alg::config);

    auto c = py::class_<Alg::Config>(alg, "Config")
                 .def(py::init<const Acts::GeometryContext&,
                               const Acts::MagneticFieldContext&>());

    ACTS_PYTHON_STRUCT_BEGIN(c, Alg::Config);
    ACTS_PYTHON_MEMBER(inputMaterialTracks);
    ACTS_PYTHON_MEMBER(mappingMaterialCollection);
    ACTS_PYTHON_MEMBER(materialSurfaceMapper);
    ACTS_PYTHON_MEMBER(materialVolumeMapper);
    ACTS_PYTHON_MEMBER(materialWriters);
    ACTS_PYTHON_MEMBER(trackingGeometry);
    ACTS_PYTHON_MEMBER(geoContext);
    ACTS_PYTHON_MEMBER(magFieldContext);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto cls =
        py::class_<SurfaceMaterialMapper,
                   std::shared_ptr<SurfaceMaterialMapper>>(
            m, "SurfaceMaterialMapper")
            .def(py::init([](const SurfaceMaterialMapper::Config& config,
                             SurfaceMaterialMapper::StraightLinePropagator prop,
                             Acts::Logging::Level level) {
                   return std::make_shared<SurfaceMaterialMapper>(
                       config, std::move(prop),
                       getDefaultLogger("SurfaceMaterialMapper", level));
                 }),
                 py::arg("config"), py::arg("propagator"), py::arg("level"));

    auto c = py::class_<SurfaceMaterialMapper::Config>(cls, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, SurfaceMaterialMapper::Config);
    ACTS_PYTHON_MEMBER(etaRange);
    ACTS_PYTHON_MEMBER(emptyBinCorrection);
    ACTS_PYTHON_MEMBER(mapperDebugOutput);
    ACTS_PYTHON_MEMBER(computeVariance);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto cls =
        py::class_<VolumeMaterialMapper, std::shared_ptr<VolumeMaterialMapper>>(
            m, "VolumeMaterialMapper")
            .def(py::init([](const VolumeMaterialMapper::Config& config,
                             VolumeMaterialMapper::StraightLinePropagator prop,
                             Acts::Logging::Level level) {
                   return std::make_shared<VolumeMaterialMapper>(
                       config, std::move(prop),
                       getDefaultLogger("VolumeMaterialMapper", level));
                 }),
                 py::arg("config"), py::arg("propagator"), py::arg("level"));

    auto c = py::class_<VolumeMaterialMapper::Config>(cls, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, VolumeMaterialMapper::Config);
    ACTS_PYTHON_MEMBER(mappingStep);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::IAssignmentFinder,
               std::shared_ptr<Acts::IAssignmentFinder>>(m,
                                                         "IAssignmentFinder");
  }

  {
    auto isma =
        py::class_<Acts::IntersectionMaterialAssigner, Acts::IAssignmentFinder,
                   std::shared_ptr<Acts::IntersectionMaterialAssigner>>(
            m, "IntersectionMaterialAssigner")
            .def(py::init([](const Acts::IntersectionMaterialAssigner::Config&
                                 config,
                             Acts::Logging::Level level) {
                   return std::make_shared<Acts::IntersectionMaterialAssigner>(
                       config,
                       getDefaultLogger("IntersectionMaterialAssigner", level));
                 }),
                 py::arg("config"), py::arg("level"))
            .def("assignmentCandidates",
                 &Acts::IntersectionMaterialAssigner::assignmentCandidates);

    auto c =
        py::class_<Acts::IntersectionMaterialAssigner::Config>(isma, "Config")
            .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Acts::IntersectionMaterialAssigner::Config);
    ACTS_PYTHON_MEMBER(surfaces);
    ACTS_PYTHON_MEMBER(trackingVolumes);
    ACTS_PYTHON_MEMBER(detectorVolumes);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    py::class_<Acts::ISurfaceMaterialAccumulater,
               std::shared_ptr<Acts::ISurfaceMaterialAccumulater>>(
        m, "ISurfaceMaterialAccumulater");
  }

  {
    auto bsma =
        py::class_<BinnedSurfaceMaterialAccumulater,
                   ISurfaceMaterialAccumulater,
                   std::shared_ptr<BinnedSurfaceMaterialAccumulater>>(
            m, "BinnedSurfaceMaterialAccumulater")
            .def(
                py::init(
                    [](const BinnedSurfaceMaterialAccumulater::Config& config,
                       Acts::Logging::Level level) {
                      return std::make_shared<BinnedSurfaceMaterialAccumulater>(
                          config,
                          getDefaultLogger("BinnedSurfaceMaterialAccumulater",
                                           level));
                    }),
                py::arg("config"), py::arg("level"))
            .def("createState", &BinnedSurfaceMaterialAccumulater::createState)
            .def("accumulate", &BinnedSurfaceMaterialAccumulater::accumulate)
            .def("finalizeMaterial",
                 &BinnedSurfaceMaterialAccumulater::finalizeMaterial);

    auto c =
        py::class_<BinnedSurfaceMaterialAccumulater::Config>(bsma, "Config")
            .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, BinnedSurfaceMaterialAccumulater::Config);
    ACTS_PYTHON_MEMBER(emptyBinCorrection);
    ACTS_PYTHON_MEMBER(materialSurfaces);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto mm = py::class_<MaterialMapper, std::shared_ptr<MaterialMapper>>(
                  m, "MaterialMapper")
                  .def(py::init([](const MaterialMapper::Config& config,
                                   Acts::Logging::Level level) {
                         return std::make_shared<MaterialMapper>(
                             config, getDefaultLogger("MaterialMapper", level));
                       }),
                       py::arg("config"), py::arg("level"));

    auto c = py::class_<MaterialMapper::Config>(mm, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, MaterialMapper::Config);
    ACTS_PYTHON_MEMBER(assignmentFinder);
    ACTS_PYTHON_MEMBER(surfaceMaterialAccumulater);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto mmca = py::class_<CoreMaterialMapping, IAlgorithm,
                           std::shared_ptr<CoreMaterialMapping>>(
                    mex, "CoreMaterialMapping")
                    .def(py::init<const CoreMaterialMapping::Config&,
                                  Acts::Logging::Level>(),
                         py::arg("config"), py::arg("level"));

    auto c = py::class_<CoreMaterialMapping::Config>(mmca, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, CoreMaterialMapping::Config);
    ACTS_PYTHON_MEMBER(inputMaterialTracks);
    ACTS_PYTHON_MEMBER(mappedMaterialTracks);
    ACTS_PYTHON_MEMBER(unmappedMaterialTracks);
    ACTS_PYTHON_MEMBER(materialMapper);
    ACTS_PYTHON_MEMBER(materiaMaplWriters);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto mvc =
        py::class_<MaterialValidater, std::shared_ptr<MaterialValidater>>(
            m, "MaterialValidater")
            .def(py::init([](const MaterialValidater::Config& config,
                             Acts::Logging::Level level) {
                   return std::make_shared<MaterialValidater>(
                       config, getDefaultLogger("MaterialValidater", level));
                 }),
                 py::arg("config"), py::arg("level"))
            .def("recordMaterial", &MaterialValidater::recordMaterial);

    auto c =
        py::class_<MaterialValidater::Config>(mvc, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, MaterialValidater::Config);
    ACTS_PYTHON_MEMBER(materialAssigner);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto mv = py::class_<MaterialValidation, IAlgorithm,
                         std::shared_ptr<MaterialValidation>>(
                  mex, "MaterialValidation")
                  .def(py::init<const MaterialValidation::Config&,
                                Acts::Logging::Level>(),
                       py::arg("config"), py::arg("level"))
                  .def("execute", &MaterialValidation::execute)
                  .def_property_readonly("config", &MaterialValidation::config);

    auto c =
        py::class_<MaterialValidation::Config>(mv, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, MaterialValidation::Config);
    ACTS_PYTHON_MEMBER(ntracks);
    ACTS_PYTHON_MEMBER(startPosition);
    ACTS_PYTHON_MEMBER(phiRange);
    ACTS_PYTHON_MEMBER(etaRange);
    ACTS_PYTHON_MEMBER(randomNumberSvc);
    ACTS_PYTHON_MEMBER(materialValidater);
    ACTS_PYTHON_MEMBER(outputMaterialTracks);
    ACTS_PYTHON_STRUCT_END();
  }
}

}  // namespace Acts::Python
