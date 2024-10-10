// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimVertex.hpp"

#include <HepMC3/FourVector.h>
#include <HepMC3/GenEvent.h>
#include <HepMC3/GenParticle.h>
#include <HepMC3/GenVertex.h>

namespace ActsExamples::HepMC3Event {

///
/// Setter
///

/// @brief Sets new units for momentums
/// @note The allowed units are MeV and Gev
/// @param event event in HepMC data type
/// @param momentumUnit new unit of momentum
void momentumUnit(HepMC3::GenEvent& event, const double momentumUnit);

/// @brief Sets new units for lengths
/// @note The allowed units are mm and cm
/// @param event event in HepMC data type
/// @param lengthUnit new unit of length
void lengthUnit(HepMC3::GenEvent& event, const double lengthUnit);

/// @brief Shifts the positioning of an event in space and time
/// @param event event in HepMC data type
/// @param deltaPos relative spatial shift that will be applied
/// @param deltaTime relative time shift that will be applied
void shiftPositionBy(HepMC3::GenEvent& event, const Acts::Vector3& deltaPos,
                     const double deltaTime);

/// @brief Shifts the positioning of an event to a paint in space and time
/// @param event event in HepMC data type
/// @param pos new position of the event
/// @param time new time of the event
void shiftPositionTo(HepMC3::GenEvent& event, const Acts::Vector3& pos,
                     const double time);

/// @brief Shifts the positioning of an event to a paint in space
/// @param event event in HepMC data type
/// @param pos new position of the event
void shiftPositionTo(HepMC3::GenEvent& event, const Acts::Vector3& pos);

/// @brief Shifts the positioning of an event to a paint in time
/// @param event event in HepMC data type
/// @param time new time of the event
void shiftPositionTo(HepMC3::GenEvent& event, const double time);

///
/// Adder
///

/// @brief Adds a new particle
/// @param event event in HepMC data type
/// @param particle new particle that will be added
void addParticle(HepMC3::GenEvent& event,
                 const std::shared_ptr<SimParticle>& particle);

/// @brief Adds a new vertex
/// @param event event in HepMC data type
/// @param vertex new vertex that will be added
/// @note The statuses are not represented in Acts and therefore set to 0
void addVertex(HepMC3::GenEvent& event,
               const std::shared_ptr<SimVertex>& vertex);
///
/// Remover
///

/// @brief Removes a particle from the record
/// @param event event in HepMC data type
/// @param particle particle that will be removed
void removeParticle(HepMC3::GenEvent& event,
                    const std::shared_ptr<SimParticle>& particle);

/// @brief Removes a vertex from the record
/// @note The identification of the vertex is potentially unstable (c.f.
/// HepMC3Event::compareVertices())
/// @param event event in HepMC data type
/// @param vertex vertex that will be removed
void removeVertex(HepMC3::GenEvent& event,
                  const std::shared_ptr<SimVertex>& vertex);

///
/// Getter
///

/// @brief Getter of the unit of momentum used
/// @param event event in HepMC data type
/// @return unit of momentum
double momentumUnit(const HepMC3::GenEvent& event);

/// @brief Getter of the unit of length used
/// @param event event in HepMC data type
/// @return unit of length
double lengthUnit(const HepMC3::GenEvent& event);

/// @brief Getter of the position of the event
/// @param event event in HepMC data type
/// @return vector to the location of the event
Acts::Vector3 eventPos(const HepMC3::GenEvent& event);

/// @brief Getter of the time of the event
/// @param event event in HepMC data type
/// @return time of the event
double eventTime(const HepMC3::GenEvent& event);

/// @brief Get list of particles
/// @param event event in HepMC data type
/// @return List of particles
std::vector<SimParticle> particles(const HepMC3::GenEvent& event);

/// @brief Get list of vertices
/// @param event event in HepMC data type
/// @return List of vertices
std::vector<std::unique_ptr<SimVertex>> vertices(const HepMC3::GenEvent& event);

/// @brief Get beam particles
/// @param event event in HepMC data type
/// @return List of beam particles
std::vector<SimParticle> beams(const HepMC3::GenEvent& event);

/// @brief Get final state particles
/// @param event event in HepMC data type
/// @return List of final state particles
std::vector<SimParticle> finalState(const HepMC3::GenEvent& event);

}  // namespace ActsExamples::HepMC3Event
