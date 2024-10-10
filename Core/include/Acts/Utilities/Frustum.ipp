// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/VectorHelpers.hpp"

template <typename value_t, std::size_t DIM, std::size_t SIDES>
Acts::Frustum<value_t, DIM, SIDES>::Frustum(const VertexType& origin,
                                            const VertexType& dir,
                                            value_type opening_angle)
  requires(DIM == 2)
    : m_origin(origin) {
  using rotation_t = Eigen::Rotation2D<value_type>;

  static_assert(SIDES == 2, "2D frustum can only have 2 sides");
  assert(opening_angle < M_PI);

  translation_t translation(origin);
  value_type angle = VectorHelpers::phi(dir);
  Eigen::Rotation2D<value_type> rot(angle);

  value_type normal_angle = 0.5 * M_PI - 0.5 * opening_angle;
  VertexType normal1 = rotation_t(normal_angle) * VertexType::UnitX();
  VertexType normal2 = rotation_t(-normal_angle) * VertexType::UnitX();

  m_normals = {rot * VertexType::UnitX(), rot * normal1, rot * normal2};
}

template <typename value_t, std::size_t DIM, std::size_t SIDES>
Acts::Frustum<value_t, DIM, SIDES>::Frustum(const VertexType& origin,
                                            const VertexType& dir,
                                            value_type opening_angle)
  requires(DIM == 3)
    : m_origin(origin) {
  static_assert(SIDES > 2, "3D frustum must have 3 or more sides");
  assert(opening_angle < M_PI);
  using angle_axis_t = Eigen::AngleAxis<value_type>;

  const VertexType ldir = VertexType::UnitZ();
  const VertexType lup = VertexType::UnitX();

  transform_type transform;
  transform = (Eigen::Quaternion<value_type>().setFromTwoVectors(ldir, dir));

  m_normals[0] = ldir;

  const value_type phi_sep = 2 * M_PI / sides;
  transform_type rot;
  rot = angle_axis_t(phi_sep, ldir);

  value_type half_opening_angle = opening_angle / 2.;
  auto calculate_normal =
      [&ldir, &half_opening_angle](const VertexType& out) -> VertexType {
    const VertexType tilt_axis = -1 * out.cross(ldir);
    return (-1 * (angle_axis_t(half_opening_angle, tilt_axis) * out))
        .normalized();
  };

  VertexType current_outward = lup;
  m_normals[1] = calculate_normal(current_outward);

  for (std::size_t i = 1; i < sides; i++) {
    current_outward = rot * current_outward;
    m_normals[i + 1] = calculate_normal(current_outward);
  }

  for (auto& normal : m_normals) {
    normal = transform * normal;
  }
}

template <typename value_t, std::size_t DIM, std::size_t SIDES>
void Acts::Frustum<value_t, DIM, SIDES>::draw(IVisualization3D& helper,
                                              value_type far_distance) const
  requires(DIM == 3)
{
  static_assert(DIM == 3, "Drawing is only supported in 3D");

  // Iterate around normals, calculate cross with "far" plane
  // to get intersection lines.
  // Work in local reference frame of the frustum, and only convert to global
  // right before drawing.
  VertexType far_normal = m_normals[0];  // far has same normal as pseudo-near
  VertexType far_center = m_normals[0] * far_distance;
  std::array<std::pair<VertexType, VertexType>, SIDES> planeFarIXs;

  auto ixPlanePlane = [](const auto& n1, const auto& p1, const auto& n2,
                         const auto& p2) -> std::pair<VertexType, VertexType> {
    const VertexType m = n1.cross(n2).normalized();
    const double j = (n2.dot(p2 - p1)) / (n2.dot(n1.cross(m)));
    const VertexType q = p1 + j * n1.cross(m);
    return {m, q};
  };

  auto ixLineLine = [](const auto& p1, const auto& d1, const auto& p2,
                       const auto& d2) -> VertexType {
    return p1 + (((p2 - p1).cross(d2)).norm() / (d1.cross(d2)).norm()) * d1;
  };

  // skip i=0 <=> pseudo-near
  for (std::size_t i = 1; i < n_normals; i++) {
    const auto ixLine =
        ixPlanePlane(far_normal, far_center, m_normals[i], VertexType::Zero());
    planeFarIXs.at(i - 1) = ixLine;
  }

  std::array<VertexType, SIDES> points;

  for (std::size_t i = 0; i < std::size(planeFarIXs); i++) {
    std::size_t j = (i + 1) % std::size(planeFarIXs);
    const auto& l1 = planeFarIXs.at(i);
    const auto& l2 = planeFarIXs.at(j);
    const VertexType ix =
        m_origin + ixLineLine(l1.second, l1.first, l2.second, l2.first);
    points.at(i) = ix;
  }

  for (std::size_t i = 0; i < std::size(points); i++) {
    std::size_t j = (i + 1) % std::size(points);
    helper.face(
        std::vector<VertexType>({m_origin, points.at(i), points.at(j)}));
  }
}

template <typename value_t, std::size_t DIM, std::size_t SIDES>
std::ostream& Acts::Frustum<value_t, DIM, SIDES>::svg(std::ostream& os,
                                                      value_type w,
                                                      value_type h,
                                                      value_type far_distance,
                                                      value_type unit) const
  requires(DIM == 2)
{
  static_assert(DIM == 2, "SVG is only supported in 2D");

  VertexType mid(w / 2., h / 2.);

  // set up transform for svg. +y is down, normally, and unit is pixels.
  // We flip the y axis, and scale up by `unit`.
  transform_type trf = transform_type::Identity();
  trf.translate(mid);
  trf = trf * Eigen::Scaling(VertexType(1, -1));
  trf.scale(unit);

  std::array<std::string, 3> colors({"orange", "blue", "red"});

  auto draw_line = [&](const VertexType& left_, const VertexType& right_,
                       const std::string& color, std::size_t width) {
    VertexType left = trf * left_;
    VertexType right = trf * right_;
    os << "<line ";

    os << "x1=\"" << left.x() << "\" ";
    os << "y1=\"" << left.y() << "\" ";
    os << "x2=\"" << right.x() << "\" ";
    os << "y2=\"" << right.y() << "\" ";

    os << " stroke=\"" << color << "\" stroke-width=\"" << width << "\"/>\n";
  };

  auto draw_point = [&](const VertexType& p_, const std::string& color,
                        std::size_t r) {
    VertexType p = trf * p_;
    os << "<circle ";
    os << "cx=\"" << p.x() << "\" cy=\"" << p.y() << "\" r=\"" << r << "\"";
    os << " fill=\"" << color << "\"";
    os << "/>\n";
  };

  using vec3 = Eigen::Matrix<value_type, 3, 1>;
  auto ixLineLine = [](const VertexType& p1_2, const VertexType& d1_2,
                       const VertexType& p2_2,
                       const VertexType& d2_2) -> VertexType {
    const vec3 p1(p1_2.x(), p1_2.y(), 0);
    const vec3 p2(p2_2.x(), p2_2.y(), 0);
    const vec3 d1(d1_2.x(), d1_2.y(), 0);
    const vec3 d2(d2_2.x(), d2_2.y(), 0);

    vec3 num = (p2 - p1).cross(d2);
    vec3 den = d1.cross(d2);

    value_type f = 1.;
    value_type dot = num.normalized().dot(den.normalized());
    if (std::abs(dot) - 1 < 1e-9 && dot < 0) {
      f = -1.;
    }

    const vec3 p = p1 + f * (num.norm() / den.norm()) * d1;
    assert(std::abs(p.z()) < 1e-9);
    return {p.x(), p.y()};
  };

  const VertexType far_dir = {m_normals[0].y(), -m_normals[0].x()};
  const VertexType far_point = m_normals[0] * far_distance;

  std::array<VertexType, 2> points;

  for (std::size_t i = 1; i < n_normals; i++) {
    VertexType plane_dir(m_normals[i].y(), -m_normals[i].x());

    const VertexType ix = ixLineLine(far_point, far_dir, {0, 0}, plane_dir);
    draw_point(m_origin + ix, "black", 3);
    draw_line(m_origin, m_origin + ix, "black", 2);
    points.at(i - 1) = ix;
  }

  draw_line(m_origin + points.at(0), m_origin + points.at(1), "black", 2);

  draw_line(m_origin, m_origin + m_normals[0] * 2, colors[0], 3);

  draw_point({0, 0}, "yellow", 5);
  draw_point(m_origin, "green", 5);

  return os;
}

template <typename value_t, std::size_t DIM, std::size_t SIDES>
Acts::Frustum<value_t, DIM, SIDES>
Acts::Frustum<value_t, DIM, SIDES>::transformed(
    const transform_type& trf) const {
  const auto& rot = trf.rotation();

  std::array<VertexType, n_normals> new_normals;
  for (std::size_t i = 0; i < n_normals; i++) {
    new_normals[i] = rot * m_normals[i];
  }

  return Frustum<value_t, DIM, SIDES>(trf * m_origin, std::move(new_normals));
}
