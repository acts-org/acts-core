
#pragma once

#include "Acts/EventData/StationSpacePoint.hpp"

namespace Acts{
        class MuonSpacePoint {
            public:
                /** @brief Return the local meaurement position */
                const Acts::Vector3& localPosition() const{
                    return m_pos;
                }
                /** @brief Return the local sensor direction */
                const Acts::Vector3& sensorDirection() const{
                   return m_dir;
                }
                /** @brief Return the normal vector to the plane */
                const Acts::Vector3& stripPlaneNormal() const {
                    return m_pos;
                }
                /** @brief Return the drift radius */
                double driftRadius() const{
                    return m_radius;
                }
                /** @brief Return the measurement time */
                double time() const {
                    return m_time;
                }
                /** @brief Define the space point coordinates.
                 *  @param pos: Space point position
                 *  @param sensorDir: Direction of the sensor */
                void defineCoordinates(Acts::Vector3&& pos,
                                       Acts::Vector3&& sensorDir){
                    m_pos = std::move(pos);
                    m_dir = std::move(sensorDir);
                }
                /** @brief Define the space point normal*/
                void defineNormal(Acts::Vector3&& norm) {
                    m_norm = std::move(norm);
                }
                /** @brief Define the space point radius */
                void setRadius(const double r) {
                    m_radius = r;
                }
                /** @brief Define the time of the space point measurement */
                void setTime(const double t){
                    m_time = t;
                }
            private:
                Acts::Vector3 m_pos{Acts::Vector3::Zero()};
                Acts::Vector3 m_dir{Acts::Vector3::Zero()};
                Acts::Vector3 m_norm{Acts::Vector3::Zero()};
                double m_radius{0.};
                double m_time{0.};

        };

}