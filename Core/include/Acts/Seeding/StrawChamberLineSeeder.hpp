// This file is part of the ACTS project.
//
// Copyright (C) 2025 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#pragma once

#include "Acts/EventData/StationSpacePoint.hpp"

namespace Acts{

    /** @brief Define the concept of the space point measurement sorter. The sorter shall take a collection 
     *         of station space points and sort them first into straw and strip hits. Then each category
     *         needs to be sorted by the logical measurement layers. */
    template <typename MeasurementSorterType,typename spType>
    // 
                    
    concept MeasurementSorter = std::constructible_from<MeasurementSorterType, const std::vector<const spType*>&> &&
        requires(MeasurementSorterType sorter, spType SP){
            { sorter.strawHits()} -> std::same_as<const std::vector<std::vector<const spType*>>& >;
            { sorter.stripHits()} -> std::same_as<const std::vector<std::vector<const spType*>>& >;
    };
    /** @brief Define the  */
     template <StationSpacePoint s, MeasurementSorter<s> m>
    class StrawChamberLineSeeder{
        public:
            

    };

}
#include "Acts/Seeding/StrawChamberLineSeeder.ipp"