// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

template <typename T>
void ObjVisualization3D<T>::vertex(const Vector3& vtx, Color color) {
  m_vertexColors[m_vertices.size()] = color;
  m_vertices.push_back(vtx.template cast<ValueType>());
}

template <typename T>
void ObjVisualization3D<T>::line(const Vector3& a, const Vector3& b,
                                 Color color) {
  if (color != Color{0, 0, 0}) {
    m_lineColors[m_lines.size()] = color;
  }
  // not implemented
  vertex(a, color);
  vertex(b, color);
  m_lines.push_back({m_vertices.size() - 2, m_vertices.size() - 1});
}

template <typename T>
void ObjVisualization3D<T>::face(const std::vector<Vector3>& vtxs,
                                 Color color) {
  if (color != Color{0, 0, 0}) {
    m_faceColors[m_faces.size()] = color;
  }
  FaceType idxs;
  idxs.reserve(vtxs.size());
  for (const auto& vtx : vtxs) {
    vertex(vtx, color);
    idxs.push_back(m_vertices.size() - 1);
  }
  m_faces.push_back(std::move(idxs));
}

template <typename T>
void ObjVisualization3D<T>::faces(const std::vector<Vector3>& vtxs,
                                  const std::vector<FaceType>& faces,
                                  Color color) {
  // No faces given - call the face() method
  if (faces.empty()) {
    face(vtxs, color);
  } else {
    if (color != Color{0, 0, 0}) {
      m_faceColors[m_faces.size()] = color;
    }
    auto vtxoffs = m_vertices.size();
    if (color != Color{0, 0, 0}) {
      m_vertexColors[m_vertices.size()] = color;
    }
    m_vertices.insert(m_vertices.end(), vtxs.begin(), vtxs.end());
    for (const auto& face : faces) {
      if (face.size() == 2) {
        m_lines.push_back({face[0] + vtxoffs, face[2] + vtxoffs});
      } else {
        FaceType rawFace = face;
        std::transform(rawFace.begin(), rawFace.end(), rawFace.begin(),
                       [&](std::size_t& iv) { return (iv + vtxoffs); });
        m_faces.push_back(rawFace);
      }
    }
  }
}

template <typename T>
void ObjVisualization3D<T>::write(const std::filesystem::path& path) const {
  std::ofstream os;
  std::filesystem::path objectpath = path;
  if (!objectpath.has_extension()) {
    objectpath.replace_extension(std::filesystem::path("obj"));
  }
  os.open(std::filesystem::absolute(objectpath).string());
  std::filesystem::path mtlpath = objectpath;
  mtlpath.replace_extension(std::filesystem::path("mtl"));

  const std::string mtlpathString = std::filesystem::absolute(mtlpath).string();
  os << "mtllib " << mtlpathString << "\n";
  std::ofstream mtlos;
  mtlos.open(mtlpathString);

  write(os, mtlos);
  os.close();
  mtlos.close();
}

template <typename T>
void ObjVisualization3D<T>::write(std::ostream& os) const {
  std::stringstream sterile;
  write(os, sterile);
}

template <typename T>
void ObjVisualization3D<T>::write(std::ostream& os, std::ostream& mos) const {
  std::map<std::string, bool> materials;

  auto mixColor = [&](const Color& color) -> std::string {
    std::string materialName;
    materialName = "material_";
    materialName += std::to_string(color[0]) + std::string("_");
    materialName += std::to_string(color[1]) + std::string("_");
    materialName += std::to_string(color[2]);

    if (!materials.contains(materialName)) {
      mos << "newmtl " << materialName << "\n";
      std::vector<std::string> shadings = {"Ka", "Kd", "Ks"};
      for (const auto& shd : shadings) {
        mos << shd << " " << std::to_string(color[0] / 256.) << " ";
        mos << std::to_string(color[1] / 256.) << " ";
        mos << std::to_string(color[2] / 256.) << " "
            << "\n";
      }
      mos << "\n";
    }
    return std::string("usemtl ") + materialName;
  };

  std::size_t iv = 0;
  Color lastVertexColor = {0, 0, 0};
  for (const VertexType& vtx : m_vertices) {
    if (m_vertexColors.contains(iv)) {
      auto color = m_vertexColors.find(iv)->second;
      if (color != lastVertexColor) {
        os << mixColor(color) << "\n";
        lastVertexColor = color;
      }
    }

    os << "v " << std::setprecision(m_outputPrecision)
       << m_outputScalor * vtx.x() << " " << m_outputScalor * vtx.y() << " "
       << m_outputScalor * vtx.z() << "\n";
    ++iv;
  }
  std::size_t il = 0;
  Color lastLineColor = {0, 0, 0};
  for (const LineType& ln : m_lines) {
    if (m_lineColors.contains(il)) {
      auto color = m_lineColors.find(il)->second;
      if (color != lastLineColor) {
        os << mixColor(color) << "\n";
        lastLineColor = color;
      }
    }
    os << "l " << ln.first + 1 << " " << ln.second + 1 << "\n";
    ++il;
  }
  std::size_t is = 0;
  Color lastFaceColor = {0, 0, 0};
  for (const FaceType& fc : m_faces) {
    if (m_faceColors.contains(is)) {
      auto color = m_faceColors.find(is)->second;
      if (color != lastFaceColor) {
        os << mixColor(color) << "\n";
        lastFaceColor = color;
      }
    }
    os << "f";
    for (std::size_t i = 0; i < fc.size(); i++) {
      os << " " << fc[i] + 1;
    }
    os << "\n";
    ++is;
  }
}

template <typename T>
void ObjVisualization3D<T>::clear() {
  m_vertices.clear();
  m_faces.clear();
  m_lines.clear();
  m_lineColors.clear();
  m_vertexColors.clear();
  m_faceColors.clear();
}
