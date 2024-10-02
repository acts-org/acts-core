// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "DD4hepTestsHelper.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Plugins/DD4hep/DD4hepConversionHelpers.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <DD4hep/DD4hepUnits.h>

void DD4hepTestsHelper::decodeBinning(
    dd4hep::rec::VariantParameters& variantParams, const xml_comp_t& xmlBinning,
    const std::string& bname, const std::vector<std::string>& bvals) {
  // Set the surface binninng parameter to true
  variantParams.set<int>(bname + "_dim", bvals.size());
  for (const auto& bv : bvals) {
    // Gather the number of bins, 0 indicates variable binning
    int nBins = Acts::getAttrValueOr<int>(xmlBinning, "n" + bv, 0);
    // Gather the bin expansion parameter, expansion of 0 is default
    int nExpansion = Acts::getAttrValueOr<int>(xmlBinning, bv + "expansion", 0);
    // Auto-range detection
    bool autoRange =
        Acts::getAttrValueOr<bool>(xmlBinning, bv + "autorange", false);
    variantParams.set<bool>(bname + "_" + bv + "_autorange", autoRange);
    variantParams.set<int>(bname + "_" + bv + "_exp", nExpansion);
    // Equidistant binning detected
    if (nBins > 0) {
      // Set the type identification
      variantParams.set<std::string>(bname + "_" + bv + "_type", "equidistant");
      // Set the number of bins
      variantParams.set<int>(bname + "_" + bv + "_n", nBins);
      // Set min/max parameter
      if (!autoRange) {
        variantParams.set<double>(
            bname + "_" + bv + "_min",
            xmlBinning.attr<double>((bv + "min").c_str()));
        variantParams.set<double>(
            bname + "_" + bv + "_max",
            xmlBinning.attr<double>((bv + "max").c_str()));
      }
    } else {
      // Variable binning detected
      variantParams.set<std::string>(bname + "_" + bv + "_type", "variable");
      // Get the number of bins explicitly
      auto boundaries =
          xmlBinning.attr<std::string>((bv + "boundaries").c_str());
      std::string del = ",";
      auto end = boundaries.find(del);
      int ib = 0;
      // Unit conversion
      double unitScalar = 1.;
      if (bv != "phi") {
        unitScalar = Acts::UnitConstants::mm / dd4hep::millimeter;
      }
      // Split and convert
      while (end != std::string::npos) {
        double bR = unitScalar * dd4hep::_toFloat(boundaries.substr(0, end));
        variantParams.set<double>(
            bname + "_" + bv + "_b" + std::to_string(ib++), bR);
        boundaries.erase(boundaries.begin(), boundaries.begin() + end + 1);
        end = boundaries.find(del);
      }
      double bR = unitScalar * std::stod(boundaries.substr(0, end));
      variantParams.set<double>(bname + "_" + bv + "_b" + std::to_string(ib),
                                bR);
      // The number of bins are needed to unpack the data
      variantParams.set<int>(bname + "_" + bv + "_n", ib);
    }
  }
}

dd4hep::Transform3D DD4hepTestsHelper::createTransform(
    const xml_comp_t& x_det_comp) {
  // Build the transform - center def
  double cx = Acts::getAttrValueOr<double>(x_det_comp, "cx", 0.);
  double cy = Acts::getAttrValueOr<double>(x_det_comp, "cy", 0.);
  double cz = Acts::getAttrValueOr<double>(x_det_comp, "cz", 0.);

  double xx = Acts::getAttrValueOr<double>(x_det_comp, "xx", 1.);
  double xy = Acts::getAttrValueOr<double>(x_det_comp, "xy", 0.);
  double xz = Acts::getAttrValueOr<double>(x_det_comp, "xz", 0.);

  double yx = Acts::getAttrValueOr<double>(x_det_comp, "yx", 0.);
  double yy = Acts::getAttrValueOr<double>(x_det_comp, "yy", 1.);
  double yz = Acts::getAttrValueOr<double>(x_det_comp, "yz", 0.);

  Position xAxis(xx, xy, xz);
  Position yAxis(yx, yy, yz);
  Position zAxis = xAxis.Cross(yAxis);
  double zx = zAxis.X();
  double zy = zAxis.Y();
  double zz = zAxis.Z();

  // Create the transform
  return Transform3D(xx, yx, zx, cx, xy, yy, zy, cy, xz, yz, zz, cz);
}

std::string DD4hepTestsHelper::transformToXML(const Acts::Transform3& tf,
                                              const std::array<int, 2u>& axes) {
  auto tr = tf.translation();
  auto rot = tf.rotation();

  std::stringstream sxml;
  sxml << "cx=\"" << tr[0u] << "*mm\" ";
  sxml << "cy=\"" << tr[1u] << "*mm\" ";
  sxml << "cz=\"" << tr[2u] << "*mm\" ";

  sxml << "xx=\"" << rot.col(axes[0u])[0u] << "\" ";
  sxml << "xy=\"" << rot.col(axes[0u])[1u] << "\" ";
  sxml << "xz=\"" << rot.col(axes[0u])[2u] << "\" ";
  sxml << "yx=\"" << rot.col(axes[1u])[0u] << "\" ";
  sxml << "yy=\"" << rot.col(axes[1u])[1u] << "\" ";
  sxml << "yz=\"" << rot.col(axes[1u])[2u] << "\" ";

  return sxml.str();
}

std::string DD4hepTestsHelper::surfaceToXML(const Acts::GeometryContext& gctx,
                                            const Acts::Surface& surface,
                                            const Acts::Transform3& ref) {
  // The xml to be translated
  std::stringstream sxml;
  auto boundValues = surface.bounds().values();

  std::array<int, 2u> axes = {0, 1};
  // Change/adapt the behavior
  switch (surface.bounds().type()) {
    case Acts::SurfaceBounds::eRectangle: {
      sxml << "<box ";
      double dx = (boundValues[2u] - boundValues[0u]);
      double dy = (boundValues[3u] - boundValues[1u]);
      double dz = 0.125;
      sxml << "dx=\"" << dx << "*mm\" ";
      sxml << "dy=\"" << dy << "*mm\" ";
      sxml << "dz=\"" << dz << "*mm\" ";
    }; break;
    case Acts::SurfaceBounds::eTrapezoid: {
      axes = {2, 0};

      sxml << "<trap ";
      double hxmin = boundValues[0u];
      double hxmax = boundValues[1u];
      double dy = 2 * boundValues[2u];
      double dz = 0.125;
      sxml << "x1=\"" << hxmin << "*mm\" ";
      sxml << "x2=\"" << hxmax << "*mm\" ";
      sxml << "dy=\"" << dy << "*mm\" ";
      sxml << "dz=\"" << dz << "*mm\" ";
    }; break;
    default:
      break;
  }

  // Unwind the placement you have already
  auto relTransform = ref * surface.transform(gctx);
  sxml << transformToXML(relTransform, axes);
  sxml << " material=\"Air\"";
  sxml << " sensitive=\"true\"/>";
  return sxml.str();
}
