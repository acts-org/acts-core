// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

template <typename T>
void PlyVisualization3D<T>::vertex(const Vector3& vtx, Color color) {
  m_vertices.emplace_back(vtx.template cast<ValueType>(), color);
}

template <typename T>
void PlyVisualization3D<T>::face(const std::vector<Vector3>& vtxs,
                                 Color color) {
  FaceType idxs;
  idxs.reserve(vtxs.size());
  for (const auto& vtx : vtxs) {
    vertex(vtx, color);
    idxs.push_back(m_vertices.size() - 1);
  }
  m_faces.push_back(std::move(idxs));
}

template <typename T>
void PlyVisualization3D<T>::faces(const std::vector<Vector3>& vtxs,
                                  const std::vector<FaceType>& /*faces*/,
                                  Color color) {
  face(vtxs, color);
}

template <typename T>
void PlyVisualization3D<T>::line(const Vector3& a, const Vector3& b,
                                 Color color) {
  vertex(a, color);
  std::size_t idx_a = m_vertices.size() - 1;
  vertex(b, color);
  std::size_t idx_b = m_vertices.size() - 1;
  m_edges.emplace_back(std::make_pair(std::make_pair(idx_a, idx_b), color));
}

template <typename T>
void PlyVisualization3D<T>::write(const std::filesystem::path& path) const {
  std::ofstream os;
  std::filesystem::path objectpath = path;
  if (!objectpath.has_extension()) {
    objectpath.replace_extension(std::filesystem::path("ply"));
  }
  os.open(std::filesystem::absolute(objectpath).string());
  write(os);
  os.close();
}

template <typename T>
void PlyVisualization3D<T>::write(std::ostream& os) const {
  os << "ply\n";
  os << "format ascii 1.0\n";
  os << "element vertex " << m_vertices.size() << "\n";
  os << "property float x\n";
  os << "property float y\n";
  os << "property float z\n";
  os << "property uchar red\n";
  os << "property uchar green\n";
  os << "property uchar blue\n";
  os << "element face " << m_faces.size() << "\n";
  os << "property list uchar int vertex_index\n";
  os << "element edge " << m_edges.size() << "\n";
  os << "property int vertex1\n";
  os << "property int vertex2\n";
  os << "property uchar red\n";
  os << "property uchar green\n";
  os << "property uchar blue\n";
  os << "end_header\n";

  for (const std::pair<VertexType, Color>& vtx : m_vertices) {
    os << vtx.first.x() << " " << vtx.first.y() << " " << vtx.first.z() << " ";
    os << vtx.second[0] << " " << vtx.second[1] << " " << vtx.second[2] << "\n";
  }

  for (const FaceType& fc : m_faces) {
    os << fc.size();
    for (std::size_t i = 0; i < fc.size(); i++) {
      os << " " << fc[i];
    }
    os << "\n";
  }

  for (const std::pair<std::pair<std::size_t, std::size_t>, Color>& edge :
       m_edges) {
    std::pair<std::size_t, std::size_t> idxs = edge.first;
    os << idxs.first << " " << idxs.second << " ";
    os << edge.second[0] << " " << edge.second[1] << " " << edge.second[2]
       << "\n";
  }
}

template <typename T>
void PlyVisualization3D<T>::clear() {
  m_vertices.clear();
  m_faces.clear();
  m_edges.clear();
}
