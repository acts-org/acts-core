// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/MaterialDesignatorBlueprintNode.hpp"

#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Portal.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/GraphViz.hpp"
#include "Acts/Utilities/Helpers.hpp"

namespace Acts {

const std::string& MaterialDesignatorBlueprintNode::name() const {
  return m_name;
}

void MaterialDesignatorBlueprintNode::toStream(std::ostream& os) const {
  os << "MaterialDesignatorBlueprintNode(" << name() << ")";
}

Volume& MaterialDesignatorBlueprintNode::build(const BlueprintOptions& options,
                                               const GeometryContext& gctx,
                                               const Logger& logger) {
  if (children().size() != 1) {
    ACTS_ERROR(prefix() << "MaterialDesignatorBlueprintNode must have exactly "
                           "one child, but has "
                        << children().size());
    throw std::runtime_error(
        "MaterialDesignatorBlueprintNode must have exactly one child");
  }

  if (!m_binning) {
    ACTS_ERROR(prefix() << "Binning is not set");
    throw std::runtime_error("Binning is not set");
  }

  return children().at(0).build(options, gctx, logger);
}

void MaterialDesignatorBlueprintNode::handleCylinderBinning(
    CylinderPortalShell& cylShell,
    const std::vector<
        std::tuple<CylinderPortalShell::Face, Experimental::ProtoBinning,
                   Experimental::ProtoBinning>>& binning,
    const Logger& logger) {
  ACTS_DEBUG(prefix() << "Binning is set to compatible type");
  using enum CylinderVolumeBounds::Face;

  for (auto& [face, loc0, loc1] : binning) {
    if (face == OuterCylinder || face == InnerCylinder) {
      if (loc0.axisDir != AxisDirection::AxisRPhi) {
        ACTS_ERROR(prefix() << "Binning is not in RPhi");
        throw std::runtime_error("Binning is not in RPhi");
      }

      if (loc1.axisDir != AxisDirection::AxisZ) {
        ACTS_ERROR(prefix() << "Binning is not in Z");
        throw std::runtime_error("Binning is not in Z");
      }
    }

    if (face == PositiveDisc || face == NegativeDisc) {
      if (loc0.axisDir != AxisDirection::AxisR) {
        ACTS_ERROR(prefix() << "Binning is not in R");
        throw std::runtime_error("Binning is not in R");
      }
      if (loc1.axisDir != AxisDirection::AxisPhi) {
        ACTS_ERROR(prefix() << "Binning is not in Phi");
        throw std::runtime_error("Binning is not in Phi");
      }
    }

    Experimental::BinningDescription desc{.binning = {loc0, loc1}};
    ACTS_DEBUG(prefix() << "~> Assigning proto binning " << desc.toString()
                        << " to face " << face);

    auto material = std::make_shared<ProtoGridSurfaceMaterial>(std::move(desc));

    auto portal = cylShell.portal(face);
    if (portal == nullptr) {
      ACTS_ERROR(prefix() << "Portal is nullptr");
      throw std::runtime_error("Portal is nullptr");
    }
    portal->surface().assignSurfaceMaterial(std::move(material));
  }
}

PortalShellBase& MaterialDesignatorBlueprintNode::connect(
    const BlueprintOptions& options, const GeometryContext& gctx,
    const Logger& logger) {
  ACTS_DEBUG(prefix() << "MaterialDesignatorBlueprintNode::connect");
  if (children().size() != 1) {
    ACTS_ERROR(prefix() << "MaterialDesignatorBlueprintNode must have exactly "
                           "one child, but has "
                        << children().size());
    throw std::runtime_error(
        "MaterialDesignatorBlueprintNode must have exactly one child");
  }
  if (!m_binning) {
    ACTS_ERROR(prefix() << "Binning is not set");
    throw std::runtime_error("Binning is not set");
  }

  auto& shell = children().at(0).connect(options, gctx, logger);

  ACTS_DEBUG(prefix() << "Received shell from child "
                      << children().at(0).name());

  if (auto* cylShell = dynamic_cast<CylinderPortalShell*>(&shell)) {
    ACTS_DEBUG(prefix() << "Connecting cylinder shell");

    if (const auto* binning = std::get_if<std::vector<
            std::tuple<CylinderPortalShell::Face, Experimental::ProtoBinning,
                       Experimental::ProtoBinning>>>(&m_binning.value());
        binning != nullptr) {
      handleCylinderBinning(*cylShell, *binning, logger);
    } else {
      ACTS_ERROR(prefix() << "Binning is set to unknown type");
      throw std::runtime_error("Unknown binning type");
    }

  }
  // @TODO: Handle cuboid volume shell here
  else {
    ACTS_ERROR(prefix() << "Shell is not supported");
    throw std::runtime_error("Shell is not supported");
  }

  return shell;
}

void MaterialDesignatorBlueprintNode::finalize(const BlueprintOptions& options,
                                               const GeometryContext& gctx,
                                               TrackingVolume& parent,
                                               const Logger& logger) {
  if (children().size() != 1) {
    throw std::runtime_error(
        "MaterialDesignatorBlueprintNode must have exactly one child");
  }
  return children().at(0).finalize(options, gctx, parent, logger);
}

void MaterialDesignatorBlueprintNode::addToGraphviz(std::ostream& os) const {
  if (!m_binning) {
    throw std::runtime_error("Binning is not set");
  }

  std::stringstream ss;
  ss << "" + name() + "";
  ss << "<br/><i>CylinderContainer</i>";

  std::visit(
      overloaded{
          [&](const std::vector<
              std::tuple<CylinderPortalShell::Face, Experimental::ProtoBinning,
                         Experimental::ProtoBinning>>& binning) {
            for (const auto& [face, loc0, loc1] : binning) {
              ss << "<br/>" << face;
              ss << ": " << loc0.axisDir << "=" << loc0.bins();
              ss << ", " << loc1.axisDir << "=" << loc1.bins();
            }
          },
          [](const auto& /*binning*/) {
            // No output in all other cases
          }},
      m_binning.value());
  os << GraphViz::Node{
      .id = name(), .label = ss.str(), .shape = GraphViz::Shape::Hexagon};
  BlueprintNode::addToGraphviz(os);
}

const std::optional<MaterialDesignatorBlueprintNode::BinningConfig>&
MaterialDesignatorBlueprintNode::binning() const {
  return m_binning;
}

MaterialDesignatorBlueprintNode& MaterialDesignatorBlueprintNode::setBinning(
    BinningConfig binning) {
  m_binning = std::move(binning);
  return *this;
}

}  // namespace Acts
