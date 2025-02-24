// This file is part of the ACTS project.
//
// Copyright (C) 2025 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#pragma once

#include  "Acts/Definitions/Algebra.hpp"

#include <type_traits>


namespace Acts{
    /** @brief Concept definition of the station space points. They're primarly used in composite detectors,
     *         like the Muon chambers in side the ATLAS experiment. The chambers usually consist of few layers
     *         of drift tubes which maybe sandwiched by other strip detector layers to measure the local coordinates
     *         on the reference plane the particle's passage.
     * 
     *  To describe the 
     */
    template <typename SpacePointType>
        concept StationSpacePoint = requires(SpacePointType sp) {
            /** @brief Local position of the space point measurement. It'either
             *         the position of the wire or the position of the fired strip in the chamber */
            { sp.localPosition() } -> std::same_as<const Acts::Vector3&>;
            /** @brief Orientation of the sensor, which is either the wire orientation or 
             *         the strip orientation. Travelling along the direction does not alter the residual */
            { sp.sensorDirection()} -> std::same_as<const Acts::Vector3&>;
            /** @brief Normal vector on the strip-plane. */
            { sp.stripPlaneNormal()} -> std::same_as<const Acts::Vector3&>;
            /** @brief Radius of the  tube drift-measurement. The returned value is zero for strip measurements */
            { sp.driftRadius() } -> std::same_as<double>;
            /** @brief Time when the measurement was taken */
            { sp.time()} -> std::same_as<double>;
        };

        template<StationSpacePoint> 
            class MeasurementSorter{
                public:
                    MeasurementSorter() = default;
        };

}