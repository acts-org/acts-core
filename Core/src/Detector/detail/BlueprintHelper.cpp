// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/detail/BlueprintHelper.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"

#include <algorithm>
#include <array>

namespace {

std::array<Acts::Vector3, 2u> endPointsXYZ(
    const Acts::Experimental::Blueprint::Node& node, Acts::BinningValue bVal) {
  unsigned int bIdx = 0;
  switch (bVal) {
    case Acts::BinningValue::binX:
      bIdx = 0;
      break;
    case Acts::BinningValue::binY:
      bIdx = 1;
      break;
    case Acts::BinningValue::binZ:
      bIdx = 2;
      break;
    default:
      break;
  }
  Acts::Vector3 axis = node.transform.rotation().col(bIdx);
  auto halfL = node.boundaryValues[bIdx];
  Acts::Vector3 center = node.transform.translation();
  Acts::Vector3 p0 = center - halfL * axis;
  Acts::Vector3 p1 = center + halfL * axis;
  return {p0, p1};
}

}  // namespace

void Acts::Experimental::detail::BlueprintHelper::sort(Blueprint::Node& node,
                                                       bool recursive) {
  if (node.children.size() < 2u) {
    return;
  }
  // Sort along x, y, z
  if (node.binning.size() == 1) {
    auto bVal = node.binning.front();
    // x,y,z binning along the axis
    if (bVal == BinningValue::binX || bVal == BinningValue::binY ||
        bVal == BinningValue::binZ) {
      Vector3 nodeCenter = node.transform.translation();
      Vector3 nodeSortAxis = node.transform.rotation().col(toUnderlying(bVal));
      std::ranges::sort(node.children, {}, [&](const auto& c) {
        return (c->transform.translation() - nodeCenter).dot(nodeSortAxis);
      });
    } else if (bVal == BinningValue::binR &&
               node.boundsType == VolumeBounds::eCylinder) {
      std::ranges::sort(node.children, {}, [](const auto& c) {
        return c->boundaryValues[0] + c->boundaryValues[1];
      });
    }
  }

  // Sort the children
  if (recursive) {
    for (auto& child : node.children) {
      sort(*child, true);
    }
  }
}

void Acts::Experimental::detail::BlueprintHelper::fillGaps(
    Blueprint::Node& node, bool adjustToParent) {
  // Return if this is a leaf node
  if (node.isLeaf()) {
    return;
  }
  if (node.boundsType == VolumeBounds::eCylinder && node.binning.size() == 1) {
    fillGapsCylindrical(node, adjustToParent);
  } else if (node.boundsType == VolumeBounds::eCuboid &&
             node.binning.size() == 1) {
    // Doesn't look like NOT adjusting to parent
    // makes sense. The gaps are not going
    // to be filled in non-binned directions
    fillGapsCuboidal(node, adjustToParent);
  } else {
    throw std::runtime_error(
        "BlueprintHelper: gap filling is not implemented for "
        "this boundary type");
  }
}

void Acts::Experimental::detail::BlueprintHelper::fillGapsCylindrical(
    Blueprint::Node& node, bool adjustToParent) {
  // Nodes must be sorted
  sort(node, false);

  // Container values
  auto cInnerR = node.boundaryValues[0];
  auto cOuterR = node.boundaryValues[1];
  auto cHalfZ = node.boundaryValues[2];

  std::vector<std::unique_ptr<Blueprint::Node>> gaps;
  // Only 1D binning implemented for the moment
  if (BinningValue bVal = node.binning.front(); bVal == BinningValue::binZ) {
    // adjust inner/outer radius
    if (adjustToParent) {
      std::for_each(node.children.begin(), node.children.end(),
                    [&](auto& child) {
                      child->boundaryValues[0] = cInnerR;
                      child->boundaryValues[1] = cOuterR;
                    });
    }
    auto [negC, posC] = endPointsXYZ(node, bVal);
    // Assume sorted along the local z axis
    unsigned int igap = 0;
    for (auto& child : node.children) {
      auto [neg, pos] = endPointsXYZ(*child, bVal);
      ActsScalar gapSpan = (neg - negC).norm();
      if (gapSpan > s_onSurfaceTolerance) {
        // Fill a gap node
        auto gapName = node.name + "_gap_" + std::to_string(igap);
        auto gapTransform = Transform3::Identity();
        gapTransform.rotate(node.transform.rotation());
        gapTransform.pretranslate(0.5 * (neg + negC));
        auto gap = std::make_unique<Blueprint::Node>(
            gapName, gapTransform, VolumeBounds::eCylinder,
            std::vector<ActsScalar>{cInnerR, cOuterR, 0.5 * gapSpan});
        gaps.push_back(std::move(gap));
        ++igap;
      }
      // Set to new current negative value
      negC = pos;
    }
    // Check if a last one needs to be filled
    ActsScalar gapSpan = (negC - posC).norm();
    if (gapSpan > s_onSurfaceTolerance) {
      // Fill a gap node
      auto gapName = node.name + "_gap_" + std::to_string(igap);
      auto gapTransform = Transform3::Identity();
      gapTransform.rotate(node.transform.rotation());
      gapTransform.pretranslate(0.5 * (negC + posC));
      auto gap = std::make_unique<Blueprint::Node>(
          gapName, gapTransform, VolumeBounds::eCylinder,
          std::vector<ActsScalar>{cInnerR, cOuterR, 0.5 * gapSpan});
      gaps.push_back(std::move(gap));
    }

  } else if (bVal == BinningValue::binR) {
    // We have binning in R present
    if (adjustToParent) {
      std::for_each(node.children.begin(), node.children.end(),
                    [&](auto& child) {
                      child->transform = node.transform;
                      child->boundaryValues[2] = cHalfZ;
                    });
    }
    // Fill the gaps in R
    unsigned int igap = 0;
    ActsScalar lastR = cInnerR;
    for (auto& child : node.children) {
      ActsScalar iR = child->boundaryValues[0];
      if (std::abs(iR - lastR) > s_onSurfaceTolerance) {
        auto gap = std::make_unique<Blueprint::Node>(
            node.name + "_gap_" + std::to_string(igap), node.transform,
            VolumeBounds::eCylinder,
            std::vector<ActsScalar>{lastR, iR, cHalfZ});
        gaps.push_back(std::move(gap));
        ++igap;
      }
      // Set to new outer radius
      lastR = child->boundaryValues[1];
    }
    // Check if a last one needs to be filled
    if (std::abs(lastR - cOuterR) > s_onSurfaceTolerance) {
      auto gap = std::make_unique<Blueprint::Node>(
          node.name + "_gap_" + std::to_string(igap), node.transform,
          VolumeBounds::eCylinder,
          std::vector<ActsScalar>{lastR, cOuterR, cHalfZ});
      gaps.push_back(std::move(gap));
    }
  } else {
    throw std::runtime_error(
        "BlueprintHelper: gap filling not implemented for "
        "cylinder and this binning type.");
  }

  // Insert
  for (auto& gap : gaps) {
    node.add(std::move(gap));
  }

  // Sort again after inserting
  sort(node, false);
  // Fill the gaps recursively
  for (auto& child : node.children) {
    fillGaps(*child, adjustToParent);
  }
}

void Acts::Experimental::detail::BlueprintHelper::fillGapsCuboidal(
    Blueprint::Node& node, bool adjustToParent) {
  // Nodes must be sorted
  sort(node, false);

  // Cuboidal detector binnings
  std::array<Acts::BinningValue, 3u> allowedBinVals = {
      BinningValue::binX, BinningValue::binY, BinningValue::binZ};

  std::vector<std::unique_ptr<Blueprint::Node>> gaps;
  auto binVal = node.binning.front();

  // adjust non-binned directions
  if (adjustToParent) {
    std::for_each(node.children.begin(), node.children.end(), [&](auto& child) {
      for (auto bv : allowedBinVals) {
        if (bv != binVal) {
          // Both boundary values and translation
          // have to be adjusted
          child->boundaryValues[toUnderlying(bv)] =
              node.boundaryValues[toUnderlying(bv)];
          child->transform.translation()[toUnderlying(bv)] =
              node.transform.translation()[toUnderlying(bv)];
        }
      }
    });
  }
  auto [negC, posC] = endPointsXYZ(node, binVal);

  // Assume sorted along the local binned axis
  unsigned int igap = 0;
  for (auto& child : node.children) {
    auto [neg, pos] = endPointsXYZ(*child, binVal);
    ActsScalar gapSpan = (neg - negC).norm();
    if (gapSpan > s_onSurfaceTolerance) {
      // Fill a gap node
      auto gapName = node.name + "_gap_" + std::to_string(igap);
      auto gapTransform = Transform3::Identity();
      gapTransform.rotate(node.transform.rotation());
      gapTransform.pretranslate(0.5 * (neg + negC));
      std::vector<ActsScalar> gapBounds{0, 0, 0};
      gapBounds[toUnderlying(binVal)] = 0.5 * gapSpan;
      for (auto bv : allowedBinVals) {
        if (bv != binVal) {
          gapBounds[toUnderlying(bv)] = node.boundaryValues[toUnderlying(bv)];
        }
      }
      auto gap = std::make_unique<Blueprint::Node>(
          gapName, gapTransform, VolumeBounds::eCuboid, gapBounds);
      gaps.push_back(std::move(gap));
      ++igap;
    }
    // Set to new current negative value
    negC = pos;
  }
  // Check if a last one needs to be filled
  ActsScalar gapSpan = (negC - posC).norm();
  if (gapSpan > s_onSurfaceTolerance) {
    // Fill a gap node
    auto gapName = node.name + "_gap_" + std::to_string(igap);
    auto gapTransform = Transform3::Identity();
    gapTransform.rotate(node.transform.rotation());
    gapTransform.pretranslate(0.5 * (negC + posC));
    std::vector<ActsScalar> gapBounds{0, 0, 0};
    gapBounds[toUnderlying(binVal)] = 0.5 * gapSpan;
    for (auto bv : allowedBinVals) {
      if (bv != binVal) {
        gapBounds[toUnderlying(bv)] = node.boundaryValues[toUnderlying(bv)];
      }
    }
    auto gap = std::make_unique<Blueprint::Node>(
        gapName, gapTransform, VolumeBounds::eCuboid, gapBounds);
    gaps.push_back(std::move(gap));
  }

  // Insert
  for (auto& gap : gaps) {
    node.add(std::move(gap));
  }

  // Sort again after inserting
  sort(node, false);
  // Fill the gaps recursively
  for (auto& child : node.children) {
    fillGaps(*child, adjustToParent);
  }
}
