// This file is part of the ACTS project.
//
// Copyright (C) 2025 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/MathHelpers.hpp"

namespace Acts{
    namespace LineHelper{
        /** @brief Intersect two straight N-dimensional lines with each other or more generally calculate the point of 
         *         closest approach of the second line to the first line.
         *  @param linePosA: Arbitrary point on the first line
         *  @param lineDirA: Direction of the first line (Unit-length)
         *  @param linePosB: Arbitrary point on the second line
         *  @param lineDirB: Direction of the second line (Unit-length) */
        template <unsigned N>
        inline Intersection<N> lineIntersect(const Acts::Vector<N>& linePosA, const Acts::Vector<N>& lineDirA,
                                             const Acts::Vector<N>& linePosB, const Acts::Vector<N>& lineDirB) {
            
            static_assert(N>=2, "One dimensional intersect not sensible");
            /**  Use the formula
             **    A + lambda dirA  = B + mu dirB
             **    (A-B) + lambda dirA = mu dirB
             **    <A-B, dirB> + lambda <dirA,dirB> = mu
             **     A + lambda dirA = B + (<A-B, dirB> + lambda <dirA,dirB>)dirB
             **     <A-B,dirA> + lambda <dirA, dirA> = <A-B, dirB><dirA,dirB> + lamda<dirA,dirB><dirA,dirB>
             **   -> lambda = -(<A-B, dirA> - <A-B, dirB> * <dirA, dirB>) / (1- <dirA,dirB>^2)
             **   --> mu    =  (<A-B, dirB> - <A-B, dirA> * <dirA, dirB>) / (1- <dirA,dirB>^2) */
            const double dirDots = lineDirA.dot(lineDirB);
            const double divisor = (1. - square(dirDots));
            /// If the two directions are parallel to each other there's no way of intersection
            if (std::abs(divisor) < std::numeric_limits<double>::epsilon()) {
                return Intersection<N>::invalid();
            }
            const ActsVector<N> AminusB = linePosA - linePosB;
            const double pathLength =  (AminusB.dot(lineDirB) - AminusB.dot(lineDirA) * dirDots) / divisor;
            
            return Intersection<N>{linePosB + pathLength * lineDirB, IntersectionStatus::onSurface};
        }
        /** @brief Intersect the lines of two line surfaces using their respective transforms.
         *  @param lineSurfTrf1: local -> global transform of the first surface
         *  @param lineSurfTrf2: local -> global transform of the second surface */
        inline Intersection3D lineIntersect(const Acts::Transform3& lineSurfTrf1, 
                                           const Acts::Transform3& lineSurfTrf2) {
            return lineIntersect<3>(lineSurfTrf1.translation(), lineSurfTrf1.linear().col(2),
                                    lineSurfTrf2.translation(), lineSurfTrf2.linear().col(2));
        }
        /** @brief Intersect a line in 3D space with a plane represented by the Hesse-Normal form
         *  @param linePos: Arbitrary point on the line to intersect
         *  @param lineDir: Direction of the line to intersect
         *  @param planeNorm: Normal vector of the plane
         *  @param offSet: Offset to move the plane along the normal vector */
        inline InterSection3D planeIntersect(const Acts::Vector3& linePos,
                                             const Acts::Vector3& lineDir,
                                             const Acts::Vector3& planeNorm,
                                             const double offset) {
            /** Use the formula: <P, N> - C = 0
             *   --> insert line equation: <A + lambda * B, N> - C = 0
             *   --> lambda = (C - <A,N>)/ <N, B> */
            const double normDot = planeNorm.dot(lineDir); 
            if (std::abs(normDot) < std::numeric_limits<double>::epsilon()) {
                return Intersection3D::invalid();
            }
            const double path = (offset - linePos.dot(planeNorm)) / normDot;
            return Intersection3D{linePos + path * lineDir, path, IntersectionStatus::onSurface};
        }
        /** @brief Intersect a line in 3D space with a plane represented by the Hesse-Normal form
         *  @param linePos: Arbitrary point on the line to intersect
         *  @param lineDir: Direction of the line to intersect
         *  @param planeNorm: Normal vector of the plane
         *  @param planePoint: Point on the plane */
        inline InterSection3D planeIntersect(const Acts::Vector3& linePos,
                                             const Acts::Vector3& lineDir,
                                             const Acts::Vector3& planeNorm,
                                             const Acts::Vector3& planePoint) {
            return planeIntersect(linePos, lineDir, planeNorm, planePoint.dot(planeNorm));
        }
    }
}