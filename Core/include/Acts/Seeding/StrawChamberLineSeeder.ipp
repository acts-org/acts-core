// This file is part of the ACTS project.
//
// Copyright (C) 2025 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/StringHelpers.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Definitions/Units.hpp"


namespace Acts{

    template <StationSpacePoint UncalibSp_t, 
              StationSpacePointSorter<UncalibSp_t> Sorter_t>
    StrawChamberLineSeeder<UncalibSp_t,Sorter_t>::StrawChamberLineSeeder(const UnCalibHitVec_t& seedHits,
                                                                        Config&& cfg,
                                                                        std::unique_ptr<const Acts::Logger> logObj):
        m_hitLayers{seedHits},
        m_cfg{std::move(cfg)},
        m_logger{std::move(logObj)} {
            
        /** The StrawChamberLine needs to have at least 2 straw layers */
        if (m_hitLayers.strawHits().size() < 2) {
            ACTS_VERBOSE("Not enough straw layers have been parsed");
            return;
        }
        if (std::ranges::find_if(m_hitLayers.strawHits(), [this](const UnCalibHitVec_t& vec){
                return vec.size() > m_cfg.busyLayerLimit;
            }) != m_hitLayers.strawHits().end()) {
            ACTS_VERBOSE("Detected at least one busy layer with more than "<<m_cfg.busyLayerLimit
                <<". Will not attempt the external seed as first seed");
            m_cfg.startWithPattern = false;
        }
        // Set the start for the upper layer
        m_upperLayer = m_hitLayers.strawHits().size()-1; 

        /** Check whether the first layer is too busy */
        while (m_lowerLayer < m_upperLayer && m_hitLayers.strawHits()[m_lowerLayer].size() >m_cfg.busyLayerLimit){
            ACTS_VERBOSE("Lower layer "<<m_lowerLayer<<" has too many hits. Don't use it as seeding layer");
            ++m_lowerLayer;
        }
        /** Check whether the lower layer is too busy */
        while (m_lowerLayer < m_upperLayer && m_hitLayers.strawHits()[m_upperLayer].size() > m_cfg.busyLayerLimit) {
            ACTS_VERBOSE("Upper layer "<<m_upperLayer<<" has too many hits. Don't use it as seeding layer");
            --m_upperLayer;
        }
    }

    template <StationSpacePoint UncalibSp_t, StationSpacePointSorter<UncalibSp_t> Sorter_t>
    void StrawChamberLineSeeder<UncalibSp_t,Sorter_t>::moveToNextCandidate() {
        const UnCalibHitVec_t& lower = m_hitLayers.strawHits()[m_lowerLayer];
        const UnCalibHitVec_t& upper = m_hitLayers.strawHits()[m_upperLayer];
        /// Vary the left-right solutions 
        if (++m_signComboIndex < s_signCombos.size()) {
            return;
        }
        m_signComboIndex = 0; 
            
        /// Move to the next hit in the lower layer
            if (++m_lowerHitIndex < lower.size()) {
            return;
        }
        m_lowerHitIndex=0;
        /// Move to the next hit in the upper layer
        if (++m_upperHitIndex < upper.size()) {
            return;
        }
        m_upperHitIndex = 0;
        /// All combinations of hits & lines in both layers are processed
        /// Switch to the next lowerLayer. But skip the busy ones according to the configuration
        while (m_lowerLayer < m_upperLayer && m_hitLayers.strawHits()[++m_lowerLayer].size() > m_cfg.busyLayerLimit) {
        } 
            
        if (m_lowerLayer < m_upperLayer) {
            return;
        }
        /** Abort the loop if we parsed the multi-layer boundary */
        if (m_lowerLayer >= m_hitLayers.firstLayerFrom2ndMl() && numGenerated()){
            m_lowerLayer = m_upperLayer;
            return;
        }
        m_lowerLayer = 0; 
        while (m_lowerLayer < m_upperLayer && m_hitLayers.strawHits()[--m_upperLayer].size() > m_cfg.busyLayerLimit){
        
        }
    }
     
    template <StationSpacePoint UncalibSp_t, StationSpacePointSorter<UncalibSp_t> Sorter_t>
    std::optional<typename StrawChamberLineSeeder<UncalibSp_t, Sorter_t>::DriftCircleSeed>
        StrawChamberLineSeeder<UncalibSp_t, Sorter_t>::buildSeed(const CalibrationContext& ctx,
                                                                 const UncalibSp_t& topHit, 
                                                                 const UncalibSp_t& bottomHit, 
                                                                 const SignCombo_t& signs) {
        
        // Fetch the signs
        const auto&[signTop, signBot] = signs;
        /// Calculate the relative radius
        double R = signBot *bottomHit->driftRadius() - signTop * topHit->driftRadius(); 
        const Vector3& bottomPos{bottomHit->localPosition()};
        const Vector3& topPos{topHit->localPosition()};
        /// Calculate the distance between the two and their relative angle
        const Vector3 D = topPos - bottomPos;
        const double thetaTubes = std::atan2(D.y(), D.z()); 
        const double distTubes =  std::hypot(D.y(), D.z());
        ACTS_VERBOSE("Try to build new 2 circle seed from bottom Hit: "<<toString(bottomPos)<<", r: "<<bottomHit->driftRadius()
                    <<", top hit: "<<toString(topPos)<<", r: "<<topHit->driftRadius()
                    <<" --> tube distance: "<<toString(D)<<", mag: "<<distTubes<<", theta: "<<thetaTubes);
        
        /// Calculate the seed theta.
        double theta{thetaTubes - std::asin(std::clamp(R / distTubes, -1., 1.))};
                        
        Vector3 seedDir = makeDirectionFromPhiTheta(90.*UnitConstants::degree, theta);
        double y0 = bottomPos.y() * seedDir.z() - bottomPos.z() * seedDir.y() + signBot * bottomHit->driftRadius();

        
        return std::nullopt;
     }
}