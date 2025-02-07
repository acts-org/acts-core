// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Plugins/TGeo/TGeoDetectorElement.hpp"
#include "Acts/Plugins/TGeo/TGeoLayerBuilder.hpp"
#include "Acts/Plugins/TGeo/TGeoParser.hpp"

#include <vector>

#include <TGeoManager.h>
#include <TGeoVolume.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {
void addTGeo(Context& ctx) {
  auto [m, mex] = ctx.get("main", "examples");

  auto tgeo = mex.def_submodule("tgeo");

  {
    py::class_<Acts::TGeoDetectorElement,
               std::shared_ptr<Acts::TGeoDetectorElement>>(
        tgeo, "TGeoDetectorElement")
        .def("surface", [](const Acts::TGeoDetectorElement& self) {
          return self.surface().getSharedPtr();
        });
  }

  {
    /// Helper function to test if the automatic geometry conversion works
    ///
    /// @param rootFileName is the name of the GDML file
    /// @param sensitiveMatches is a list of strings to match sensitive volumes
    /// @param localAxes is the TGeo->ACTS axis conversion convention
    /// @param scaleConversion is a unit scalor conversion factor
    tgeo.def("_convertToElements",
             [](const std::string& rootFileName,
                const std::vector<std::string>& sensitiveMatches,
                const std::string& localAxes, double scaleConversion) {
               // Return vector
               std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>>
                   tgElements;
               // TGeo import
               TGeoManager::Import(rootFileName.c_str());
               if (gGeoManager != nullptr) {
                 auto tVolume = gGeoManager->GetTopVolume();
                 if (tVolume != nullptr) {
                   TGeoHMatrix gmatrix = TGeoIdentity(tVolume->GetName());

                   TGeoParser::Options tgpOptions;
                   tgpOptions.volumeNames = {tVolume->GetName()};
                   tgpOptions.targetNames = sensitiveMatches;
                   tgpOptions.unit = scaleConversion;
                   TGeoParser::State tgpState;
                   tgpState.volume = tVolume;
                   tgpState.onBranch = true;

                   TGeoParser::select(tgpState, tgpOptions, gmatrix);
                   tgElements.reserve(tgpState.selectedNodes.size());

                   for (const auto& snode : tgpState.selectedNodes) {
                     auto identifier = Acts::TGeoDetectorElement::Identifier();
                     auto tgElement = TGeoLayerBuilder::defaultElementFactory(
                         identifier, *snode.node, *snode.transform, localAxes,
                         scaleConversion, nullptr);
                     tgElements.emplace_back(tgElement);
                   }
                 }
               }
               // Return the elements
               return tgElements;
             });
  }
}

}  // namespace Acts::Python
