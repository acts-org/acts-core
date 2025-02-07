// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

template <typename entity_t, typename value_t, std::size_t DIM>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::AxisAlignedBoundingBox(
    const entity_t* entity, const VertexType& vmin, const VertexType& vmax)
    : m_entity(entity),
      m_vmin(vmin),
      m_vmax(vmax),
      m_center((vmin + vmax) / 2.),
      m_width(vmax - vmin),
      m_iwidth(1 / m_width) {}

template <typename entity_t, typename value_t, std::size_t DIM>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::AxisAlignedBoundingBox(
    const entity_t* entity, const VertexType& center, const Size& size)
    : m_entity(entity),
      m_vmin(center - size.get() * 0.5),
      m_vmax(center + size.get() * 0.5),
      m_center(center),
      m_width(size.get()),
      m_iwidth(1 / m_width) {}

template <typename entity_t, typename value_t, std::size_t DIM>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::AxisAlignedBoundingBox(
    const std::vector<self_t*>& boxes, vertex_array_type envelope)
    : m_entity(nullptr) {
  assert(boxes.size() > 1);

  for (std::size_t i = 0; i < boxes.size(); i++) {
    if (i < boxes.size() - 1) {
      // set next on i to i+1
      boxes[i]->setSkip(boxes[i + 1]);
    } else {
      // make sure last is set to nullptr, this marks end
      // boxes[i]->m_next = nullptr;
      boxes[i]->setSkip(nullptr);
    }
  }

  m_left_child = boxes.front();
  m_right_child = boxes.back();
  m_skip = nullptr;

  std::tie(m_vmin, m_vmax) = wrap(boxes, envelope);

  m_center = (m_vmin + m_vmax) / 2.;
  m_width = m_vmax - m_vmin;
  m_iwidth = 1 / m_width;
}

template <typename entity_t, typename value_t, std::size_t DIM>
std::pair<
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType,
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::wrap(
    const std::vector<const self_t*>& boxes, vertex_array_type envelope) {
  assert(boxes.size() > 1);
  // figure out extent of boxes
  // use array for Eigen coefficient wise min/max
  vertex_array_type vmax(
      vertex_array_type::Constant(std::numeric_limits<value_type>::lowest()));
  vertex_array_type vmin(
      vertex_array_type::Constant(std::numeric_limits<value_type>::max()));

  for (std::size_t i = 0; i < boxes.size(); i++) {
    vmin = vmin.min(boxes[i]->min().array());
    vmax = vmax.max(boxes[i]->max().array());
  }

  vmax += envelope;
  vmin -= envelope;

  return {vmin, vmax};
}

template <typename entity_t, typename value_t, std::size_t DIM>
std::pair<
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType,
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::wrap(
    const std::vector<self_t*>& boxes, vertex_array_type envelope) {
  assert(boxes.size() > 1);
  std::vector<const self_t*> box_ptrs;
  box_ptrs.reserve(boxes.size());
  std::transform(boxes.begin(), boxes.end(), std::back_inserter(box_ptrs),
                 [](const auto* box) { return box; });
  return wrap(box_ptrs, envelope);
}

template <typename entity_t, typename value_t, std::size_t DIM>
std::pair<
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType,
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::wrap(
    const std::vector<self_t>& boxes, vertex_array_type envelope) {
  assert(boxes.size() > 1);
  std::vector<const self_t*> box_ptrs;
  box_ptrs.reserve(boxes.size());
  std::transform(boxes.begin(), boxes.end(), std::back_inserter(box_ptrs),
                 [](auto& box) { return &box; });
  return wrap(box_ptrs, envelope);
}

template <typename entity_t, typename value_t, std::size_t DIM>
bool Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::intersect(
    const VertexType& point) const {
  vertex_array_type t = (point - m_vmin).array() * m_iwidth;
  return t.minCoeff() >= 0 && t.maxCoeff() < 1;
}

template <typename entity_t, typename value_t, std::size_t DIM>
bool Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::intersect(
    const Ray<value_type, DIM>& ray) const {
  const VertexType& origin = ray.origin();
  const vertex_array_type& idir = ray.idir();

  // Calculate the intersect distances with the min and max planes along the ray
  // direction, from the ray origin. See Ch VII.5 Fig.1 in [1].
  // This is done in all dimensions at the same time:
  vertex_array_type t0s = (m_vmin - origin).array() * idir;
  vertex_array_type t1s = (m_vmax - origin).array() * idir;

  // Calculate the component wise min/max between the t0s and t1s
  // this is non-compliant with IEEE-754-2008, NaN gets propagated through
  // https://eigen.tuxfamily.org/bz/show_bug.cgi?id=564
  // this means that rays parallel to boundaries might not be considered
  // to intersect.
  vertex_array_type tsmaller = t0s.min(t1s);
  vertex_array_type tbigger = t0s.max(t1s);

  // extract largest and smallest component of the component wise extrema
  value_type tmin = tsmaller.maxCoeff();
  value_type tmax = tbigger.minCoeff();

  // If tmin is smaller than tmax and tmax is positive, then the box is in
  // positive ray direction, and the ray intersects the box.
  return tmin < tmax && tmax > 0.0;
}

template <typename entity_t, typename value_t, std::size_t DIM>
template <std::size_t sides>
bool Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::intersect(
    const Frustum<value_type, DIM, sides>& fr) const {
  const auto& normals = fr.normals();
  // Transform vmin and vmax into the coordinate system, at which the frustum is
  // located at the coordinate origin.
  const vertex_array_type fr_vmin = m_vmin - fr.origin();
  const vertex_array_type fr_vmax = m_vmax - fr.origin();

  // For each plane, find the p-vertex, which is the vertex that is at the
  // furthest distance from the plane *along* its normal direction.
  // See Fig. 2 in [2].
  VertexType p_vtx;
  // for loop, we could eliminate this, probably,
  // but sides+1 is known at compile time, so the compiler
  // will most likely unroll the loop
  for (std::size_t i = 0; i < sides + 1; i++) {
    const VertexType& normal = normals[i];

    // for AABBs, take the component from the min vertex, if the normal
    // component is negative, else take the component from the max vertex.
    p_vtx = (normal.array() < 0).template cast<value_type>() * fr_vmin +
            (normal.array() >= 0).template cast<value_type>() * fr_vmax;

    // Check if the p-vertex is at positive or negative direction along the
    // If the p vertex is along negative normal direction *once*, the box is
    // outside the frustum, and we can terminate early.
    if (p_vtx.dot(normal) < 0) {
      // p vertex is outside on this plane, box must be outside
      return false;
    }
  }

  // If we get here, no p-vertex was outside, so box intersects or is
  // contained. We don't care, so report 'intersect'
  return true;
}

template <typename entity_t, typename value_t, std::size_t DIM>
void Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::setSkip(
    self_t* skip) {
  // set next on this
  m_skip = skip;
  // find last child and set its skip
  if (m_right_child != nullptr) {
    m_right_child->setSkip(skip);
  }
}

template <typename entity_t, typename value_t, std::size_t DIM>
const Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>*
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::getLeftChild() const {
  return m_left_child;
}

template <typename entity_t, typename value_t, std::size_t DIM>
const Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>*
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::getSkip() const {
  return m_skip;
}

template <typename entity_t, typename value_t, std::size_t DIM>
bool Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::hasEntity() const {
  return m_entity != nullptr;
}

template <typename entity_t, typename value_t, std::size_t DIM>
const entity_t* Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::entity()
    const {
  return m_entity;
}

template <typename entity_t, typename value_t, std::size_t DIM>
void Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::setEntity(
    const entity_t* entity) {
  m_entity = entity;
}

template <typename entity_t, typename value_t, std::size_t DIM>
const typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType&
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::center() const {
  return m_center;
}

template <typename entity_t, typename value_t, std::size_t DIM>
const typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType&
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::min() const {
  return m_vmin;
}

template <typename entity_t, typename value_t, std::size_t DIM>
const typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType&
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::max() const {
  return m_vmax;
}

template <typename entity_t, typename value_t, std::size_t DIM>
std::ostream& Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::toStream(
    std::ostream& os) const {
  os << "AABB(ctr=(";

  for (std::size_t i = 0; i < DIM; i++) {
    if (i > 0) {
      os << ", ";
    }
    os << m_center[i];
  }

  os << ") vmin=(";
  for (std::size_t i = 0; i < DIM; i++) {
    if (i > 0) {
      os << ", ";
    }
    os << m_vmin[i];
  }

  os << ") vmax=(";

  for (std::size_t i = 0; i < DIM; i++) {
    if (i > 0) {
      os << ", ";
    }
    os << m_vmax[i];
  }

  os << "))";

  return os;
}

template <typename entity_t, typename value_t, std::size_t DIM>
std::pair<
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType,
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::transformVertices(
    const transform_type& trf) const
  requires(DIM == 3)
{
  // we need to enumerate all the vertices, transform,
  // and then recalculate min and max

  std::array<VertexType, 8> vertices({{
      {m_vmin.x(), m_vmin.y(), m_vmin.z()},
      {m_vmin.x(), m_vmax.y(), m_vmin.z()},
      {m_vmax.x(), m_vmax.y(), m_vmin.z()},
      {m_vmax.x(), m_vmin.y(), m_vmin.z()},
      {m_vmin.x(), m_vmin.y(), m_vmax.z()},
      {m_vmin.x(), m_vmax.y(), m_vmax.z()},
      {m_vmax.x(), m_vmax.y(), m_vmax.z()},
      {m_vmax.x(), m_vmin.y(), m_vmax.z()},
  }});

  VertexType vmin = trf * vertices[0];
  VertexType vmax = trf * vertices[0];

  for (std::size_t i = 1; i < 8; i++) {
    const VertexType vtx = trf * vertices[i];
    vmin = vmin.cwiseMin(vtx);
    vmax = vmax.cwiseMax(vtx);
  }

  return {vmin, vmax};
}

template <typename entity_t, typename value_t, std::size_t DIM>
std::pair<
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType,
    typename Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::VertexType>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::transformVertices(
    const transform_type& trf) const
  requires(DIM == 2)
{
  // we need to enumerate all the vertices, transform,
  // and then recalculate min and max

  std::array<VertexType, 4> vertices({{{m_vmin.x(), m_vmin.y()},
                                       {m_vmin.x(), m_vmax.y()},
                                       {m_vmax.x(), m_vmax.y()},
                                       {m_vmax.x(), m_vmin.y()}}});

  VertexType vmin = trf * vertices[0];
  VertexType vmax = trf * vertices[0];

  for (std::size_t i = 1; i < 4; i++) {
    const VertexType vtx = trf * vertices[i];
    vmin = vmin.cwiseMin(vtx);
    vmax = vmax.cwiseMax(vtx);
  }

  return {vmin, vmax};
}

template <typename entity_t, typename value_t, std::size_t DIM>
void Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::transform(
    const transform_type& trf) {
  std::tie(m_vmin, m_vmax) = transformVertices(trf);
}

template <typename entity_t, typename value_t, std::size_t DIM>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>
Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::transformed(
    const transform_type& trf) const {
  const auto [vmin, vmax] = transformVertices(trf);
  return self_t(m_entity, vmin, vmax);
}

template <typename entity_t, typename value_t, std::size_t DIM>
void Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::draw(
    IVisualization3D& helper, Color color, const transform_type& trf) const
  requires(DIM == 3)
{
  static_assert(DIM == 3, "PLY output only supported in 3D");

  const VertexType& vmin = m_vmin;
  const VertexType& vmax = m_vmax;

  auto write = [&](const VertexType& a, const VertexType& b,
                   const VertexType& c, const VertexType& d) {
    helper.face(std::vector<VertexType>({trf * a, trf * b, trf * c, trf * d}),
                color);
  };

  write({vmin.x(), vmin.y(), vmin.z()}, {vmin.x(), vmax.y(), vmin.z()},
        {vmin.x(), vmax.y(), vmax.z()}, {vmin.x(), vmin.y(), vmax.z()});

  write({vmax.x(), vmin.y(), vmin.z()}, {vmax.x(), vmax.y(), vmin.z()},
        {vmax.x(), vmax.y(), vmax.z()}, {vmax.x(), vmin.y(), vmax.z()});

  write({vmin.x(), vmin.y(), vmin.z()}, {vmax.x(), vmin.y(), vmin.z()},
        {vmax.x(), vmin.y(), vmax.z()}, {vmin.x(), vmin.y(), vmax.z()});

  write({vmin.x(), vmax.y(), vmin.z()}, {vmax.x(), vmax.y(), vmin.z()},
        {vmax.x(), vmax.y(), vmax.z()}, {vmin.x(), vmax.y(), vmax.z()});

  write({vmin.x(), vmin.y(), vmin.z()}, {vmax.x(), vmin.y(), vmin.z()},
        {vmax.x(), vmax.y(), vmin.z()}, {vmin.x(), vmax.y(), vmin.z()});

  write({vmin.x(), vmin.y(), vmax.z()}, {vmax.x(), vmin.y(), vmax.z()},
        {vmax.x(), vmax.y(), vmax.z()}, {vmin.x(), vmax.y(), vmax.z()});
}

template <typename entity_t, typename value_t, std::size_t DIM>
std::ostream& Acts::AxisAlignedBoundingBox<entity_t, value_t, DIM>::svg(
    std::ostream& os, value_type w, value_type h, value_type unit,
    const std::string& label, const std::string& fillcolor) const
  requires(DIM == 2)
{
  static_assert(DIM == 2, "SVG is only supported in 2D");

  VertexType mid(w / 2., h / 2.);

  using transform_t = Eigen::Transform<value_t, DIM, Eigen::Affine>;

  transform_t trf = transform_t::Identity();
  trf.translate(mid);
  trf = trf * Eigen::Scaling(VertexType(1, -1));
  trf.scale(unit);

  auto draw_point = [&](const VertexType& p_, const std::string& color,
                        std::size_t r) {
    VertexType p = trf * p_;
    os << "<circle ";
    os << "cx=\"" << p.x() << "\" cy=\"" << p.y() << "\" r=\"" << r << "\"";
    os << " fill=\"" << color << "\"";
    os << "/>\n";
  };

  auto draw_rect = [&](const VertexType& center_, const VertexType& size_,
                       const std::string& color) {
    VertexType size = size_ * unit;
    VertexType center = trf * center_ - size * 0.5;

    os << "<rect ";
    os << "x=\"" << center.x() << "\" y=\"" << center.y() << "\" ";
    os << "width=\"" << size.x() << "\" height=\"" << size.y() << "\"";
    os << " fill=\"" << color << "\"";
    os << "/>\n";
  };

  auto draw_text = [&](const VertexType& center_, const std::string& text,
                       const std::string& color, std::size_t size) {
    VertexType center = trf * center_;
    os << "<text dominant-baseline=\"middle\" text-anchor=\"middle\" ";
    os << "fill=\"" << color << "\" font-size=\"" << size << "\" ";
    os << "x=\"" << center.x() << "\" y=\"" << center.y() << "\">";
    os << text << "</text>\n";
  };

  draw_rect(m_center, m_width, fillcolor);
  draw_point(m_vmin, "black", 2);
  draw_point(m_vmax, "black", 2);
  draw_text(m_center, label, "white", 10);

  return os;
}

template <typename box_t>
box_t* octree_inner(std::vector<std::unique_ptr<box_t>>& store,
                    std::size_t max_depth,
                    typename box_t::vertex_array_type envelope,
                    const std::vector<box_t*>& lprims, std::size_t depth) {
  using VertexType = typename box_t::VertexType;

  assert(!lprims.empty());
  if (lprims.size() == 1) {
    // just return
    return lprims.front();
  }

  if (depth >= max_depth) {
    // just wrap them all up
    auto bb = std::make_unique<box_t>(lprims, envelope);
    store.push_back(std::move(bb));
    return store.back().get();
  }

  std::array<std::vector<box_t*>, 8> octants;
  // calc center of boxes
  const auto [vmin, vmax] = box_t::wrap(lprims);
  VertexType glob_ctr = (vmin + vmax) / 2.;

  for (auto* box : lprims) {
    VertexType ctr = box->center() - glob_ctr;
    if (ctr.x() < 0 && ctr.y() < 0 && ctr.z() < 0) {
      octants[0].push_back(box);
      continue;
    }
    if (ctr.x() > 0 && ctr.y() < 0 && ctr.z() < 0) {
      octants[1].push_back(box);
      continue;
    }
    if (ctr.x() < 0 && ctr.y() > 0 && ctr.z() < 0) {
      octants[2].push_back(box);
      continue;
    }
    if (ctr.x() > 0 && ctr.y() > 0 && ctr.z() < 0) {
      octants[3].push_back(box);
      continue;
    }

    if (ctr.x() < 0 && ctr.y() < 0 && ctr.z() > 0) {
      octants[4].push_back(box);
      continue;
    }
    if (ctr.x() > 0 && ctr.y() < 0 && ctr.z() > 0) {
      octants[5].push_back(box);
      continue;
    }
    if (ctr.x() < 0 && ctr.y() > 0 && ctr.z() > 0) {
      octants[6].push_back(box);
      continue;
    }
    if (ctr.x() > 0 && ctr.y() > 0 && ctr.z() > 0) {
      octants[7].push_back(box);
      continue;
    }

    // not in any quadrant (numerics probably)
    octants[0].push_back(box);
  }

  std::vector<box_t*> sub_octs;
  for (const auto& sub_prims : octants) {
    if (sub_prims.size() <= 8) {
      if (sub_prims.empty()) {
        // done
      } else if (sub_prims.size() == 1) {
        sub_octs.push_back(sub_prims.front());
      } else {
        store.push_back(std::make_unique<box_t>(sub_prims, envelope));
        sub_octs.push_back(store.back().get());
      }
    } else {
      // recurse
      sub_octs.push_back(
          octree_inner(store, max_depth, envelope, sub_prims, depth + 1));
    }
  }

  if (sub_octs.size() == 1) {
    return sub_octs.front();
  }

  auto bb = std::make_unique<box_t>(sub_octs, envelope);
  store.push_back(std::move(bb));
  return store.back().get();
}

template <typename box_t>
box_t* Acts::make_octree(std::vector<std::unique_ptr<box_t>>& store,
                         const std::vector<box_t*>& prims,
                         std::size_t max_depth,
                         typename box_t::value_type envelope1) {
  static_assert(box_t::dim == 3, "Octree can only be created in 3D");

  using vertex_array_type = typename box_t::vertex_array_type;

  vertex_array_type envelope(vertex_array_type::Constant(envelope1));

  box_t* top = octree_inner(store, max_depth, envelope, prims, 0);
  return top;
}

template <typename T, typename U, std::size_t V>
std::ostream& Acts::operator<<(
    std::ostream& os, const Acts::AxisAlignedBoundingBox<T, U, V>& box) {
  return box.toStream(os);
}
