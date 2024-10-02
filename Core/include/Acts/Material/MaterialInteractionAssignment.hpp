// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/interface/IAssignmentFinder.hpp"

#include <array>
#include <functional>
#include <optional>
#include <tuple>
#include <vector>

namespace Acts {

class Surface;

namespace MaterialInteractionAssignment {

/// The result struct of the assignment run
struct Result {
  /// The assigned material interactions
  std::vector<MaterialInteraction> assigned = {};
  /// The unassigned material interactions
  std::vector<MaterialInteraction> unassigned = {};
  /// Surfaces without assignment (for empty hit correction)
  std::vector<IAssignmentFinder::SurfaceAssignment> unassignedSurfaces = {};
};

/// @brief definition of a global veto for assigning material interactions
///
/// This can be used to restrict the assignment to a specific volume, or
/// exclude certain materials (if one wants), etc.
///
/// @param materialInteraction is the material interaction to be checked for the veto
///
/// It will be globally applied, i.e. every single material interaction
/// will have to go through this veto
using GlobalVeto =
    std::function<bool(const MaterialInteraction& materialInteraction)>;

/// @brief definition of a local veto on a material interaction
///
/// This can take already the suggested surface assignment into account
/// return true if the assignment should be vetoed. This can be used for
/// having exclusion rules based on surface information.
///
/// @param materialInteraction is the material interaction to be checked for the veto
/// @param suggestedAssignment is the suggested assignment: surface, position, direction
using LocalVeto = std::function<bool(
    const MaterialInteraction&,
    const IAssignmentFinder::SurfaceAssignment& suggestedAssignment)>;

/// @brief definition of possible re-assignments to next surface, this could e.g.
/// be used for respecting pre/post mapping directives that are not fully
/// handled by closest distance matching
///
/// The provided parameters are the mutable material interaction, the suggested
/// assignment and the next possible assignment, due to the ordered nature of
/// the material interactions, assignment to previous is excluded
///
/// @param materialInteraction is the material interaction to be checked for the veto
/// @param suggestedAssignment is the suggested assignment: surface, position, direction
/// @param suggestedReAssignment is the suggested assignment: surface, position, direction
///
/// @note this changes the MaterialInteraction if the re-assignment is accepted
using ReAssignment = std::function<void(
    MaterialInteraction& materialInteraction,
    const IAssignmentFinder::SurfaceAssignment& suggestedAssignment,
    const IAssignmentFinder::SurfaceAssignment& suggestedReAssignment)>;

/// @brief Options for the material interaction matcher
/// The options are used to specify the vetos for the assignment
struct Options {
  /// Allow global vetos for the assignment, e.g. restricting
  /// the assignment to a specific volume
  std::vector<GlobalVeto> globalVetos = {};

  /// Allow specific vetoes for the assignment, e.g. only locally to
  /// surface, rest to next mapper (e.g. volume mapper)
  GeometryHierarchyMap<LocalVeto> localVetos = {};

  /// Allow re-assignment of the material interaction, e.g. to respect
  GeometryHierarchyMap<ReAssignment> reAssignments = {};
};

/// @brief Match the material interactions to surfaces intersections while respecting
/// eventual vetos for the assignment
///
/// @param gctx is the geometry context
/// @param materialInteractions is the vector of material interaction
/// @param intersectedSurfaces are the surfac assignment candidates
/// @param options are the options for the assignment
///
/// @return a pair of vectors of assigned and unassigned material interactions
Result assign(const GeometryContext& gctx,
              const std::vector<MaterialInteraction>& materialInteractions,
              const std::vector<IAssignmentFinder::SurfaceAssignment>&
                  intersectedSurfaces,
              const Options& options = Options());

}  // namespace MaterialInteractionAssignment

}  // namespace Acts
