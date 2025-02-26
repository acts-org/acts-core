// This file is part of the ACTS project.
//
// Copyright (C) 2025 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#pragma once

#include "Acts/EventData/StationSpacePoint.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Surfaces/detail/LineHelper.hpp"


#include <array>
#include <optional>
namespace Acts{

    /** @brief Define the concept of the space point measurement sorter. The sorter shall take a collection 
     *         of station space points and sort them first into straw and strip hits. Then each category
     *         needs to be sorted by the logical measurement layers. */
    template <typename MeasurementSorterType,typename SpType>
    concept StationSpacePointSorter = std::constructible_from<MeasurementSorterType, const std::vector<const SpType*>&> &&
        requires(MeasurementSorterType sorter, SpType SP){
            /** @brief Return the straw-hit space point sorted by straw layer */
            { sorter.strawHits()} -> std::same_as<const std::vector<std::vector<const SpType*>>& >;
            /** @brief Return the strip-hit  space points sorted by detector layer */
            { sorter.stripHits()} -> std::same_as<const std::vector<std::vector<const SpType*>>& >;
    };

    /** @brief Define the  */
 ///   template <typename CalibratorType, typename CalibSpType, typename UnCalibSpType>
 ///   concept SpacePointCalibrator = requires(CalibratorType calibrator, CalibSpType CalibSp_t, UnCalibSpType unCalibSp){
 ///       /** @brief Calibration of the space point  */
 ///       { calibrator.calibrate(const CalibrationContext& ctx,
 ///                              const Acts::Vector3& seedPos,
 ///                              const Acts::Vector3& seedDir,
 ///                              const double t0,
 ///                              const UnCalibSpType& uncalibSp)} -> std::same_as<std::unique_ptr<UnCalibSpType>;
 ///   };
    
    template <StationSpacePoint UncalibSp_t, StationSpacePointSorter<UncalibSp_t> Sorter_t>
    class StrawChamberLineSeeder{
        public:
            /** @brief Abbreviation of the uncalibrated hit vectors */
            using UnCalibHitVec_t = std::vector<const UncalibSp_t*>;
            struct Config{
                /** @brief Cut on the theta angle */
                std::array<double, 2> thetaRange{0, 180.*UnitConstants::degree};
                /** @brief Cut on the intercept range */
                std::array<double, 2> interceptRange{-20.*UnitConstants::m, 20.*UnitConstants::m};
                /** @brief Upper cut on the hit chi2 w.r.t. seed in order to be associated to the seed*/
                double hitPullCut{5.};
                /** @brief Try at the first time the pattern seed as candidate */
                bool startWithPattern{false};
                /** @brief How many drift circles may be on a layer to be used for seeding */
                unsigned int busyLayerLimit{2};
                /** @brief How many drift circle hits needs the seed to contain in order to be valid */
                unsigned int nMdtHitCut{3};
                /** @brief Hit cut based on the fraction of collected tube layers. 
                 *         The seed must pass the tighter of the two requirements.  */
                double nMdtLayHitCut{2./3.};
                /** @brief Once a seed with even more than initially required hits is found,
                 *         reject all following seeds with less hits */
                bool tightenHitCut{true};
                /** @brief Check whether a new seed candidate shares the same left-right solution with already accepted ones
                 *         Reject the seed if it has the same amount of hits */
                bool overlapCorridor{true};
                /** @brief Recalibrate the seed drift circles from the initial estimate  */
                bool recalibSeedCircles{false};
                /** @brief Pointer to the space point calibrator */
                // const ISpacePointCalibrator* calibrator{nullptr};
                /** @brief Toggle whether the seed is rapidly refitted */
                bool fastSeedFit{true};
                /** @brief Toggle whether an initial t0 fit shall be executed */
                bool fastSegFitWithT0{false};
                /** @brief Maximum number of iterations in the fast segment fit */
                unsigned int nMaxIter{100};
                /** @brief Precision cut off in the fast segment fit */
                double precCutOff{1.e-6};
            };


            StrawChamberLineSeeder(const UnCalibHitVec_t& seedHits,
                                   Config&& cfg,
                                   std::unique_ptr<const Acts::Logger> logObj);

            /** @brief Remove the copy constructor */
            StrawChamberLineSeeder(const StrawChamberLineSeeder& other) = delete;
            /** @brief Remove the copy assignment */
            StrawChamberLineSeeder& operator=(const StrawChamberLineSeeder& other) = delete;

            /** @brief Returns the number of already generated seeds */
            unsigned int numGenerated() const {
                return m_nGenSeeds;
            }

            /** @brief Seed object returned by the seeder. The seed contains the initial parameter estimate w.r.t
             *         to the central plane surface inside the chamber. *Note* the parameter q/p is set to zero and 
             *         does not serve any purpose here.
             *          
             *         Further the seed contains the list of associated measurements. If the seeder is run in the 
             *         fast 2D fit mode, the number of iterations w.r.t. this fit procedure is appended as well. */
            struct DriftCircleSeed {
                /** @param Initial straight line seed parameters  */
                ActsVector<5> parameters{ActsVector<5>::Zero()};
                /** @brief  */
                std::vector<std::unique_ptr<UncalibSp_t>> measurements{};
                /** @brief Iterations to obtain the seed */
                unsigned int nIter{0};
                /** @brief Seed chi2 */
                double chi2{0.};
                // /** @brief Pointer to the parent bucket */
                // const SpacePointBucket* parentBucket{nullptr};
                /** @brief number of Mdt hits on the seed */
                unsigned int nMdt{0};
            };
        private:
            Sorter_t m_hitLayers;
            Config m_cfg{};
            std::unique_ptr<const Acts::Logger> m_logger{};
            /** @brief Sign combinations to draw the 4 lines tangent to 2 drift circles s*/
            using SignCombo_t = std::array<int, 2>;
            constexpr static std::array<SignCombo_t, 4> s_signCombos{std::array{ 1, 1}, std::array{ 1,-1}, 
                                                                     std::array{-1,-1}, std::array{-1, 1}};
            /** @brief Cache of all solutions seen thus far */
            struct SeedSolution{
                /** @brief: Theta of the line */
                double theta{0.};
                /** @brief Intersecpt of the line */
                double Y0{0.};
                /** @brief: Uncertainty on the slope*/
                double dTheta{0.};
                /** @brief: Uncertainty on the intercept */
                double dY0{0.};
                /** @brief Used hits in the seed */
                UnCalibHitVec_t seedHits{};
                /** @brief Vector of radial signs of the valid hits */
                std::vector<int> solutionSigns{};
                /** @brief Stringstream output operator */
                friend std::ostream& operator<<(std::ostream& ostr, const SeedSolution& sol) {
                    return sol.print(ostr);
                }
                std::ostream& print(std::ostream& ostr) const;
            };

            /** @brief Return the reference to the logger */
            const Logger& logger() const { return *m_logger; }

            /** @brief Prepares the generator to generate the seed from the next pair of drift circles */
            void moveToNextCandidate();

            /** @brief Tries to build the seed from the two hits. Fails if the solution is invalid
             *         or if the seed has already been built before
             *  @param topHit: Hit candidate from the upper layer
             *  @param bottomHit: Hit candidate from the lower layer
             *  @param sign: Object encoding whether the tangent is left / right  */
            std::optional<DriftCircleSeed> buildSeed(const CalibrationContext& ctx,
                                                     const UncalibSp_t& topHit, 
                                                     const UncalibSp_t& bottomHit, 
                                                     const SignCombo_t& signs); 

            /** @brief Auxillary struct to calculate fit constants */
            struct SeedFitAuxilliaries {
                /** @brief Tube position center weigthed with inverse covariances */
                Vector3 centerOfGrav{Vector3::Zero()};
                /** @brief Vector of inverse covariances */
                std::vector<double> invCovs{};
                /** @brief Vector of drfit signs */
                std::vector<int> driftSigns{};
                /** @brief Covariance norm */
                double covNorm{0.};
                /** @brief Expectation value of T_{z}^{2} - T_{y}^{2} */
                double T_zzyy{0.};
                /** @brief Expectation value of T_{y} * T_{z} */
                double T_yz{0.}; 
                /** @brief Expectation value of T_{z} * r  */
                double T_rz{0.};
                /** @brief Expectation value of T_{y} * r  */
                double T_ry{0.};
                /** @brief Prediced y0 given as the expection value of the radii
                 *         divided by the inverse covariance sum. */
                double fitY0{0.};
            };

            struct SeedFitAuxWithT0: public SeedFitAuxilliaries {
                /** @brief Constructor */
                SeedFitAuxWithT0(SeedFitAuxilliaries&& parent):
                    SeedFitAuxilliaries{std::move(parent)}{}
                    /** @brief Expectation value of T_{y} * v */
                    double T_vy{0.};
                    /** @brief Expectation value of T_{z} * v */
                    double T_vz{0.};
                    /** @brief Expectation value of T_{y} * a */
                    double T_ay{0.};
                    /** @brief Expectation value of T_{z} * a */
                    double T_az{0.};
                    /** @brief Expectation value of r * v */
                    double R_vr{0.};
                    /** @brief Expectation value of v * v */
                    double R_vv{0.};
                    /** @brief Expectation value of r * a */
                    double R_va{0.};
                    /** @brief First derivative of the fitted Y0 */
                    double fitY0Prime{0.};
                    /** @brief Second derivative of the ftted Y0 */
                    double fitY0TwoPrime{0.};
            };
            /** @brief Considered layer to pick the top drift circle from*/
            std::size_t m_upperLayer{0};
            /** @brief Considered layer to pick the bottom drift circle from*/
            std::size_t m_lowerLayer{0}; 
            /** @brief Explicit hit to pick in the selected bottom layer */
            std::size_t m_lowerHitIndex{0};
            /** @brief Explicit hit to pick in the selected top layer */
            std::size_t m_upperHitIndex{0};
            /** @brief Index of the left-right ambiguity between the circles */
            std::size_t m_signComboIndex{0};
            /** @brief Vector caching equivalent solutions to avoid double seeding */
            std::vector<SeedSolution> m_seenSolutions{};
            /** Counter on how many seeds have been generated */
            unsigned int m_nGenSeeds{0};
    };

}
#include "Acts/Seeding/StrawChamberLineSeeder.ipp"