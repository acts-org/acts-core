// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Plugins/DD4hep/DD4hepBinningHelpers.hpp"
#include "Acts/Plugins/DD4hep/DD4hepConversionHelpers.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <DD4hep/DetFactoryHelper.h>
#include <DD4hep/Objects.h>
#include <DDRec/DetectorData.h>
#include <XML/Utilities.h>

#include "DD4hepTestsHelper.hpp"

using namespace dd4hep;

/// @brief  Helper method to add a layer to the detector
///
/// @param dd the detector to which this is added
/// @param dAssembly the detector assembly
/// @param x_layer the xml element describing the layer
/// @param sens the sensitive detector to be used
/// @param layerID the layer ID
///
/// @return a DetElement
DetElement addCylinderLayer(Detector &dd, Assembly &dAssembly,
                            const xml_comp_t &x_layer, SensitiveDetector sens,
                            int layerID) {
  // Make the cylinder detector element
  auto layerName = x_layer.nameStr();
  DetElement layerElement(layerName, layerID);
  // Layer parameters
  auto &layerParams =
      DD4hepTestsHelper::ensureExtension<dd4hep::rec::VariantParameters>(
          layerElement);

  // This should have a volume definition attached
  if (x_layer.hasChild(_Unicode(acts_volume))) {
    xml_comp_t actsVolume = x_layer.child(_Unicode(acts_volume));
    layerParams.set<bool>("acts_volume", true);
    layerParams.set<double>("acts_volume_z",
                            Acts::getAttrValueOr<double>(actsVolume, "cz", 0.));
    layerParams.set<int>("acts_volume_type", 3);
    layerParams.set<int>("acts_volume_bvalues_n", 3);
    layerParams.set<double>(
        "acts_volume_bvalues_0",
        Acts::getAttrValueOr<double>(actsVolume, "rmin", 0.));
    layerParams.set<double>(
        "acts_volume_bvalues_1",
        Acts::getAttrValueOr<double>(actsVolume, "rmax", 0.));
    layerParams.set<double>("acts_volume_bvalues_2",
                            Acts::getAttrValueOr<double>(actsVolume, "dz", 0.));

    layerParams.set<bool>("acts_volume_internals", true);
    layerParams.set<std::string>("acts_volume_internals_type", "layer");
  }

  // The layer Assembly
  Assembly layerAssembly(layerName + "_assembly");
  layerAssembly.setVisAttributes(dd, x_layer.visStr());

  // Active layer surfaces - Count and fill the sensors
  if (x_layer.hasChild(_Unicode(modules))) {
    // Check if the cylinder has a surface binning instruction
    if (x_layer.hasChild(_Unicode(acts_surface_binning))) {
      xml_comp_t sfBinning = x_layer.child(_Unicode(acts_surface_binning));
      DD4hepTestsHelper::decodeBinning(layerParams, sfBinning,
                                       "acts_surface_binning", {"z", "phi"});
    }
    // Go through the sensors
    unsigned int sensorID = 1u;
    xml_comp_t x_layer_modules = x_layer.child(_Unicode(modules));
    for (xml_coll_t bmodule(x_layer_modules, _U(box)); bmodule != nullptr;
         ++bmodule) {
      xml_comp_t x_layer_box = bmodule;

      // Due to convention this causes an axis flip on x
      Box boxShape(0.5 * x_layer_box.dx(), 0.5 * x_layer_box.dy(),
                   0.5 * x_layer_box.dz());

      // Set an orientation
      DetElement boxElement(layerName + "_module" + std::to_string(sensorID),
                            sensorID);

      Volume boxVolume(layerName, boxShape,
                       dd.material(x_layer_box.materialStr()));
      boxVolume.setVisAttributes(dd, x_layer_box.visStr());
      boxVolume.setSensitiveDetector(sens);

      PlacedVolume placedBox = layerAssembly.placeVolume(
          boxVolume, DD4hepTestsHelper::createTransform(x_layer_box));

      placedBox.addPhysVolID("sensor", sensorID++);
      boxElement.setPlacement(placedBox);
      // Add the module elements
      layerElement.add(boxElement);
    }
  }
  // Passive layer surface - place it inside the envelope
  for (xml_coll_t psurface(x_layer, _Unicode(acts_passive_surface));
       psurface != nullptr; ++psurface) {
    xml_comp_t x_passive_xml = psurface;
    // Direct definition of a child surface
    if (x_passive_xml.hasChild(_Unicode(tubs))) {
      xml_comp_t x_tubs_t = x_passive_xml.child(_Unicode(tubs));
      // Crete the corresponding detector element
      DetElement passiveElement(layerName + "_passiveEl", x_layer.id());
      Tube passiveShape(layerName + "_shape", x_tubs_t.rmin(), x_tubs_t.rmax(),
                        x_tubs_t.dz());
      Volume passiveVolume(layerName + "_volume", passiveShape,
                           dd.material(x_tubs_t.materialStr()));
      passiveVolume.setVisAttributes(dd, x_layer.visStr());
      // The places layer after all
      PlacedVolume placedPassive = layerAssembly.placeVolume(
          passiveVolume, DD4hepTestsHelper::createTransform(x_passive_xml));
      // Transport the passive surface knowledge
      auto &params =
          DD4hepTestsHelper::ensureExtension<dd4hep::rec::VariantParameters>(
              passiveElement);
      params.set<bool>("acts_passive_surface", true);
      // Set the placement and add
      passiveElement.setPlacement(placedPassive);
      // Add the module elements
      layerElement.add(passiveElement);
    }
  }

  auto placedLayer = dAssembly.placeVolume(layerAssembly);
  placedLayer.addPhysVolID("layer", layerID);
  layerElement.setPlacement(placedLayer);
  // Return the layer element
  return layerElement;
}

/// @brief  Helper method to add a layer to the detector
///
/// @param dd the detector to which this is added
/// @param dAssembly the detector assembly
/// @param x_layer the xml element describing the layer
/// @param sens the sensitive detector to be used
/// @param layerID the layer ID
///
/// @return a DetElement
DetElement addDiscLayer(Detector &dd, Assembly &dAssembly,
                        const xml_comp_t &x_layer, SensitiveDetector sens,
                        int layerID) {
  // Make the cylinder detector element
  auto layerName = x_layer.nameStr();
  DetElement layerElement(layerName, layerID);
  // Layer parameters
  auto &layerParams =
      DD4hepTestsHelper::ensureExtension<dd4hep::rec::VariantParameters>(
          layerElement);

  // This should have a volume definition attached
  if (x_layer.hasChild(_Unicode(acts_volume))) {
    xml_comp_t actsVolume = x_layer.child(_Unicode(acts_volume));
    layerParams.set<bool>("acts_volume", true);
    layerParams.set<double>("acts_volume_z",
                            Acts::getAttrValueOr<double>(actsVolume, "cz", 0.));
    layerParams.set<int>("acts_volume_type", 3);
    layerParams.set<int>("acts_volume_bvalues_n", 3);
    layerParams.set<double>(
        "acts_volume_bvalues_0",
        Acts::getAttrValueOr<double>(actsVolume, "rmin", 0.));
    layerParams.set<double>(
        "acts_volume_bvalues_1",
        Acts::getAttrValueOr<double>(actsVolume, "rmax", 0.));
    layerParams.set<double>("acts_volume_bvalues_2",
                            Acts::getAttrValueOr<double>(actsVolume, "dz", 0.));

    layerParams.set<bool>("acts_volume_internals", true);
    layerParams.set<std::string>("acts_volume_internals_type", "layer");
  }

  // The layer Assembly
  Assembly layerAssembly(layerName);
  layerAssembly.setVisAttributes(dd, x_layer.visStr());

  // Active layer surfaces - Count and fill the sensors
  if (x_layer.hasChild(_Unicode(modules))) {
    // Check if the cylinder has a surface binning instruction
    if (x_layer.hasChild(_Unicode(acts_surface_binning))) {
      xml_comp_t sfBinning = x_layer.child(_Unicode(acts_surface_binning));
      DD4hepTestsHelper::decodeBinning(layerParams, sfBinning,
                                       "acts_surface_binning", {"r", "phi"});
    }

    // Loop over modules
    unsigned int sensorID = 1u;
    xml_comp_t x_layer_modules = x_layer.child(_Unicode(modules));
    for (xml_coll_t bmodule(x_layer_modules, _U(trap)); bmodule != nullptr;
         ++bmodule) {
      xml_comp_t x_layer_trap = bmodule;

      // Due to convention this causes an axis flip on x
      Trapezoid trapShape(0.5 * x_layer_trap.dz(), 0.5 * x_layer_trap.dz(),
                          x_layer_trap.x1(), x_layer_trap.x2(),
                          0.5 * x_layer_trap.dy());

      // Set an orientation
      DetElement trapElement(layerName + "_module" + std::to_string(sensorID),
                             sensorID);
      auto &params =
          DD4hepTestsHelper::ensureExtension<dd4hep::rec::VariantParameters>(
              trapElement);
      params.set<std::string>("axis_definitions", "YZ");

      Volume trapVolume(layerName + "_vol", trapShape,
                        dd.material(x_layer_trap.materialStr()));
      trapVolume.setVisAttributes(dd, x_layer.visStr());
      trapVolume.setSensitiveDetector(sens);

      PlacedVolume placedTrap = layerAssembly.placeVolume(
          trapVolume, DD4hepTestsHelper::createTransform(x_layer_trap));

      placedTrap.addPhysVolID("sensor", sensorID++);
      trapElement.setPlacement(placedTrap);
      // Add the module elements
      layerElement.add(trapElement);
    }

    // Passive layer surface - place it inside the envelope
    for (xml_coll_t psurface(x_layer, _Unicode(acts_passive_surface));
         psurface != nullptr; ++psurface) {
      xml_comp_t x_passive_xml = psurface;
      // Direct definition of a child surface
      if (x_passive_xml.hasChild(_Unicode(tubs))) {
        xml_comp_t x_tubs_t = x_passive_xml.child(_Unicode(tubs));
        // Create the corresponding detector element
        DetElement passiveElement(layerName + "_passiveEl", x_layer.id());
        Tube passiveShape(layerName + "_passiveShape", x_tubs_t.rmin(),
                          x_tubs_t.rmax(), x_tubs_t.dz());
        Volume passiveVolume(layerName + "_passiveVolume", passiveShape,
                             dd.material(x_tubs_t.materialStr()));
        passiveVolume.setVisAttributes(dd, x_layer.visStr());
        // The places layer after all
        PlacedVolume placedPassive = layerAssembly.placeVolume(
            passiveVolume, DD4hepTestsHelper::createTransform(x_passive_xml));
        // Transport the passive surface knowledge
        auto &params =
            DD4hepTestsHelper::ensureExtension<dd4hep::rec::VariantParameters>(
                passiveElement);
        params.set<bool>("acts_passive_surface", true);
        // Set the placement and add
        passiveElement.setPlacement(placedPassive);
        // Add the module elements
        layerElement.add(passiveElement);
      }
    }
  }
  auto placedLayer = dAssembly.placeVolume(layerAssembly);
  placedLayer.addPhysVolID("layer", layerID);
  layerElement.setPlacement(placedLayer);
  // Return the layer element
  return layerElement;
}

/// Standard create_barrel_detector(...) create a barrel
/// like detector
///
/// @param dd the detector to which this is addedded
/// @param xml the input xml element
/// @param sens is ignored
///
/// @return a reference counted DetElement
static Ref_t create_barrel_detector(Detector &dd, xml_h xml,
                                    SensitiveDetector sens) {
  xml_det_t x_det = xml;
  std::string detName = x_det.nameStr();

  // create the master detector element
  DetElement detectorElement(detName, x_det.id());
  dd4hep::xml::setDetectorTypeFlag(xml, detectorElement);

  // The Shape and Volume
  Assembly detectorAssembly(detName);
  detectorAssembly.setVisAttributes(dd, x_det.visStr());

  // Add layers if they exist as such
  if (x_det.hasChild(_Unicode(layers))) {
    xml_comp_t x_det_layers = x_det.child(_Unicode(layers));
    int layerID = 0;
    for (xml_coll_t layer(x_det_layers, _Unicode(layer)); layer != nullptr;
         ++layer) {
      xml_comp_t x_det_layer = layer;
      auto layerElement =
          addCylinderLayer(dd, detectorAssembly, x_det_layer, sens, layerID++);
      // Add is to the detector element
      detectorElement.add(layerElement);
    }
  }
  // Place it into the mother volume
  Volume motherVolume = dd.pickMotherVolume(detectorElement);
  Position translation(0., 0., 0.);
  PlacedVolume placedDetector =
      motherVolume.placeVolume(detectorAssembly, translation);
  // "system" is hard coded in the DD4Hep::VolumeManager
  placedDetector.addPhysVolID("system", x_det.id());
  detectorElement.setPlacement(placedDetector);

  // return this element
  return detectorElement;
}

DECLARE_DETELEMENT(BarrelDetector, create_barrel_detector)

/// Standard create_endcap_detector(...) create a simple disc layer
///
/// @param dd the detector to which this is addedded
/// @param xml the input xml element
/// @param sens is ignored
///
/// @return a reference counted DetElement
static Ref_t create_endcap_detector(Detector &dd, xml_h xml,
                                    SensitiveDetector sens) {
  xml_det_t x_det = xml;
  std::string detName = x_det.nameStr();

  // create the master detector element
  DetElement detectorElement(detName, x_det.id());
  dd4hep::xml::setDetectorTypeFlag(xml, detectorElement);

  // The Shape and Volume
  Assembly detectorAssembly(detName);
  detectorAssembly.setVisAttributes(dd, x_det.visStr());

  // Add layers if they exist as such
  if (x_det.hasChild(_Unicode(layers))) {
    xml_comp_t x_det_layers = x_det.child(_Unicode(layers));
    int layerID = 0;
    for (xml_coll_t layer(x_det_layers, _Unicode(layer)); layer != nullptr;
         ++layer) {
      xml_comp_t x_det_layer = layer;
      auto layerElement =
          addDiscLayer(dd, detectorAssembly, x_det_layer, sens, layerID++);
      // Add the layer element to the detector element
      detectorElement.add(layerElement);
    }
  }

  // Place it into the mother volume
  Volume motherVolume = dd.pickMotherVolume(detectorElement);
  Position translation(0., 0., 0.);
  PlacedVolume placedDetector =
      motherVolume.placeVolume(detectorAssembly, translation);
  // "system" is hard coded in the DD4Hep::VolumeManager
  placedDetector.addPhysVolID("system", x_det.id());
  detectorElement.setPlacement(placedDetector);

  // return this element
  return detectorElement;
}

DECLARE_DETELEMENT(EndcapDetector, create_endcap_detector)
