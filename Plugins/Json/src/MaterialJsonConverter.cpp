// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/MaterialJsonConverter.hpp"

#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousVolumeMaterial.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/InterpolatedMaterialMap.hpp"
#include "Acts/Material/MaterialGridHelper.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Material/ProtoVolumeMaterial.hpp"
#include "Acts/Plugins/Json/GeometryJsonKeys.hpp"
#include "Acts/Plugins/Json/UtilitiesJsonConverter.hpp"
#include "Acts/Utilities/BinUtility.hpp"

void Acts::to_json(nlohmann::json& j, const Material& t) {
  if (!t) {
    return;
  }
  for (unsigned i = 0; i < t.parameters().size(); ++i) {
    j.push_back(t.parameters()[i]);
  }
}

void Acts::from_json(const nlohmann::json& j, Material& t) {
  if (j.is_null()) {
    return;
  }
  Acts::Material::ParametersVector params =
      Acts::Material::ParametersVector::Zero();
  for (auto i = params.size(); 0 < i--;) {
    // .at(...) ensures bound checks
    params[i] = j.at(i);
  }
  t = Acts::Material(params);
  return;
}

void Acts::to_json(nlohmann::json& j, const MaterialSlab& t) {
  nlohmann::json jmat(t.material());
  j["material"] = jmat;
  j["thickness"] = t.thickness();
}

void Acts::from_json(const nlohmann::json& j, MaterialSlab& t) {
  Material mat(j["material"].get<Material>());
  t = Acts::MaterialSlab(mat, j.at("thickness").get<float>());
}

void Acts::from_json(const nlohmann::json& j, MaterialSlabMatrix& t) {
  // the input data must be array[array[object]]
  for (auto& outer : j) {
    Acts::MaterialSlabVector mpVector;
    for (auto& inner : outer) {
      MaterialSlab mat = inner.get<MaterialSlab>();
      mpVector.emplace_back(mat);
    }
    t.push_back(std::move(mpVector));
  }
}

void Acts::to_json(nlohmann::json& j, const surfaceMaterialPointer& material) {
  nlohmann::json jMaterial;
  // A bin utility needs to be written
  const Acts::BinUtility* bUtility = nullptr;
  // Check if we have a proto material
  auto psMaterial = dynamic_cast<const Acts::ProtoSurfaceMaterial*>(material);
  if (psMaterial != nullptr) {
    // Type is proto material
    jMaterial[Acts::jsonKey().typekey] = "proto";
    // Set mapping type
    nlohmann::json mapType(material->mappingType());
    jMaterial[Acts::jsonKey().maptype] = mapType;
    // by default the protoMaterial is not used for mapping
    jMaterial[Acts::jsonKey().mapkey] = false;
    // write the bin utility
    bUtility = &(psMaterial->binUtility());
    // Check in the number of bin is different from 1
    auto& binningData = bUtility->binningData();
    for (size_t ibin = 0; ibin < binningData.size(); ++ibin) {
      if (binningData[ibin].bins() > 1) {
        jMaterial[Acts::jsonKey().mapkey] = true;
        break;
      }
    }
    nlohmann::json jBin(*bUtility);
    jMaterial[Acts::jsonKey().binkey] = jBin;
    j[Acts::jsonKey().materialkey] = jMaterial;
    return;
  }
  // Now check if we have a homogeneous material
  auto hsMaterial =
      dynamic_cast<const Acts::HomogeneousSurfaceMaterial*>(material);
  if (hsMaterial != nullptr) {
    // type is homogeneous
    jMaterial[Acts::jsonKey().typekey] = "homogeneous";
    // Set mapping type
    nlohmann::json mapType(material->mappingType());
    jMaterial[Acts::jsonKey().maptype] = mapType;
    // Material has been mapped
    jMaterial[Acts::jsonKey().mapkey] = true;
    nlohmann::json jmat(hsMaterial->materialSlab(0, 0));
    jMaterial[Acts::jsonKey().datakey] = nlohmann::json::array({
        nlohmann::json::array({
            jmat,
        }),
    });
    j[Acts::jsonKey().materialkey] = jMaterial;
    return;
  }
  // Only option remaining: BinnedSurface material
  auto bsMaterial = dynamic_cast<const Acts::BinnedSurfaceMaterial*>(material);
  if (bsMaterial != nullptr) {
    // type is binned
    jMaterial[Acts::jsonKey().typekey] = "binned";
    // Set mapping type
    nlohmann::json mapType(material->mappingType());
    jMaterial[Acts::jsonKey().maptype] = mapType;
    // Material has been mapped
    jMaterial[Acts::jsonKey().mapkey] = true;
    bUtility = &(bsMaterial->binUtility());
    // convert the data
    // get the material matrix
    nlohmann::json mmat = nlohmann::json::array();
    for (const auto& mpVector : bsMaterial->fullMaterial()) {
      nlohmann::json mvec = nlohmann::json::array();
      for (const auto& mp : mpVector) {
        nlohmann::json jmat(mp);
        mvec.push_back(jmat);
      }
      mmat.push_back(std::move(mvec));
    }
    jMaterial[Acts::jsonKey().datakey] = std::move(mmat);
    // write the bin utility
    nlohmann::json jBin(*bUtility);
    jMaterial[Acts::jsonKey().binkey] = jBin;
    j[Acts::jsonKey().materialkey] = jMaterial;
    return;
  }
  // No material the json object is left empty.
  return;
}

void Acts::from_json(const nlohmann::json& j,
                     surfaceMaterialPointer& material) {
  if (j.find(Acts::jsonKey().materialkey) == j.end()) {
    return;
  }
  nlohmann::json jMaterial = j[Acts::jsonKey().materialkey];
  // By default no material is return.
  material = nullptr;
  if (jMaterial[Acts::jsonKey().mapkey] == false) {
    return;
  }

  // The bin utility and material
  Acts::BinUtility bUtility;
  Acts::MaterialSlabMatrix mpMatrix;
  Acts::MappingType mapType = Acts::MappingType::Default;
  for (auto& [key, value] : jMaterial.items()) {
    if (key == Acts::jsonKey().binkey and not value.empty()) {
      from_json(value, bUtility);
    }
    if (key == Acts::jsonKey().datakey and not value.empty()) {
      from_json(value, mpMatrix);
    }
    if (key == Acts::jsonKey().maptype and not value.empty()) {
      from_json(value, mapType);
    }
  }
  // Return the appropriate typr of material
  if (mpMatrix.empty()) {
    material = new Acts::ProtoSurfaceMaterial(bUtility, mapType);
  } else if (bUtility.bins() == 1) {
    material = new Acts::HomogeneousSurfaceMaterial(mpMatrix[0][0], mapType);
  } else {
    material = new Acts::BinnedSurfaceMaterial(bUtility, mpMatrix, mapType);
  }
}

void Acts::to_json(nlohmann::json& j, const volumeMaterialPointer& material) {
  nlohmann::json jMaterial;
  // A bin utility needs to be written
  const Acts::BinUtility* bUtility = nullptr;
  // Check if we have a proto material
  auto pvMaterial = dynamic_cast<const Acts::ProtoVolumeMaterial*>(material);
  if (pvMaterial != nullptr) {
    // Type is proto material
    jMaterial[Acts::jsonKey().typekey] = "proto";
    // By default the protoMaterial is not used for mapping
    jMaterial[Acts::jsonKey().mapkey] = false;
    bUtility = &(pvMaterial->binUtility());
    // Check in the number of bin is different from 1
    auto& binningData = bUtility->binningData();
    for (size_t ibin = 0; ibin < binningData.size(); ++ibin) {
      if (binningData[ibin].bins() > 1) {
        jMaterial[Acts::jsonKey().mapkey] = true;
        break;
      }
    }
    // Write the bin utility
    nlohmann::json jBin(*bUtility);
    jMaterial[Acts::jsonKey().binkey] = jBin;
    j[Acts::jsonKey().materialkey] = jMaterial;
    return;
  }
  // Now check if we have a homogeneous material
  auto hvMaterial =
      dynamic_cast<const Acts::HomogeneousVolumeMaterial*>(material);
  if (hvMaterial != nullptr) {
    // type is homogeneous
    jMaterial[Acts::jsonKey().typekey] = "homogeneous";
    jMaterial[Acts::jsonKey().mapkey] = true;
    // array of encoded materials w/ one entry
    nlohmann::json jmat(hvMaterial->material({0, 0, 0}));
    jMaterial[Acts::jsonKey().datakey] = nlohmann::json::array({
        jmat,
    });
    j[Acts::jsonKey().materialkey] = jMaterial;
    return;
  }
  // Only option remaining: material map
  auto bvMaterial2D = dynamic_cast<const Acts::InterpolatedMaterialMap<
      Acts::MaterialMapper<Acts::MaterialGrid2D>>*>(material);
  // Now check if we have a 2D map
  if (bvMaterial2D != nullptr) {
    // type is binned
    jMaterial[Acts::jsonKey().typekey] = "interpolated2D";
    jMaterial[Acts::jsonKey().mapkey] = true;
    bUtility = &(bvMaterial2D->binUtility());
    // convert the data
    nlohmann::json mmat = nlohmann::json::array();
    Acts::MaterialGrid2D grid = bvMaterial2D->getMapper().getGrid();
    for (size_t bin = 0; bin < grid.size(); bin++) {
      nlohmann::json jmat(Material(grid.at(bin)));
      mmat.push_back(jmat);
    }
    jMaterial[Acts::jsonKey().datakey] = std::move(mmat);
    // Write the bin utility
    nlohmann::json jBin(*bUtility);
    jMaterial[Acts::jsonKey().binkey] = jBin;
    j[Acts::jsonKey().materialkey] = jMaterial;
    return;
  }
  // Only option remaining: material map
  auto bvMaterial3D = dynamic_cast<const Acts::InterpolatedMaterialMap<
      Acts::MaterialMapper<Acts::MaterialGrid3D>>*>(material);
  // Now check if we have a 3D map
  if (bvMaterial3D != nullptr) {
    // type is binned
    jMaterial[Acts::jsonKey().typekey] = "interpolated3D";
    jMaterial[Acts::jsonKey().mapkey] = true;
    bUtility = &(bvMaterial3D->binUtility());
    // convert the data
    nlohmann::json mmat = nlohmann::json::array();
    Acts::MaterialGrid3D grid = bvMaterial3D->getMapper().getGrid();
    for (size_t bin = 0; bin < grid.size(); bin++) {
      nlohmann::json jmat(Material(grid.at(bin)));
      mmat.push_back(jmat);
    }
    jMaterial[Acts::jsonKey().datakey] = std::move(mmat);
    // Write the bin utility
    nlohmann::json jBin(*bUtility);
    jMaterial[Acts::jsonKey().binkey] = jBin;
    j[Acts::jsonKey().materialkey] = jMaterial;
    return;
  }
}

void Acts::from_json(const nlohmann::json& j, volumeMaterialPointer& material) {
  if (j.find(Acts::jsonKey().materialkey) == j.end()) {
    return;
  }
  nlohmann::json jMaterial = j[Acts::jsonKey().materialkey];
  // By default no material is return.
  material = nullptr;
  if (jMaterial[Acts::jsonKey().mapkey] == false) {
    return;
  }
  // The bin utility and material
  Acts::BinUtility bUtility;
  std::vector<Acts::Material> mmat;
  for (auto& [key, value] : jMaterial.items()) {
    if (key == Acts::jsonKey().binkey and not value.empty()) {
      from_json(value, bUtility);
    }
    if (key == Acts::jsonKey().datakey and not value.empty()) {
      for (const auto& bin : value) {
        Acts::Material mat(bin.get<Acts::Material>());
        mmat.push_back(mat);
      }
    }
  }
  // We have protoMaterial
  if (mmat.empty()) {
    material = new Acts::ProtoVolumeMaterial(bUtility);
    return;
  }
  if (mmat.size() == 1) {
    material = new Acts::HomogeneousVolumeMaterial(mmat[0]);
    return;
  }
  if (bUtility.dimensions() == 2) {
    std::function<Acts::Vector2(Acts::Vector3)> transfoGlobalToLocal;
    Acts::Grid2D grid = createGrid2D(bUtility, transfoGlobalToLocal);

    Acts::Grid2D::point_t min = grid.minPosition();
    Acts::Grid2D::point_t max = grid.maxPosition();
    Acts::Grid2D::index_t nBins = grid.numLocalBins();

    Acts::EAxis axis1(min[0], max[0], nBins[0]);
    Acts::EAxis axis2(min[1], max[1], nBins[1]);

    // Build the grid and fill it with data
    Acts::MaterialGrid2D mGrid(std::make_tuple(axis1, axis2));

    for (size_t bin = 0; bin < mmat.size(); bin++) {
      mGrid.at(bin) = mmat[bin].parameters();
    }
    Acts::MaterialMapper<Acts::MaterialGrid2D> matMap(transfoGlobalToLocal,
                                                      mGrid);
    material = new Acts::InterpolatedMaterialMap<
        Acts::MaterialMapper<Acts::MaterialGrid2D>>(std::move(matMap),
                                                    bUtility);
    return;
  }
  if (bUtility.dimensions() == 3) {
    std::function<Acts::Vector3(Acts::Vector3)> transfoGlobalToLocal;
    Acts::Grid3D grid = createGrid3D(bUtility, transfoGlobalToLocal);

    Acts::Grid3D::point_t min = grid.minPosition();
    Acts::Grid3D::point_t max = grid.maxPosition();
    Acts::Grid3D::index_t nBins = grid.numLocalBins();

    Acts::EAxis axis1(min[0], max[0], nBins[0]);
    Acts::EAxis axis2(min[1], max[1], nBins[1]);
    Acts::EAxis axis3(min[2], max[2], nBins[2]);

    // Build the grid and fill it with data
    Acts::MaterialGrid3D mGrid(std::make_tuple(axis1, axis2, axis3));

    for (size_t bin = 0; bin < mmat.size(); bin++) {
      mGrid.at(bin) = mmat[bin].parameters();
    }
    Acts::MaterialMapper<Acts::MaterialGrid3D> matMap(transfoGlobalToLocal,
                                                      mGrid);
    material = new Acts::InterpolatedMaterialMap<
        Acts::MaterialMapper<Acts::MaterialGrid3D>>(std::move(matMap),
                                                    bUtility);
    return;
  }
}
