// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ActSVG/SurfaceArraySvgConverter.hpp"

#include "Acts/Plugins/ActSVG/SurfaceSvgConverter.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <algorithm>
#include <numbers>

std::tuple<std::vector<Acts::Svg::ProtoSurfaces>, Acts::Svg::ProtoGrid,
           std::vector<Acts::Svg::ProtoAssociations> >
Acts::Svg::SurfaceArrayConverter::convert(
    const GeometryContext& gctx, const SurfaceArray& surfaceArray,
    const SurfaceArrayConverter::Options& cOptions) {
  // Prepare the return objects
  ProtoSurfaces pSurfaces;
  ProtoGrid pGrid;
  ProtoAssociations pAssociations;

  const auto& surfaces = surfaceArray.surfaces();

  // The edges of the grid
  auto binning = surfaceArray.binningValues();
  auto axes = surfaceArray.getAxes();

  enum ViewType { cylinder, polar, planar, none };
  ViewType vType = none;

  if (!binning.empty() && binning.size() == 2 && axes.size() == 2) {
    // The endges values
    std::vector<double> edges0;
    std::vector<double> edges1;
    // Helper method to convert from ACTS to Grid edges
    auto convertGridEdges = [](const std::vector<double>& actsEdges)
        -> std::vector<actsvg::scalar> {
      std::vector<actsvg::scalar> svgEdges;
      svgEdges.reserve(actsEdges.size());
      for (const auto ae : actsEdges) {
        svgEdges.push_back(static_cast<actsvg::scalar>(ae));
      }
      return svgEdges;
    };

    // Walk through the binning and translate
    if (binning[0] == AxisDirection::AxisPhi &&
        binning[1] == AxisDirection::AxisZ) {
      vType = cylinder;
      //  flip to fit with actsvg convention
      edges1 = axes[0]->getBinEdges();
      edges0 = axes[1]->getBinEdges();
      pGrid._type = actsvg::proto::grid::e_z_phi;
    } else if (binning[0] == AxisDirection::AxisPhi &&
               binning[1] == AxisDirection::AxisR) {
      vType = polar;
      //  flip to fit with actsvg convention
      edges1 = axes[0]->getBinEdges();
      edges0 = axes[1]->getBinEdges();
      pGrid._type = actsvg::proto::grid::e_r_phi;
    } else if (binning[0] == AxisDirection::AxisZ &&
               binning[1] == AxisDirection::AxisPhi) {
      // good
      vType = cylinder;
      edges0 = axes[0]->getBinEdges();
      edges1 = axes[1]->getBinEdges();
      pGrid._type = actsvg::proto::grid::e_z_phi;
    } else if (binning[0] == AxisDirection::AxisR &&
               binning[1] == AxisDirection::AxisPhi) {
      // good
      vType = polar;
      edges0 = axes[0]->getBinEdges();
      edges1 = axes[1]->getBinEdges();
      pGrid._type = actsvg::proto::grid::e_r_phi;
    }
    // Assign
    pGrid._edges_0 = convertGridEdges(edges0);
    pGrid._edges_1 = convertGridEdges(edges1);
  }

  // Find the template surfaces & prepare template objects to be assigned
  std::vector<actsvg::svg::object> templateObjects;
  std::vector<const SurfaceBounds*> templateBounds;

  for (const auto& sf : surfaces) {
    // Get bounds and check them
    const SurfaceBounds& sBounds = sf->bounds();
    // Helper to find bounds
    auto sameBounds = [&](const SurfaceBounds* test) {
      return ((*test) == sBounds);
    };
    // Check if you have this template object already before creating new
    // reference bounds and new reference object
    if (std::ranges::none_of(templateBounds, sameBounds)) {
      // Let's get the right style
      SurfaceConverter::Options sOptions;
      sOptions.templateSurface = true;
      // Find a corresponding file in the playbook
      auto sfStyle = cOptions.surfaceStyles.find(sf->geometryId());
      if (sfStyle != cOptions.surfaceStyles.end()) {
        sOptions.style = *sfStyle;
      }

      // Create a reference surface and reference object from it
      auto referenceSurface = SurfaceConverter::convert(gctx, *sf, sOptions);
      auto referenceObject =
          View::xy(referenceSurface,
                   "Template_" + std::to_string(templateObjects.size()));
      templateBounds.push_back(&sBounds);
      templateObjects.push_back(referenceObject);
    }
  }

  // Estimate a reference radius
  double radius = 0.;

  // Now draw the surfaces from the correct template
  for (const auto& sf : surfaces) {
    radius += Acts::VectorHelpers::perp(sf->center(gctx));

    // Let's get the right style
    SurfaceConverter::Options sOptions;
    sOptions.templateSurface = vType != cylinder;
    // Find a corresponding file in the playbook
    auto sfStyle = cOptions.surfaceStyles.find(sf->geometryId());
    if (sfStyle != cOptions.surfaceStyles.end()) {
      sOptions.style = *sfStyle;
    }

    // Convert the surface from ACTS to actsvg
    auto cSurface = Acts::Svg::SurfaceConverter::convert(gctx, *sf, sOptions);
    cSurface._name = "Module_n_" + std::to_string(pSurfaces.size());

    cSurface._aux_info["grid_info"] = {
        "* module " + std::to_string(pSurfaces.size()) +
        ", surface = " + std::to_string(sf->geometryId().sensitive())};
    // Assign the template for cylinder layers
    if (vType == cylinder) {
      const SurfaceBounds& sBounds = sf->bounds();
      // Helper to find bounds
      auto sameBounds = [&](const SurfaceBounds* test) {
        return ((*test) == sBounds);
      };
      // Check if you have this template object already
      auto tBounds = std::ranges::find_if(templateBounds, sameBounds);
      // New reference bounds and new reference object
      if (tBounds != templateBounds.end()) {
        std::size_t tObject = std::distance(templateBounds.begin(), tBounds);
        cSurface._template_object = templateObjects[tObject];
      }
    }
    // Correct view transform for disc/planar layers
    if (vType == planar || vType == polar) {
      // Get the transform and estimate the rotation of phi
      // Assumes x/y view
      const auto& sTransform = sf->transform(gctx);
      Vector3 localA = sTransform.rotation().col(0);
      Vector3 localZ = sTransform.rotation().col(2);
      // Find out orientation w.r.t. global transform
      double projZ = localZ.dot(Vector3(0., 0., 1.));
      double alpha = std::atan2(localA[1], localA[0]) / std::numbers::pi * 180.;
      if (projZ < 0.) {
        alpha += 180.;
      }
      auto surfaceCenter = sf->center(gctx);
      // Set the transform for an eventual placement
      cSurface._transform._tr = {static_cast<actsvg::scalar>(surfaceCenter[0]),
                                 static_cast<actsvg::scalar>(surfaceCenter[1])};
      cSurface._transform._rot = {static_cast<actsvg::scalar>(alpha), 0., 0.};
    }

    pSurfaces.push_back(cSurface);
  }
  radius /= surfaces.size();

  // Create the bin associations
  for (unsigned int il0 = 1; il0 < pGrid._edges_0.size(); ++il0) {
    double p0 = 0.5 * (pGrid._edges_0[il0] + pGrid._edges_0[il0 - 1]);
    for (unsigned int il1 = 1; il1 < pGrid._edges_1.size(); ++il1) {
      double p1 = 0.5 * (pGrid._edges_1[il1] + pGrid._edges_1[il1 - 1]);
      // Create the fitting bin center estimates
      Vector3 bCenter;
      if (vType == polar) {
        bCenter = Vector3(p0 * std::cos(p1), p0 * std::sin(p1), 0.);
      } else if (vType == cylinder) {
        bCenter = Vector3(radius * std::cos(p1), radius * std::sin(p1), p0);
      }
      // Get all the bin entries and members
      auto bSurfaces = surfaceArray.neighbors(bCenter);
      std::vector<std::size_t> binnAssoc;
      for (const auto& bs : bSurfaces) {
        auto candidate = std::ranges::find(surfaces, bs);
        if (candidate != surfaces.end()) {
          binnAssoc.push_back(std::distance(surfaces.begin(), candidate));
        }
      }
      pAssociations.push_back(binnAssoc);
    }
  }
  // Return the surfaces and the grid
  std::vector<ProtoSurfaces> pSurfaceBatches = {pSurfaces};
  std::vector<ProtoAssociations> pAssociationBatchs = {pAssociations};
  return {pSurfaceBatches, pGrid, pAssociationBatchs};
}
