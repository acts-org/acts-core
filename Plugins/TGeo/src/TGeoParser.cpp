// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/TGeo/TGeoParser.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/TGeo/TGeoPrimitivesHelper.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include "RtypesCore.h"
#include "TCollection.h"
#include "TGeoBBox.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "TObjArray.h"
#include "TObject.h"

void Acts::TGeoParser::select(Acts::TGeoParser::State& state,
                              const Acts::TGeoParser::Options& options,
                              const TGeoMatrix& gmatrix) {
  // Volume is present
  if (state.volume != nullptr) {
    std::string volumeName = state.volume->GetName();
    // If you are on branch, you stay on branch
    state.onBranch =
        state.onBranch ||
        TGeoPrimitivesHelper::match(options.volumeNames, volumeName.c_str());
    // Loop over the daughters and collect them
    auto daughters = state.volume->GetNodes();
    // Daughter node iteration
    TIter iObj(daughters);
    while (TObject* obj = iObj()) {
      TGeoNode* node = dynamic_cast<TGeoNode*>(obj);
      if (node != nullptr) {
        state.volume = nullptr;
        state.node = node;
        select(state, options, gmatrix);
      }
    }
  } else if (state.node != nullptr) {
    // The node name for checking
    std::string nodeName = state.node->GetName();
    std::string nodeVolName = state.node->GetVolume()->GetName();
    // Get the matrix of the current node for positioning
    const TGeoMatrix* nmatrix = state.node->GetMatrix();
    TGeoHMatrix transform = TGeoCombiTrans(gmatrix) * TGeoCombiTrans(*nmatrix);
    std::string suffix = "_transform";
    transform.SetName((nodeName + suffix).c_str());
    // Check if you had found the target node
    if (state.onBranch &&
        TGeoPrimitivesHelper::match(options.targetNames, nodeVolName.c_str())) {
      // Get the placement and orientation in respect to its mother
      const Double_t* rotation = transform.GetRotationMatrix();
      const Double_t* translation = transform.GetTranslation();

      // Create a eigen transform
      Vector3 t(options.unit * translation[0], options.unit * translation[1],
                options.unit * translation[2]);
      Vector3 cx(rotation[0], rotation[3], rotation[6]);
      Vector3 cy(rotation[1], rotation[4], rotation[7]);
      Vector3 cz(rotation[2], rotation[5], rotation[8]);
      auto etrf = TGeoPrimitivesHelper::makeTransform(cx, cy, cz, t);

      bool accept = true;
      if (!options.parseRanges.empty()) {
        // It uses the bounding box of TGeoBBox
        if (auto* shape =
                dynamic_cast<TGeoBBox*>(state.node->GetVolume()->GetShape());
            shape != nullptr) {
          // @TODO this should be replace by a proper TGeo to Acts::VolumeBounds
          // and vertices converision which would make a more appropriate
          // parsomg
          double dx = options.unit * shape->GetDX();
          double dy = options.unit * shape->GetDY();
          double dz = options.unit * shape->GetDZ();
          for (auto x : std::vector<double>{-dx, dx}) {
            for (auto y : std::vector<double>{-dy, dy}) {
              for (auto z : std::vector<double>{-dz, dz}) {
                Vector3 edge = etrf * Vector3(x, y, z);
                for (auto& check : options.parseRanges) {
                  double val = VectorHelpers::cast(edge, check.first);
                  if (val < check.second.first || val > check.second.second) {
                    accept = false;
                    break;
                  }
                }
              }
            }
          }
        }
      }
      if (accept) {
        state.selectedNodes.push_back(
            {state.node, std::make_unique<TGeoHMatrix>(transform)});
      }
      state.node = nullptr;
    } else {
      // If it's not accepted, get the associated volume
      state.volume = state.node->GetVolume();
      state.node = nullptr;
      // Set one further down
      select(state, options, transform);
    }
  }
  return;
}

// Function to recursively find the node by volume name
TGeoNode* Acts::TGeoParser::findNodeRecursive(TGeoNode* currentNode,
                                              const char* volumeName) {
  // Check if the current node's volume matches the name
  if (std::strcmp(currentNode->GetVolume()->GetName(), volumeName) == 0) {
    return currentNode;
  }
  // Recursively search in the daughter nodes
  int nDaughters = currentNode->GetNdaughters();
  for (int i = 0; i < nDaughters; ++i) {
    TGeoNode* daughterNode = currentNode->GetDaughter(i);
    TGeoNode* foundNode = findNodeRecursive(daughterNode, volumeName);
    if (foundNode != nullptr) {
      return foundNode;
    }
  }
  return nullptr;  // Not found in this branch
}
