// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "createFeatures.hpp"

#include "Acts/Utilities/AngleHelpers.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

namespace ActsExamples {

std::vector<float> createFeatures(
    const SimSpacePointContainer& spacepoints,
    const std::optional<ClusterContainer>& clusters,
    const std::vector<TrackFindingAlgorithmExaTrkX::NodeFeature>& nodeFeatures,
    const std::vector<float>& featureScales) {
  using namespace ActsExamples;

  assert(nodeFeatures.size() == featureScales.size());
  std::vector<float> features(spacepoints.size() * nodeFeatures.size());

  for (auto isp = 0ul; isp < spacepoints.size(); ++isp) {
    const auto& sp = spacepoints[isp];

    // For now just take the first index since does require one single index
    // per spacepoint
    // TODO does it work for the module map construction to use only the first
    // sp?
    const auto& sl1 = sp.sourceLinks()[0].template get<IndexSourceLink>();

    // This should be fine, because check in constructor
    const Cluster* cl1 = clusters ? &clusters->at(sl1.index()) : nullptr;
    const Cluster* cl2 = cl1;

    if (sp.sourceLinks().size() == 2) {
      const auto& sl2 = sp.sourceLinks()[1].template get<IndexSourceLink>();
      cl2 = clusters ? &clusters->at(sl2.index()) : nullptr;
    }

    // I would prefer to use a std::span or boost::span here once available
    float* f = features.data() + isp * nodeFeatures.size();

    using NF = TrackFindingAlgorithmExaTrkX::NodeFeature;

    using namespace Acts::VectorHelpers;
    using namespace Acts::AngleHelpers;

    // clang-format off
#define MAKE_CLUSTER_FEATURES(n) \
    break; case NF::eCluster##n##X:   f[ift] = cl##n->globalPosition[Acts::ePos0]; \
    break; case NF::eCluster##n##Y:   f[ift] = cl##n->globalPosition[Acts::ePos1]; \
    break; case NF::eCluster##n##R:   f[ift] = perp(cl##n->globalPosition); \
    break; case NF::eCluster##n##Phi: f[ift] = phi(cl##n->globalPosition); \
    break; case NF::eCluster##n##Z:   f[ift] = cl##n->globalPosition[Acts::ePos2]; \
    break; case NF::eCluster##n##Eta: f[ift] = eta(cl##n->globalPosition); \
    break; case NF::eCellCount##n:    f[ift] = cl##n->channels.size(); \
    break; case NF::eChargeSum##n:    f[ift] = cl##n->sumActivations(); \
    break; case NF::eLocDir0##n:      f[ift] = cl##n->localDirection[0]; \
    break; case NF::eLocDir1##n:      f[ift] = cl##n->localDirection[1]; \
    break; case NF::eLocDir2##n:      f[ift] = cl##n->localDirection[2]; \
    break; case NF::eLengthDir0##n:   f[ift] = cl##n->lengthDirection[0]; \
    break; case NF::eLengthDir1##n:   f[ift] = cl##n->lengthDirection[1]; \
    break; case NF::eLengthDir2##n:   f[ift] = cl##n->lengthDirection[2]; \
    break; case NF::eLocEta##n:       f[ift] = cl##n->localEta; \
    break; case NF::eLocPhi##n:       f[ift] = cl##n->localPhi; \
    break; case NF::eGlobEta##n:      f[ift] = cl##n->globalEta; \
    break; case NF::eGlobPhi##n:      f[ift] = cl##n->globalPhi; \
    break; case NF::eEtaAngle##n:     f[ift] = cl##n->etaAngle; \
    break; case NF::ePhiAngle##n:     f[ift] = cl##n->phiAngle;
    // clang-format on

    Acts::Vector3 spPos{sp.x(), sp.y(), sp.z()};

    for (auto ift = 0ul; ift < nodeFeatures.size(); ++ift) {
      // clang-format off
      switch(nodeFeatures[ift]) {
        // Spacepoint features
        break; case NF::eR:           f[ift] = perp(spPos);
        break; case NF::ePhi:         f[ift] = phi(spPos);
        break; case NF::eZ:           f[ift] = sp.z();
        break; case NF::eX:           f[ift] = sp.x();
        break; case NF::eY:           f[ift] = sp.y();
        break; case NF::eEta:         f[ift] = eta(spPos);
        // Single cluster features
        break; case NF::eClusterLoc0: f[ift] = cl1->sizeLoc0;
        break; case NF::eClusterLoc1: f[ift] = cl1->sizeLoc1;
        break; case NF::eCellCount:   f[ift] = cl1->channels.size();
        break; case NF::eChargeSum:   f[ift] = cl1->sumActivations();
        // Features for split clusters
        MAKE_CLUSTER_FEATURES(1)
        MAKE_CLUSTER_FEATURES(2)
      }
      // clang-format on

      assert(std::isfinite(f[ift]));
      f[ift] /= featureScales[ift];
    }
#undef MAKE_CLUSTER_FEATURES
  }

  return features;
}

}  // namespace ActsExamples
